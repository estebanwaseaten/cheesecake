Cheesecake allows access to STM32 (or other ARM based) MCUs via the SW-Debug port using a raspberry pi as a programmer.

It is a learning by doing project.
It is probably much simpler and safer to use established ways of programming your MCU (software provided by the manufacturer or OpenOCD).

This projects consists of two parts:

1. Linux Kernel Module that is used to output a bit- and clock-stream from 2 raspberry pi GPIO pins. These pins have to be connected to SW-DIO and SW-CLK of the MCU. Running this task in kernel space allows for better reliability regarding timing of the bitsream. The bitsream follows the ARM SWD protocol: each transaction consists of an 8bit command, 3 acknowledge bits and 32 data bits + 1 parity bit.
2. Userspace application that sends bunches of commands to the kernel module via write and read operations

to build and install the kernel module execute
> make
> sudo insmod SWDPI.ko

(you have to setup your linux system to be able to build modules first with all the headers and whatnot...)


to build and run the application execute
> ./make.sh
> sudo ./cheesetest

run sudo ./cheestest with following flags:

-info
-stmprint #start #length
-stmbinprint #start (prints 4 bytes)
-stmdump
-stmwrite #address filename (writes into RAM)
-stmexecute #address filename   
-stmerase

-fileprint


example:

sudo ./cheesecake -info
should return consistent information about the connected STM32. If not, there is probably an issue with cross-talk between the SWD cables.

    SW-DP IDR (IDCODE): 0x2BA01477
    * Cortex-M4 Core w. FPU (uncomfirmed)
    * ARM SW-DP version: 0x2, partno: 0xBA01
    * DP designer: 0x23B (ARM Ltd)
    scanning for access points:
    AP-IDR: 0x24770011
    AP-IDR: 0x00000000

    Found 1 Access Port(s):

    MEM-AP #0 (IDR=0x24770011, AMBA AHB3 bus):
    * Access Port Information:
    * BASE: 0xE00FF000 (BASE2: 0x00000000)
    * AP CFG: 0 (little endian = 0; big endian = 1)
    * format: 0 (1->ADIv5/0->legacy), present: 0
    * type: 1, variant: 1, class: 0x8, designer: 0x23B (ARM Ltd), revision: 0x2
    ...


sudo ./cheesecake -stmwrite 0x20000000 ../path/to/binary.bin
writes binary to memory location 0x2000 0000 (start of RAM)


sudo ./cheesecake -stmexecute 0x20000000 ../path/to/binary.bin
writes binary to memory location 0x2000 0000 (start of RAM)
and executes code

//does not work
sudo ./cheesecake -stmrun 0x20000000
writes binary to memory location 0x2000 0000 (start of RAM)



Remarks:
The trickiest part for me was getting the bit order right (the SWD protocol is sends LSB first) and also I missed a lot of side notes in the ARM documentation of the SW debug port and memory access port.

Also the timing is still not perfect -> investigate. Maybe the raspberry pi interrupts should be disabled in the kernel module during transfer...
