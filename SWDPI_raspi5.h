#include <linux/types.h>

//#define RASPI_5_GPIO_BASE  0x0000001f000d0000
//#define RASPI_5_PADS_BASE  0x0000001f000f0000

int initRaspi5( void );
int configPinPullRaspi5( uint8_t pin, uint32_t setting );
int setPinOutputRaspi5( uint8_t pin );
int setPinInputRaspi5( uint8_t pin );
int readPinRaspi5( uint8_t pin );
int setPinRaspi5( uint8_t pin );
int unsetPinRaspi5( uint8_t pin );
int cleanupRaspi5( void );
