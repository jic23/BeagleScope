/*
 * Copyright (c) 2016 Zubeen Tolani <ZeekHuge - zeekhuge@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/rpmsg.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/cdev.h>
#include <linux/module.h>
#include <linux/kfifo.h>
#include <linux/uaccess.h>
#include <linux/poll.h>
#include <linux/iio/iio.h>

/*
 * macro to print debug info easily
 */
#define log_debug(msg) printk(KERN_DEBUG "%s: %s\n", __FILE__, msg);

#define RPMSG_BUF_SIZE		(512)
#define MAX_BLOCKS_IN_FIFO	(32)
#define FIFO_BLOCK_SIZE		RPMSG_BUF_SIZE

/* Configuration data, needed to be send to PRUs to get a raw sample */
#define BEAGLESCOPE_CONFIG_RAW_READ_0	0x00000000
#define BEAGLESCOPE_CONFIG_RAW_READ_1	0x00000000
#define BEAGLESCOPE_CONFIG_RAW_READ_2	0x00000000

#define OFFSET_REF_VDD 2
struct beaglescope_state {
	struct rpmsg_channel *rpdev;
	struct device *dev;
	u32 raw_data;
	bool got_raw;
	struct kfifo data_fifo;
	int data_idx;
	u32 data_length[MAX_BLOCKS_IN_FIFO];
	wait_queue_head_t wait_list;
};

/* beaglescope_adc_channels - structure that holds information about the
   channels that are present on the adc */
static const struct iio_chan_spec beaglescope_adc_channels[] = {
	{
		.type = IIO_VOLTAGE,
		.indexed = 1,
		.channel = 0,
		.info_mask_separate = BIT(IIO_CHAN_INFO_RAW) |
					BIT(IIO_CHAN_INFO_SCALE)|
					BIT(IIO_CHAN_INFO_OFFSET),
	},
};

/**
 * beaglescope_raw_read_from_pru() - function to read a single sample data
 *
 * The function writes necessary configuration data to the PRUs and waits for
 * the pru to provide back with the data. The configuration brings the PRUs in
 * single sample mode.
 *
 */
static int beaglescope_raw_read_from_pru(struct iio_dev *indio_dev, u32
					  *raw_data)
{
	int ret;
	struct beaglescope_state *st;
	static u32 beaglescope_config_raw_read[]={
		BEAGLESCOPE_CONFIG_RAW_READ_0,
		BEAGLESCOPE_CONFIG_RAW_READ_1,
		BEAGLESCOPE_CONFIG_RAW_READ_2};


	log_debug("raw_read_from_pru");

	st = iio_priv(indio_dev);

	ret = rpmsg_send(st->rpdev, (void *)beaglescope_config_raw_read,
			    sizeof(u32));
	if (ret)
		dev_err(st->dev, "beaglescope raw read from pru configuration failed\n");

	ret = wait_event_interruptible(st->wait_list, st->got_raw);
	if (ret)
		return -EINTR;

	*raw_data = st->raw_data;

	return ret;
}

static int beaglescope_read_raw(struct iio_dev *indio_dev,
               struct iio_chan_spec const *chan,
               int *val,
               int *val2,
               long mask)
{
       u32 regval = 0;

       log_debug("read_raw");

       switch (mask) {
       case IIO_CHAN_INFO_RAW:
	       beaglescope_raw_read_from_pru(indio_dev, &regval);
	       *val = regval;
	       return IIO_VAL_INT;
	case IIO_CHAN_INFO_SCALE:
	       *val = 1;
	       return IIO_VAL_INT;
	case IIO_CHAN_INFO_OFFSET:
	       *val = - OFFSET_REF_VDD;
	       return IIO_VAL_INT;
        default:
                return -EINVAL;
       }

}

/* beaglescope_info - Structure contains constant data about the driver */
static const struct iio_info beaglescope_info = {
	.read_raw = beaglescope_read_raw,
	.driver_module = THIS_MODULE,
};

/**
 * beaglescope_driver_cb() - function gets invoked each time the pru sends some
 * data.
 *
 */
static void beaglescope_driver_cb(struct rpmsg_channel *rpdev, void *data,
				  int len, void *priv, u32 src)
{
	struct beaglescope_state *st;
	struct iio_dev *indio_dev;

	log_debug("callback");

	indio_dev = dev_get_drvdata(&rpdev->dev);
	st = iio_priv(indio_dev);

	st->raw_data=*((u32 *)data);
	st->got_raw = 1;
	wake_up_interruptible(&st->wait_list);
}

/**
 * beaglescope_driver_probe() - function gets invoked when the rpmsg channel
 * as mentioned in the beaglescope_id table
 *
 * The function
 * - allocates space for the IIO device
 * - registers the device to the IIO subsystem
 * - exposes the sys entries according to the channels info
 */
static int beaglescope_driver_probe (struct rpmsg_channel *rpdev)
{
	int ret;
	struct iio_dev *indio_dev;
	struct beaglescope_state *st;
	struct rpmsg_device_id *id;

	log_debug("probe");

	indio_dev = devm_iio_device_alloc(&rpdev->dev, sizeof(*st));
	if (!indio_dev) {
		ret = -ENOMEM;
		goto error_ret;
	}

	id = &rpdev->id;
	st = iio_priv(indio_dev);

	st->rpdev = rpdev;
	st->dev = &rpdev->dev;

	dev_set_drvdata(&rpdev->dev, indio_dev);

	indio_dev->dev.parent = &rpdev->dev;
	indio_dev->name = id->name;
	indio_dev->info = &beaglescope_info;
	indio_dev->channels = beaglescope_adc_channels;
	indio_dev->num_channels = ARRAY_SIZE(beaglescope_adc_channels);

	ret = devm_iio_device_register(&rpdev->dev, indio_dev);
	if (ret < 0) {
		pr_err("Failed to register with iio\n");
		goto error_device_register;
	}

	init_waitqueue_head(&st->wait_list);

	return 0;

error_device_register:
	devm_iio_device_free(&rpdev->dev, indio_dev);
error_ret:
	return ret;
}

/**
 * beaglescope_driver_remove() - function gets invoked when the rpmsg device is
 * removed
 */
static void beaglescope_driver_remove(struct rpmsg_channel *rpdev)
{
	struct iio_dev *indio_dev;
	struct beaglescope_state *st;

	indio_dev = dev_get_drvdata(&rpdev->dev);
	st = iio_priv(indio_dev);

	devm_iio_device_unregister(&rpdev->dev, indio_dev);
	devm_iio_device_free(&rpdev->dev, indio_dev);
}

/* beaglescope_id - Structure that holds the channel name for which this driver
   should be probed */
static const struct rpmsg_device_id beaglescope_id[] = {
		{ .name = "beaglescope" },
		{ },
};
MODULE_DEVICE_TABLE(rpmsg, beaglescope_id);

/* beaglescope_driver - The structure containing the pointers to read/write
   functions to send data to the pru */
static struct rpmsg_driver beaglescope_driver= {
	.drv.name	= KBUILD_MODNAME,
	.drv.owner	= THIS_MODULE,
	.id_table	= beaglescope_id,
	.probe		= beaglescope_driver_probe,
	.callback	= beaglescope_driver_cb,
	.remove		= beaglescope_driver_remove,
};

/**
 * beaglescope_driver_init() : driver driver registration
 *
 * The initialization function gets invoked when the driver is loaded. The
 * function registers itself on the virtio_rpmsg_bus and it gets invoked when
 * the pru creates a channel named as in the beaglescope_id structure.
 */
static int __init beaglescope_driver_init(void)
{
	int ret;

	ret = register_rpmsg_driver(&beaglescope_driver);
	if (ret){
		pr_err("Failed to register beaglescope driver on rpmsg_bus\n");
		return ret;
	}

	pr_debug("Successfully registered to rpmsg_bus\n");

	return 0;
}

/**
 * beaglescope_driver_exit() - function invoked when the driver is unloaded
 */
static void __exit beaglescope_driver_exit(void)
{
	unregister_rpmsg_driver (&beaglescope_driver);
}

module_init(beaglescope_driver_init);
module_exit(beaglescope_driver_exit);

MODULE_AUTHOR("Zubeen Tolani <ZeekHuge - zeekhuge@gmail.com>");
MODULE_DESCRIPTION("BeagleScope Driver");
MODULE_LICENSE("GPL v2");
