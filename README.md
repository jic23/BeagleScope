[![GPL Licence](https://badges.frapsoft.com/os/gpl/gpl-150x33.png?v=102)](https://opensource.org/licenses/GPL-2.0/) 
![Open Source Love](https://badges.frapsoft.com/os/v1/open-source-150x25.png?v=102)
[![Gitter](https://badges.gitter.im/beagleboard/beagle-gsoc.svg)](https://gitter.im/beagleboard/beagle-gsoc?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge)

[![Travis](https://travis-ci.org/ZeekHuge/BeagleScope.svg)](https://travis-ci.org/ZeekHuge/BeagleScope)

    

# BeagleScope
The official repository for the GSoC-2016 project - [BeagleScope](https://zeekhuge.github.io/beaglescope.html). An introductory video to the project - [video](https://youtu.be/tdanTRSmq4E).

## What is BeagleScope ?
The project, as the part of GSoC-2016, aims to provide with software support to deploy the PRUSS subsystem on AM33xx processors, as a fast and parallel data acquisition units.The PRU cores being highly optimized for I/O operations and single cycle instruction execution (mostly), are an appropriate choice for bit-banging and offloading tasks. BeagleScope uses this very feature of PRUs to offload the parallel data transactions and fast data acquisition. This project provides:

1. Kernel support :	Kernel support will be in the form of an offload subsystem and a kernel module that will expose PRUs as 'parallel I/O bus'. The subsystem would manage all the transactions of the I/O unit, while the parallel bus unit will manage communication with the PRU core.
2. Firmware : The firmware will be the code for PRUs. The firmware would manage the communication on the PRU side and all the data I/O that PRU need to do. 

The overall software stack would allow developers to use PRUs for many cool applications. One of such applications is an Oscilloscope. This can be achieved using a fast ADC along with the developed software. This project will also try to develop a 20MHz oscilloscope as a test for the developed code. 

## This Branch - "port_to_4.4.12-ti-r31+" 
In kernel version 4.4.12-ti-r31, the RPMsg framework used ARM INTC in place of Mailboxes to generate kicks from, and to PRUs. This was a significant change and the code needed to be modified to work with this new RPMsg framework.
This branch has the code that works with kernel later than 4.4.12-ti-r31. To get examples for older version, checkout the [master branch](https://github.com/ZeekHuge/BeagleScope/tree/master).

## firmware/
firmware directory contains BeagleScope firmware code for PRUs (under developement).
BeagleScope uses these two PRU cores to sample data and transfer it to the Main Processing Unit (MPU).


## License
The kernel code is released under the [GPLv2](https://opensource.org/licenses/GPL-2.0/) license.

