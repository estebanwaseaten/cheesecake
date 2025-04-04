#include <linux/kernel.h>

#include "SWDPI_raspi4.h"


int initRaspi4( void )
{
    pr_info("initRaspi4() \n");
    return 0;
}

int configPinRaspi4( uint8_t pin, uint32_t setting )
{
    return 0;
}
int setPinOutputRaspi4( uint8_t pin )
{
    return 0;
}
int setPinInputRaspi4( uint8_t pin )
{
    return 0;
}
int readPinRaspi4( uint8_t pin )
{
    return 0;
}
int setPinRaspi4( uint8_t pin )
{
    return 0;
}
int unsetPinRaspi4( uint8_t pin )
{
    return 0;
}
/*EXPORT_SYMBOL(initRaspi4);
EXPORT_SYMBOL(configPinRaspi4);
EXPORT_SYMBOL(setPinOutputRaspi4);
EXPORT_SYMBOL(setPinInputRaspi4);
EXPORT_SYMBOL(readPinRaspi4);
EXPORT_SYMBOL(setPinRaspi4);
EXPORT_SYMBOL(unsetPinRaspi4);*/
