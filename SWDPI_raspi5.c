#include <linux/kernel.h>
#include <linux/of.h>	//device tree access
#include <linux/of_address.h>

#include "SWDPI_raspi5.h"
#include "SWDPI_SWD.h"
//#include "SWDPI_base.h"


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

    //set some default settings
    //need 2 default pins (gpio5 and gpio6) for clock and reserve some memory for transfers?
    clockPin = 5;
    //clockPinState = 0;
    dataPin = 6;
    //dataPinState = 0;

    speed_hz = 100000;	//100 kHz
    period_us = 1000000/speed_hz;
    half_period_us = period_us/2;



    //maybe backup registers... at some point

    uint64_t gpioaddr = 0;
    uint64_t gpiosize = 0;
    of_property_read_reg(gpio_node, 0, &gpioaddr, &gpiosize);
    printk( "gpio addr: %x size: %x \n", gpioaddr, gpiosize );

    gpio5Mem = (uint32_t*)of_iomap( gpio_node, 0 );  //maps whole node __>
    printk( "gpio memory mapped 0x%x \n", *gpio5Mem );

    configPinPullRaspi5( clockPin, GPIO_PULL_NONE );       //always leave it at pull down??
    configPinPullRaspi5( dataPin, GPIO_PULL_NONE );

    setPinOutputRaspi5( clockPin );
    setPinOutputRaspi5( dataPin );

    setPinRaspi5( clockPin );   //clock idles high
    unsetPinRaspi5( dataPin );

    return 0;
}

int cleanupRaspi5( void )
{
    unsetPinRaspi5( clockPin );
    unsetPinRaspi5( dataPin );
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
    //set in the pads region
    uint8_t pinReg = pin*2 + 1;
    uint8_t padReg = pin + 1;

    //regWrite( pinReg + RP1_CLR_OFFSET, 0x3F01F );     //clear INOVER, OEOVER, OUTOVER and FUNCSEL
    gpio5Mem[ pinReg + RP1_CLR_OFFSET] = 0x3F01F;     //clear INOVER, OEOVER, OUTOVER and FUNCSEL

    gpio5Mem[OFFSET_PADS_RP1 + padReg + RP1_CLR_OFFSET] = (3 << 6);     //unsets output disable, input enable
    gpio5Mem[ pinReg + RP1_SET_OFFSET ] = ((3 << 14) | 0x1F);           //select function NULL and enable output

    return 0;
}

int setPinInputRaspi5( uint8_t pin )
{
    //set in the pads region
    uint8_t pinReg = pin*2 + 1;
    uint8_t padReg = pin + 1;

    //regWrite( pinReg + RP1_CLR_OFFSET, 0x3F01F );     //clear INOVER, OEOVER, OUTOVER and FUNCSEL
    gpio5Mem[ pinReg + RP1_CLR_OFFSET] = 0x3F01F;     //clear INOVER, OEOVER, OUTOVER and FUNCSEL

    gpio5Mem[OFFSET_PADS_RP1 + padReg + RP1_SET_OFFSET] = (3 << 6);     //unsets output disable, input enable
    gpio5Mem[ pinReg + RP1_SET_OFFSET ] = 0x1F;           //select function NULL and enable output

    return 0;
}

int readPinRaspi5( uint8_t pin )
{
    uint32_t regValue = 0;
    uint8_t pinReg = pin*2;

    regValue = gpio5Mem[pinReg];
    if( ( regValue & (1 << 23) ) > 0 )
    {
        return 1;
    }
    return 0;
}

int setPinRaspi5( uint8_t pin )
{
    uint8_t pinReg = pin*2 + 1;
    gpio5Mem[pinReg + RP1_SET_OFFSET] = (3 << 12);
    return 0;
}

int unsetPinRaspi5( uint8_t pin )
{
    uint8_t pinReg = pin*2 + 1;
    gpio5Mem[pinReg + RP1_CLR_OFFSET] = (3 << 12);
    gpio5Mem[pinReg + RP1_SET_OFFSET] = (2 << 12);
    return 0;
}
