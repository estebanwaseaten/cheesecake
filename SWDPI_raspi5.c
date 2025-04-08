#include <linux/kernel.h>
#include <linux/of.h>	//device tree access
#include <linux/of_address.h>

#include "SWDPI_raspi5.h"
#include "SWDPI_base.h"


#define OFFSET_GPIO_RP1 0x00
#define OFFSET_PADS_RP1 0x8000     //0x20000 divide by four?

#define RP1_RW_OFFSET	0x0000    //normal read/write
#define RP1_XOR_OFFSET	0x400     //0x1000    // atomic XOR on write. read does not advance RF/RWF regs --> debugging FIFO
#define RP1_SET_OFFSET	0x800     //0x2000    // atomic bitmask set on write
#define RP1_CLR_OFFSET	0xC00     //0x3000    // atomic bitmask clear on write


volatile uint32_t    *gpio5Mem = NULL;
//volatile uint32_t    *pads5Mem = NULL;

int initRaspi5( void )
{
    pr_info("initRaspi5() \n");

    struct device_node *gpio_node = NULL;
	//gpio_node = of_find_compatible_node(NULL, NULL, "raspberrypi,gpiomem");        //this finds something on the soc, not the rp1 chip
    gpio_node = of_find_node_opts_by_path("/axi/pcie@1000120000/rp1/gpiomem@d0000", NULL );     //finds the correct node
	if( gpio_node == NULL )
	{
		pr_warn("could not find gpio node! \n");
        return -1;
	}

    /*int size;
    const char *propValue = of_get_property(gpio_node, "compatible", &size);
    if ( propValue == NULL)
    {
        pr_warn("could not find property! \n");
        return -1;
    }
    for (size_t i = 0; i < size; i++)
    {
        printk( "%c", *(char*)(propValue + i));
    }//*/

    //maybe backup registers... at some point

    uint64_t gpioaddr = 0;
    uint64_t gpiosize = 0;
    of_property_read_reg(gpio_node, 0, &gpioaddr, &gpiosize);
    printk( "gpio addr: %x size: %x \n", gpioaddr, gpiosize );

    gpio5Mem = (uint32_t*)of_iomap( gpio_node, 0 );  //maps whole node __>
    printk( "gpio memory mapped %x \n", *gpio5Mem );

    return 0;
}

int cleanupRaspi5( void )
{


    return 0;
}



int configPinPullRaspi5( uint8_t pin, uint32_t setting )
{
    if ( gpio5Mem == NULL )
    {
        return -1;
    }

    uint8_t padReg = pin + 1;

    gpio5Mem[padReg + OFFSET_PADS_RP1 + RP1_CLR_OFFSET] = 3 << 2;
    //myPADSdevice->regWrite( padReg + RP1_CLR_OFFSET, 3 << 2 );
    if( setting == 1 )  //pull up
    {
        gpio5Mem[padReg + OFFSET_PADS_RP1 + RP1_SET_OFFSET] = 1 << 3;
        //myPADSdevice->regWrite( padReg + RP1_SET_OFFSET, 1 << 3 );
    }
    else if( setting == 2 )
    {
        gpio5Mem[padReg + OFFSET_PADS_RP1 + RP1_SET_OFFSET] = 1 << 2;
        //myPADSdevice->regWrite( padReg + RP1_SET_OFFSET, 1 << 2 );
    }

    return 0;
}
int setPinOutputRaspi5( uint8_t pin )
{


    return 0;
}
int setPinInputRaspi5( uint8_t pin )
{

    return 0;
}
int readPinRaspi5( uint8_t pin )
{

    return 0;
}

int setPinRaspi5( uint8_t pin )
{

    return 0;
}
int unsetPinRaspi5( uint8_t pin )
{

    return 0;
}
