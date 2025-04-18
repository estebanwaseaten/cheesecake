Cheesecake allows access to STM32 (or other ARM based) MCUs via the SW-Debug port using a raspberry pi as a programmer.

It is a learning by doing project. 
It is probably much simpler and safer to use established ways of programming your MCU (software provided by the manufacturer or OpenOCD).

This projects consists of two parts:
    1. Linux Kernel Module that is used to output a bit- and clock-stream from 2 raspberry pi GPIO pins. 
        These pins have to be connected to SW-DIO and SW-CLK of the MCU. Running this task in kernel space allows for better reliability regarding timing of the bitsream.
        The bitsream follows the ARM SWD protocol: each transaction consists of an 8bit command, 3 acknowledge bits and 32 data bits + 1 parity bit.
    2. Userspace application that sends bunches of commands to the kernel module via write and read operations

to build and install the kernel module execute 
    make
    sudo insmod SWDPI.ko
(you have to setup your linux system to be able to build modules first with all the headers and whatnot...)


to build and run the application execute 
    ./make.sh
    sudo ./cheesetest








Remarks:
The trickiest part for me was getting the bit order right (the SWD protocol is sends LSB first) and also I missed a lot of side notes in the ARM documentation of the SW debug port and memory access port.


