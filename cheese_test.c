// SPDX-License-Identifier: GPL-2.0-or-later
/*
*
*
* Copyright (C) 2025 Daniel Wegkamp
*/

#include <stdio.h>

#include "cheese_utils.h"
#include "cheese_systeminfo.h"
#include "cheese_memoryaccess.h"
#include "cheese_comSWD.h"
#include "cheese_registers.h"



// general debug and test function
void cheese_test( void )
{
    int reply = 0;
    uint32_t baseAddr = 0xE000E000;


//    uint32_t base = 0x08004000;
//    comArray_writeWord( base, 0xFFFF0000 );
//    printf( "test new comArray_readWord(): 0x%08X\n", comArray_readWord( base ) );


    //HALT ON RESET:
        /*To Halt on reset, it is necessary to:
        • enable the bit0 (VC_CORRESET) of the Debug and Exception Monitor Control Register
        • enable the bit0 (C_DEBUGEN) of the Debug Halting Control and Status Register
        */

    //read a few MCU device ID code locations:

    //STM32L0x3 RefManual: flash size reg address: 0x1FF8007C
    //STM32L0x3            unique ID base address: 0x1FF80050 (offsets 0x00, 0x04 and 0x14)
    //STM32L0x3            has a SW-DP IDCODE that contains the default ARM for Cortex-M0+ value:  0x0BC11477
    //STM32L0x3            MCU device ID code located at: 0x40015800 (value: Category 3 devices: 0x417 and Category 5 devices: 0x447)
    //STM32L0x3            Debug MCU configuration register (DBG_CR): 0x40015804
    //STM32L0x3            Debug MCU APB1 freeze register (DBG_APB1_FZ): 0x40015808
    //STM32L0x3            Debug MCU APB2 freeze register (DBG_APB2_FZ): 0x4001580C
    //STM32L0x3            Flash program memory starts at: 0x08000000
    //STM32L0x3            Data EEPROM starts at: 0x08080000
    //STM32L0x3            Information Block System Memory: 0x1FF00000 - 0x1FF00FFF
    //STM32L0x3            Information Block Factory Options: 0x1FF80020 - 0x1FF8007F
    //STM32L0x3            Information Block  User Option bytes: 0x1FF80000 - 0x1FF8001F

    //STM32L0x3            AHB FLASH registers: 0X40022000 - 0X400223FF
    //STM32L0x3                 Access control register (FLASH_ACR): 0x00
    //STM32L0x3                 Program and erase control register (FLASH_PECR): 0x04
    //STM32L0x3                 Power-down key register (FLASH_PDKEYR): 0x08
    //STM32L0x3                 PECR unlock key register (FLASH_PEKEYR): 0x0C
    //STM32L0x3                 Program and erase key register (FLASH_PRGKEYR): 0x10
    //STM32L0x3                 Option bytes unlock key register (FLASH_OPTKEYR): 0x14
    //STM32L0x3                 Status register (FLASH_SR): 0x18
    //STM32L0x3                 Option bytes register (FLASH_OPTR): 0x1C
    //STM32L0x3                 Write protection register 1 (FLASH_WRPROT1): 0x20
    //STM32L0x3                 Write protection register 2 (FLASH_WRPROT2): 0x80

    printf( "FLASH (FLASH_ACR): 0x%08X\n", comArray_readWord( 0X40022000 + 0x00 ) );
    printf( "FLASH (FLASH_PECR): 0x%08X\n", comArray_readWord( 0X40022000 + 0x04 ) );
    printf( "FLASH (FLASH_PDKEYR): 0x%08X\n", comArray_readWord( 0X40022000 + 0x08 ) );
    printf( "FLASH (FLASH_PEKEYR): 0x%08X\n", comArray_readWord( 0X40022000 + 0x0C ) );
    printf( "FLASH (FLASH_PRGKEYR): 0x%08X\n", comArray_readWord( 0X40022000 + 0x10 ) );
    printf( "FLASH (FLASH_OPTKEYR): 0x%08X\n", comArray_readWord( 0X40022000 + 0x14 ) );
    printf( "FLASH (FLASH_SR): 0x%08X\n", comArray_readWord( 0X40022000 + 0x18 ) );

    // READ memory: just read
    // WRITE/ERASE memory: The operation will complete when the EOP bit of FLASH_SR register rises
    // Before performing a write/erase operation, it is necessary to enable it.
    // 1. unlock (0) PELOCK bit of the FLASH_PECR register
    // 2. To write/erase the Flash program memory, unlock PRGLOCK bit of the FLASH_PECR register. The bit can only be unlocked when PELOCK is 0.

    // Unlocking the data EEPROM and the FLASH_PECR register
    // The following sequence is used to unlock the data EEPROM and the FLASH_PECR register:
    //  • Write PEKEY1 = 0x89ABCDEF to the FLASH_PEKEYR register
    //  • Write PEKEY2 = 0x02030405 to the FLASH_PEKEYR register

    // Unlocking the Flash program memory
    // An additional protection is implemented to write/erase the Flash program memory. A write access to the Flash program memory is granted by clearing PRGLOCK bit.
    // The following sequence is used to unlock the Flash program memory:
    //  • Unlock the FLASH_PECR register (see the Unlocking the data EEPROM and the FLASH_PECR register section).
    //  • Write PRGKEY1 = 0x8C9DAEBF to the FLASH_PRGKEYR register.
    //  • Write PRGKEY2 = 0x13141516 to the FLASH_PRGKEYR register.



    printf( "0x40015800 (MCU_IDCODE): 0x%08X\n", comArray_readWord( 0x40015800 ) & 0xFFF );      //STM32L053
    printf( "0x40015804 (DBG_CR): 0x%08X\n\n", comArray_readWord( 0x40015804 ) );      //STM32L053



    // correct settings for SW-AP register CTRL/STAT and Access Port CSW register might depend on MCU version...
    // core debug registers (DHCSR, DCRSR, DCRDR, DEMCR) are within SCS space (offset ... form SCS component base address)
    // to debug on reset: VC_CORRESET=DEMCR[0]=1 and C_DEBUGEN=DHCSR[0]=1


    //printf( "0xE0042000: 0x%08X\n", comArray_readWord( 0xE0042000 ) & 0xFFF );      //STM32F303
    //printf( "0xE00E4000: 0x%08X\n", comArray_readWord( 0xE00E4000 ) & 0xFFF );      //STM32H503
    //printf( "0xE000ED00: 0x%08X\n", comArray_readWord( 0xE00E4000 ) & 0xFFF );      //STM32H503

    printf( "FLASH (FLASH_PECR): 0x%08X\n", comArray_readWord( 0X40022000 + 0x04 ) );
    //unlock write:
    // get FLASH_PECR.PELOCK zero:

    comArray myCom;
    comArrayInit( &myCom );
    comArray_prepAPaccess( &myCom, 0, 0);
    comArrayAdd( &myCom, AP_WRITE0_CMD, 0x22000002 );               // CSW --> no auto increment
    //write starting Transfer Address Register (TAR):
    comArrayAdd( &myCom, AP_WRITE1_CMD, 0x40022000 + 0x0C );
    comArrayAdd( &myCom, AP_WRITE3_CMD, 0x89ABCDEF );               //fails
    comArrayAdd( &myCom, AP_WRITE3_CMD, 0x02030405 );

    comArrayTransfer( &myCom );

    //comArray_writeWord( 0x40022000 + 0x0C, 0x89ABCDEF );    //FLASH_PEKEYR
    //comArray_writeWord( 0x40022000 + 0x0C, 0x02030405 );    //FLASH_PEKEYR

    printf( "FLASH (FLASH_PECR): 0x%08X\n", comArray_readWord( 0X40022000 + 0x04 ) );

    comArray_writeWord( 0X40022000 + 0x10, 0x8C9DAEBF );    //FLASH_PRGKEYR
    comArray_writeWord( 0X40022000 + 0x10, 0x13141516 );    //FLASH_PRGKEYR

    printf( "FLASH (FLASH_PECR): 0x%08X\n", comArray_readWord( 0X40022000 + 0x04 ) );
    //comArray_writeWord( 0X40022000 + 0x04

 return;

    uint32_t data[64] = {0};
    uint32_t base = 0x08004000;

    stmReadAligned( base, 64, data );
    for( int i = 0; i < 8; i++ )
    {
        printf( "0x%08X: 0x%08X\n", base + 4*i, data[i]);
    }

    comArray_writeWord( base, 0xFFFF0000 );

    comArray_getSWDerr();

    stmReadAligned( base, 64, data );
    for( int i = 0; i < 8; i++ )
    {
        printf( "0x%08X: 0x%08X\n", base + 4*i, data[i]);
    }






    return;

    /* for the following to work we need
            - base address of the SCS component to access DHCSR etc


    */
    // write 0xA05F0003 into DHCSR to HALT the core.        (10100000010111110000000000000011)
    // set VC_CORERESET Bit in DEMCR to HALT after reset
    // write 0xFA050004 into AIRCR --> reset core and then HALT     (11111010000001010000000000000100)
    //write to DHCSR: 0xA05F0003
/*    comArray localComArray;
    comArrayInit( &localComArray );
    int nextIndex = comArray_prepMemAccess( &localComArray, 0x00, baseAddr + 0xDF0 );    //
    comArrayAdd( &localComArray, AP_WRITE3_CMD, 0xA05F0003 );       //HALT PROCESSOR

    comArrayAdd( &localComArray, AP_WRITE1_CMD, baseAddr + 0xDF0 ); //same addr again
    comArrayAdd( &localComArray, AP_WRITE3_CMD, 0xA05F0004 );       //set C_STEP = 1

    comArrayAdd( &localComArray, AP_WRITE1_CMD, baseAddr + 0xDFC );
    comArrayAdd( &localComArray, AP_WRITE3_CMD, 0x1 );

    comArrayAdd( &localComArray, AP_WRITE1_CMD, baseAddr + 0xD0C );  //AIRCR
    comArrayAdd( &localComArray, AP_WRITE3_CMD, 0xfa050004 );

    comArrayAdd( &localComArray, AP_WRITE1_CMD, baseAddr + 0xDF8 );  //
    comArrayAdd( &localComArray, AP_WRITE3_CMD, 0x20000119 );
    comArrayAdd( &localComArray, AP_WRITE1_CMD, baseAddr + 0xDF4 );  //
    comArrayAdd( &localComArray, AP_WRITE3_CMD, 0x0001000f );

    comArrayAdd( &localComArray, AP_WRITE1_CMD, baseAddr + 0xDF8 );  //
    comArrayAdd( &localComArray, AP_WRITE3_CMD, 0x20004000 );
    comArrayAdd( &localComArray, AP_WRITE1_CMD, baseAddr + 0xDF4 );  //
    comArrayAdd( &localComArray, AP_WRITE3_CMD, 0x0001000d );

    comArrayAdd( &localComArray, AP_WRITE1_CMD, baseAddr + 0xD08 );  //
    comArrayAdd( &localComArray, AP_WRITE3_CMD, 0x20000000  );

    reply = comArrayTransfer( &localComArray );
    if( reply < 0 )
    {
        printf( "com Error: %d\nAborting!\n", reply );
        return reply;
    }



    uint32_t data[64] = {0};
    uint32_t base = 0x08004000;

    stmReadAligned( base, 64, data );
    for( int i = 0; i < 8; i++ )
    {
        printf( "0x%08X: 0x%08X\n", base + 4*i, data[i]);
    }

    uint32_t data2[8] = {0xFFFFFFF0};
    stmWriteAligned( 0x08004000, 8, data2 );

    comArray_getSWDerr();

    stmReadAligned( base, 64, data );
    for( int i = 0; i < 8; i++ )
    {
        printf( "0x%08X: 0x%08X\n", base + 4*i, data[i]);
    }

    comArray_writeWord( base, 0xFFFF0000 );
    printf( "test new comArray_readWord(): 0x%08X\n", comArray_readWord( base + 8 ) );

    comArray_getSWDerr();

    nextIndex = comArray_prepMemAccess( &localComArray, 0x00, baseAddr + 0xDF0 );    //
    comArrayAdd( &localComArray, AP_WRITE3_CMD, 0xa05f0000 );       //UNHALT PROCESSOR
    comArrayAdd( &localComArray, AP_WRITE1_CMD, baseAddr + 0xDF0 );  //
    comArrayAdd( &localComArray, AP_READ3_CMD, 0x0 );
    comArrayAdd( &localComArray, AP_READ3_CMD, 0x0 );
    reply = comArrayTransfer( &localComArray );
    if( reply < 0 )
    {
        printf( "com Error: %d\nAborting!\n", reply );
        return reply;
    }
    printf( "DHCSR: 0x%08X \n", comArrayRead( &localComArray, nextIndex + 3 ) );*/





}
