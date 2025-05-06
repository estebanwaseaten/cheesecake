#include <linux/kernel.h>
#include <linux/of.h>	//device tree access
#include <linux/of_address.h>

#include "SWDPI_raspi4.h"
#include "SWDPI_SWD.h"
//#include "SWDPI_base.h"

volatile uint32_t    *gpio4Mem = NULL;

int initRaspi4( void )
{
    pr_info("initRaspi4() \n");

    struct device_node *gpio_node = NULL;
	gpio_node = of_find_node_by_name(NULL, "gpio");        //maybe better by path?

	if( gpio_node == NULL )
	{
		pr_warn("could not find gpio node! \n");
        return -1;
	}

/*    int size;
    const char *propValue = of_get_property(gpio_node, "reg", &size);
    if ( propValue == NULL)
    {
        pr_warn("could not find property! \n");
        return -1;
    }
    for (size_t i = 0; i < size; i++)
    {
        printk( "%x", *(char*)(propValue + i));
    }*/
    //set some default settings
	//need 2 default pins (gpio5 and gpio6) for clock and reserve some memory for transfers?
	clockPin = 5;
	//clockPinState = 0;
	dataPin = 6;
	//dataPinState = 0;

	speed_hz = 100000;	//100 kHz
	period_us = 1000000/speed_hz;
	half_period_us = period_us/2;

    uint64_t gpioaddr = 0;
    uint64_t gpiosize = 0;
    of_property_read_reg(gpio_node, 0, &gpioaddr, &gpiosize);
    printk( "gpio:reg addr: %x size: %x", gpioaddr, gpiosize );

    gpio4Mem = (uint32_t*)of_iomap( gpio_node, 0 );
    printk( "memory mapped %x", *gpio4Mem );

    configPinPullRaspi4( clockPin, GPIO_PULL_DOWN );       //this worked on the pi5, maybe we have to adjust when we read the data (at the very beginning of the clock or in the center...)
    configPinPullRaspi4( dataPin, GPIO_PULL_DOWN );

    setPinOutputRaspi4( clockPin );
    setPinOutputRaspi4( dataPin );

    unsetPinRaspi4( clockPin );         //we idle low now...
    unsetPinRaspi4( dataPin );

    return 0;
}

int cleanupRaspi4( void )
{
    unsetPinRaspi4( clockPin );
    unsetPinRaspi4( dataPin );
    return 0;
}


//what do we need this for?
int configPinPullRaspi4( uint8_t pin, uint32_t setting )        //what is setting
{
    uint8_t GPFSELx = pin / 16;             //one register contains selects for 10 pins
    uint8_t posInReg = pin - (GPFSELx*16);  //position within register: 0 -> 3 LSBs

    uint32_t registerMask = 0x3;                    //0b11
    //shift left:
    registerMask <<= posInReg*2;
    setting <<= posInReg*2;

    GPFSELx += 57; //offset to pull regs

    uint32_t newRegValue = (gpio4Mem[GPFSELx] & ~registerMask) | (setting & registerMask);
    gpio4Mem[GPFSELx] = newRegValue;

    return 0;
}

int setPinOutputRaspi4( uint8_t pin )
{
    uint8_t GPFSELx = pin / 10;             //one register contains selects for 10 pins
    uint8_t posInReg = pin - (GPFSELx*10);  //position within register: 0 -> 3 LSBs

    uint32_t registerMask = 0x7 << posInReg*3;
    uint32_t function = GPIO_OUTPUT << posInReg*3;

    uint32_t newRegValue = (gpio4Mem[GPFSELx] & ~registerMask) | (function & registerMask);
    gpio4Mem[GPFSELx] = newRegValue;

    return 0;
}

int setPinInputRaspi4( uint8_t pin )
{
    uint8_t GPFSELx = pin / 10;             //one register contains selects for 10 pins
    uint8_t posInReg = pin - (GPFSELx*10);  //position within register: 0 -> 3 LSBs

    uint32_t registerMask = 0x7 << posInReg*3;
    uint32_t function = GPIO_INPUT << posInReg*3;

    uint32_t newRegValue = (gpio4Mem[GPFSELx] & ~registerMask) | (function & registerMask);
    gpio4Mem[GPFSELx] = newRegValue;
    return 0;
}

int readPinRaspi4( uint8_t pin )
{
    uint32_t myReg;

    if( pin < 32 )
    {
        myReg = gpio4Mem[0xD];
        //cout << "getPin: " << hex << myReg << endl;
        //cout << "getMask: " << (1 << pin) << endl;
    }
    else
    {
        pin -=32;
        myReg = gpio4Mem[0xE];
    }

    if( ((1 << pin) & myReg) > 0 )
    {
        return 1;
    }

    return 0;
}

int setPinRaspi4( uint8_t pin )
{
    if( pin < 32 )
    {
        gpio4Mem[0x7] = (1 << pin);      //offset in bytes: 0x1c
    }
    else
    {
        pin -=32;
        gpio4Mem[0x8] = (1 << pin);      //0x20
    }
    return 0;
}
int unsetPinRaspi4( uint8_t pin )
{
    if( pin < 32 )
    {
        gpio4Mem[0xA] = (1 << pin);      //offset in bytes: 0x1c
    }
    else
    {
        pin -=32;
        gpio4Mem[0xB] = (1 << pin);      //0x20
    }
    return 0;
}
