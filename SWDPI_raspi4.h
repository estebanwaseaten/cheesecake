
#ifndef SWDPI_RASPI4_H
#define SWDPI_RASPI4_H


#include <linux/types.h>

#define RASPI_4_GPIO_BASE  0xfe200000
//maybe we dont need, because we can use the device tree to map memory directly...


#define GPIO_INPUT   0
#define GPIO_OUTPUT  1
#define GPIO_ALT0    4
#define GPIO_ALT1    5
#define GPIO_ALT2    6
#define GPIO_ALT3    7
#define GPIO_ALT4    3
#define GPIO_ALT5    2

#define GPIO_RISING_EDGE_SYNC 0
#define GPIO_FALLING_EDGE_SYNC 1
#define GPIO_HIGH 2
#define GPIO_LOW 3
#define GPIO_RISING_EDGE_ASYNC 4
#define GPIO_FALLING_EDGE_ASYNC 5

#define GPIO_BCM2835_GPFSEL0    0
#define GPIO_BCM2835_GPFSEL1    1
#define GPIO_BCM2835_GPFSEL2    2
#define GPIO_BCM2835_GPFSEL3    3
#define GPIO_BCM2835_GPFSEL4    4
#define GPIO_BCM2835_GPFSEL5    5

#define GPIO_BCM2835_GPSET0     7
#define GPIO_BCM2835_GPSET1     8

#define GPIO_BCM2835_GPCLR0     10
#define GPIO_BCM2835_GPCLR1     11

#define GPIO_BCM2835_GPCLR0     10
#define GPIO_BCM2835_GPCLR1     11

#define GPIO_BCM2835_GPLEV0     13
#define GPIO_BCM2835_GPLEV1     14

#define GPIO_BCM2835_GPEDS0     16
#define GPIO_BCM2835_GPEDS1     17

#define GPIO_BCM2835_GPREN0     19
#define GPIO_BCM2835_GPREN1     20

#define GPIO_BCM2835_GPFEN0     22
#define GPIO_BCM2835_GPFEN1     23

#define GPIO_BCM2835_GPHEN0     25
#define GPIO_BCM2835_GPHEN1     26

#define GPIO_BCM2835_GPLEN0     28
#define GPIO_BCM2835_GPLEN1     29

#define GPIO_BCM2835_GPAREN0     31
#define GPIO_BCM2835_GPAREN1     32

#define GPIO_BCM2835_GPAFEN0     34
#define GPIO_BCM2835_GPAFEN1     35

#define GPIO_BCM2835_GPPUPPDN0   57
#define GPIO_BCM2835_GPPUPPDN1   58
#define GPIO_BCM2835_GPPUPPDN2   59
#define GPIO_BCM2835_GPPUPPDN3   60


int initRaspi4( void );
int configPinPullRaspi4( uint8_t pin, uint32_t setting );
int setPinOutputRaspi4( uint8_t pin );
int setPinInputRaspi4( uint8_t pin );
int readPinRaspi4( uint8_t pin );
int setPinRaspi4( uint8_t pin );
int unsetPinRaspi4( uint8_t pin );
int cleanupRaspi4( void );



#endif
