#include <linux/types.h>

#define RASPI_4_GPIO_BASE  0xfe200000

int initRaspi4( void );
int configPinRaspi4( uint8_t pin, uint32_t setting );
int setPinOutputRaspi4( uint8_t pin );
int setPinInputRaspi4( uint8_t pin );
int readPinRaspi4( uint8_t pin );
int setPinRaspi4( uint8_t pin );
int unsetPinRaspi4( uint8_t pin );
