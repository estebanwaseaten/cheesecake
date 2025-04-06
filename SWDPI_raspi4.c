#include <linux/kernel.h>
#include <linux/of.h>	//device tree access
#include <linux/of_address.h>

#include "SWDPI_raspi4.h"

volatile uint32_t    *gpioMem = NULL;

int initRaspi4( void )
{
    pr_info("initRaspi4() \n");

    struct device_node *gpio_node = NULL;
	gpio_node = of_find_node_by_name(NULL, "gpio");

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

    uint64_t gpioaddr = 0;
    uint64_t gpiosize = 0;
    of_property_read_reg(gpio_node, 0, &gpioaddr, &gpiosize);
    printk( "addr: %x size: %x", gpioaddr, gpiosize );
    //of_get_parent(const struct device_node *node)
    //of_property_count_u8_elems(const struct device_node *np, const char *propname)
    //of_property_read_string_array
    //of_property_count_u8_elems
    //of_property_read_u8_array
    //of_property_count_elems_of_sizes
    //of_property_read_string

    gpioMem = (uint32_t*)of_iomap( gpio_node, 0 );
    printk( "memory mapped %x", *gpioMem );

    configPinPullRaspi4( 5, GPIO_PULL_DOWN );       //what to do here?
    configPinPullRaspi4( 6, GPIO_PULL_DOWN );

    setPinOutputRaspi4( 5 );
    setPinOutputRaspi4( 6 );

    unsetPinRaspi4( 5 );
    unsetPinRaspi4( 6 );

    return 0;
}

int cleanupRaspi4( void )
{
    unsetPinRaspi4( 5 );
    unsetPinRaspi4( 6 );

    return 0;
}


//what do we need this for?
int configPinPullRaspi4( uint8_t pin, uint32_t setting )
{
    uint8_t GPFSELx = pin / 16;             //one register contains selects for 10 pins
    uint8_t posInReg = pin - (GPFSELx*16);  //position within register: 0 -> 3 LSBs

    uint32_t registerMask = 0x3;                    //0b11
    //shift left:
    registerMask <<= posInReg*2;
    setting <<= posInReg*2;

    GPFSELx += 57; //offset to pull regs

    uint32_t newRegValue = (gpioMem[GPFSELx] & ~registerMask) | (setting & registerMask);
    gpioMem[GPFSELx] = newRegValue;

    return 0;
}
int setPinOutputRaspi4( uint8_t pin )
{
    uint8_t GPFSELx = pin / 10;             //one register contains selects for 10 pins
    uint8_t posInReg = pin - (GPFSELx*10);  //position within register: 0 -> 3 LSBs

    uint32_t registerMask = 0x7 << posInReg*3;
    uint32_t function = GPIO_OUTPUT << posInReg*3;

    uint32_t newRegValue = (gpioMem[GPFSELx] & ~registerMask) | (function & registerMask);
    gpioMem[GPFSELx] = newRegValue;

    return 0;
}
int setPinInputRaspi4( uint8_t pin )
{
    uint8_t GPFSELx = pin / 10;             //one register contains selects for 10 pins
    uint8_t posInReg = pin - (GPFSELx*10);  //position within register: 0 -> 3 LSBs

    uint32_t registerMask = 0x7 << posInReg*3;
    uint32_t function = GPIO_INPUT << posInReg*3;

    uint32_t newRegValue = (gpioMem[GPFSELx] & ~registerMask) | (function & registerMask);
    gpioMem[GPFSELx] = newRegValue;
    return 0;
}
int readPinRaspi4( uint8_t pin )
{
    uint32_t myReg;

    if( pin < 32 )
    {
        myReg = gpioMem[0xD];
        //cout << "getPin: " << hex << myReg << endl;
        //cout << "getMask: " << (1 << pin) << endl;
    }
    else
    {
        pin -=32;
        myReg = gpioMem[0xE];
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
        gpioMem[0x7] = (1 << pin);      //offset in bytes: 0x1c
    }
    else
    {
        pin -=32;
        gpioMem[0x8] = (1 << pin);      //0x20
    }
    return 0;
}
int unsetPinRaspi4( uint8_t pin )
{
    if( pin < 32 )
    {
        gpioMem[0xA] = (1 << pin);      //offset in bytes: 0x1c
    }
    else
    {
        pin -=32;
        gpioMem[0xB] = (1 << pin);      //0x20
    }
    return 0;
}
