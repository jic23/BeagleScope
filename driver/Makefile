#
# Makefile written by Zubeen Tolani <ZeekHuge - zeekhuge@gmail.com>
# Copyright (C) 2016 Zubeen Tolani <ZeekHuge - zeekhuge@gmail.com>
#
#

obj-m += beaglescope_driver.o

KDIR?=/lib/modules/$(shell uname -r)/build
INSTALL?=/lib/modules/$(shell uname -r)/kernel/drivers/testing
OP=$(patsubst %.o,%.ko,$(obj-m))
MODNAME=$(patsubst %.o,%,$(obj-m))

.PHONY: all driver testing
all: driver testing

driver:
	@make -C $(KDIR) M=$(PWD) modules

testing:
	@cd testing && $(MAKE)

.PHONY: install install-driver install-testing
install: install-driver install-testing

install-driver: driver
	@mkdir -p $(INSTALL)
	@echo "  -	copying $(OP) driver to $(INSTALL)"
	@cp $(OP) $(INSTALL)
	@echo "  -	executing depmod -a"
	@depmod -a

install-testing: testing
	@cd testing && $(MAKE) install

.PHONY: load
load: install-driver
	@echo "  -	rmmod beaglescope_driver"
	@rmmod beaglescope_driver | true
	@echo "  -	modprobe beaglescope_driver"
	@modprobe beaglescope_driver

.PHONY: clean
clean:
	@make -C $(KDIR) M=$(PWD) clean
	@cd testing && $(MAKE) clean
