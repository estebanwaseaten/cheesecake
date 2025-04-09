#include <linux/types.h>

//#define RASPI_5_GPIO_BASE  0x0000001f000d0000
//#define RASPI_5_PADS_BASE  0x0000001f000f0000

#define OFFSET_GPIO_RP1 0x00
#define OFFSET_PADS_RP1 0x8000     //0x20000 divide by four?

#define RP1_RW_OFFSET	0x0000    //normal read/write
#define RP1_XOR_OFFSET	0x400     //0x1000    // atomic XOR on write. read does not advance RF/RWF regs --> debugging FIFO
#define RP1_SET_OFFSET	0x800     //0x2000    // atomic bitmask set on write
#define RP1_CLR_OFFSET	0xC00     //0x3000    // atomic bitmask clear on write


int initRaspi5( void );
int configPinPullRaspi5( uint8_t pin, uint32_t setting );
int setPinOutputRaspi5( uint8_t pin );
int setPinInputRaspi5( uint8_t pin );
int readPinRaspi5( uint8_t pin );
int setPinRaspi5( uint8_t pin );
int unsetPinRaspi5( uint8_t pin );
int cleanupRaspi5( void );
