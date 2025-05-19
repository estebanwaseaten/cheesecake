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



    // write 0xA05F0003 into DHCSR to HALT the core.        (10100000010111110000000000000011)
    // set VC_CORERESET Bit in DEMCR to HALT after reset
    // write 0xFA050004 into AIRCR --> reset core and then HALT     (11111010000001010000000000000100)
    //write to DHCSR: 0xA05F0003
    comArray localComArray;
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
    printf( "DHCSR: 0x%08X \n", comArrayRead( &localComArray, nextIndex + 3 ) );





}
