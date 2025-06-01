// SPDX-License-Identifier: GPL-2.0-or-later
/*
*  Initial Strategy was: using the debug port to access all the possible information and somehow extract the exact model of the chip.
*  Easier might be: probe a few MCU device ID code locations and see if we can identify the chip agains a list of known chips...
*  Short term goal though is to get any data written onto the flash memory...
*
* Copyright (C) 2025 Daniel Wegkamp
*/

#include <stdint.h>     //uint32_t etc

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "cheese_utils.h"
#include "cheese_registers.h"
#include "cheese_comSWD.h"
#include "cheese_memoryaccess.h"
#include "cheese_systeminfo.h"

int reply;

int main( int argc, char *argv[] )
{
    char *nullstr = "";
    char *argstr1 = nullstr;
    char *argstr2 = nullstr;
    char *argstr3 = nullstr;

    long int param2 = 0;
    long int param3  = 0;

    char *ptr1;
    char *ptr2;

    if( argv[1] != NULL )
    {
        argstr1 = argv[1];

        if( argv[2] != NULL )
        {
            argstr2 = argv[2];
            param2 = strtol( argv[2], &ptr1, 0 );

            if( argv[3] != NULL )   //if 2 == NULL --> 3 is meaningless
            {
                argstr3 = argv[3];
                param3 = strtol( argv[3], &ptr2, 0 );
            }
        }
    }

    if( strcmp(argstr1, "") == 0 )
    {
        cheese_test();
        //int cake =  open("/dev/SWDPI", O_RDWR | O_SYNC);
        //read_ids( cake );
        //read_mcu_id( cake );
    }
    else if( strcmp(argv[1], "-info") == 0 ) // -filedump filename #ofWords
    {
        printf( "-info\n");
        detectSystem();
    }
    else if( strcmp(argv[1], "-fileprint") == 0 ) // -filedump filename #ofWords
    {
        printf( "-fileprint\n");
        fileprint( argstr2, param3 );
    }
    else if ( strcmp(argv[1], "-stmprint") == 0 ) // -stmdump base-addr #ofWords
    {
        //printf( "-stmprint\n");
        //stmprint( param2, param3 );
        stmPrint( param2, param3 );
    }
    else if ( strcmp(argv[1], "-stmdump") == 0 ) // -stmdump base-addr #ofWords
    {
        //printf( "-stmdump\n");
        stmDump( param2, param3 );
    }

    return 0;
}





//    https://qcentlabs.com/posts/swd_banger/

    //RASPI 4 with  STM32L053R8     cortex M0+  12bit 1.14 MSPS A/D
    //RASPI 5 with  STM32F401...    cortex-M4 w 12bit 2.4 MSPS A/D

    //ARM_IMPLEMENTER_ARM = 0x41,
    //CORTEX_M4_PARTNO     = ARM_MAKE_CPUID(ARM_IMPLEMENTER_ARM, 0xC24),
	//CORTEX_M0P_PARTNO    = ARM_MAKE_CPUID(ARM_IMPLEMENTER_ARM, 0xC60),

    //0x24770011 on devices with a Cortex-M3 or Cortex-M4 core
    //On devices with a Cortex-M0+ it should return 0x0477003

    //DATA is stored LSB ... but each byte is transmitted in order. To transmit a value of 0xFF00FF00, we have to write the value backwards here! 00FF00FF

    // IMPORTANT:
    // If you require the value from an AP register read, that read must be followed by one of:
    //      A second AP register read, with the appropriate AP selected as the current AP.
    //      A read of the DP Read Buffer.


    //idea:
    // 1. write 32 bit cmd + 32 bit data (or 0x0 for reads) into the file repeatedly
    // 2. read same amount of lines --> a) triggers communication and b) returns ack + data/0x0
    // CMD: 32bits, DATA: 32bitss
    // CMD: [7-0(cmd)], [15-8], [23-16], [31-24(ACK)],        DATA: [7-0], [15-8], [23-16], [31-24],               a bit of a weird ordering for byte arrays
/*    uint8_t test[128] = { DP_IDCODE_CMD,        0x00, 0x00, 0x00,   0x00,  0x00,   0x00,    0x00,                    //low bytes send first...
                          DP_CTRLSTAT_R_CMD,    0x00, 0x00, 0x00,   0x00,  0x00,   0x00,    0x00,
                          DP_CTRLSTAT_W_CMD,    0x00, 0x00, 0x00,   0x00,  0x00,   0x00,    0x50,   //bit 28 and 30  --> power up system & debug. seems to work    & seems to be necessary to read out MEMAP_READ3_CMD etc in the NEXT RUN
                          //DP_CTRLSTAT_R_CMD,    0x00, 0x00, 0x00,   0x00,  0x00,   0x00,    0x00,
                          DP_SELECT_CMD,        0x00, 0x00, 0x00,   0xF0,  0x00,   0x00,    0x00,   // [7...4]   APBANKSEL  --> selects active 4 word register bank on current AP...
                          //DP_CTRLSTAT_R_CMD,    0x00, 0x00, 0x00,   0x00,  0x00,   0x00,    0x00,
                          //MEMAP_WRITE0_CMD,     0x00, 0x00, 0x00,   0x12,  0x00,   0x00,    0x22,
                          //DP_CTRLSTAT_R_CMD,    0x00, 0x00, 0x00,   0x00,  0x00,   0x00,    0x00, // [31-24]   APSEL --> should be 0x00 for single AP
// single read after select is not sufficient!!!!

                          MEMAP_READ3_CMD,      0x00, 0x00, 0x00,   0x00,  0x00,   0x00,    0x00,
                          DP_READBUF_CMD,       0x00, 0x00, 0x00,   0x00,  0x00,   0x00,    0x00,       //actually reads the AP ID

                          DP_SELECT_CMD,        0x00, 0x00, 0x00,   0x00,  0x00,   0x00,    0x00,

                          MEMAP_WRITE0_CMD,     0x00, 0x00, 0x00,   0x12,  0x00,   0x00,    0x22,
                          MEMAP_WRITE0_CMD,     0x00, 0x00, 0x00,   0x12,  0x00,   0x00,    0x22,
                          MEMAP_READ0_CMD,      0x00, 0x00, 0x00,   0x00,  0x00,   0x00,    0x00,
                          DP_READBUF_CMD,       0x00, 0x00, 0x00,   0x00,  0x00,   0x00,    0x00,

                          //MEMAP_READ3_CMD,      0x00, 0x00, 0x00,   0x00,  0x00,   0x00,    0x00,
                          //DP_READBUF_CMD,       0x00, 0x00, 0x00,   0x00,  0x00,   0x00,    0x00,

                          //MEMAP_READ3_CMD,      0x00, 0x00, 0x00,   0x00,  0x00,   0x00,    0x00,
                          //DP_SELECT_CMD,        0x00, 0x00, 0x00,   0xF0,  0x00,   0x00,    0x00,
                        //  MEMAP_READ3_CMD,      0x00, 0x00, 0x00,   0x00,  0x00,   0x00,    0x00,   //only works if DP_CTRLSTAT_W_CMD was written as above once before...
                          //MEMAP_READ0_CMD,    0x00, 0x00, 0x00,   0x00,  0x00,   0x00,    0x00,
                          //DP_READBUF_CMD,     0x00, 0x00, 0x00,   0x00,  0x00,   0x00,    0x00,
                          //DP_READRE_CMD,      0x00, 0x00, 0x00,   0x00,  0x00,   0x00,    0x00,
                          //DP_READBUF_CMD,     0x00, 0x00, 0x00,   0x00,  0x00,   0x00,    0x00,
                          DP_IDCODE_CMD,        0x00, 0x00, 0x00,   0x00,  0x00,   0x00,    0x00,
                          DP_IDCODE_CMD,        0x00, 0x00, 0x00,   0x00,  0x00,   0x00,    0x00,
                          DP_IDCODE_CMD,        0x00, 0x00, 0x00,   0x00,  0x00,   0x00,    0x00,
                          DP_IDCODE_CMD,        0x00, 0x00, 0x00,   0x00,  0x00,   0x00,    0x00,
                          DP_IDCODE_CMD,        0x00, 0x00, 0x00,   0x00,  0x00,   0x00,    0x00

                      };      //how do I select the access port...*/

/*    uint8_t commandCount = 11;

    //uint64_t test = 0xFFFFFFFFFFFFFFFE; //0xA5 << 56; //shift all to the left (left 8 bits are for direct commands)
    int reply = 0;
    uint32_t myReadBuffer[32];

    reply = write( cake, test, commandCount*8 );                //write list of commands
    printf( "write response: %d\n", reply );

    reply = read(cake, &myReadBuffer, commandCount*8);          //read 4 bytes -->> always reads IDCODE
    printf( "read response: %d\n", reply );
    if ( reply < 0)
    {
        printf( "read error response: %d\n", reply );
    }
    else
    {
        for( size_t i = 0; i < commandCount; i++ )
        {
            uint8_t ack = ((uint8_t*)myReadBuffer)[i*8 + 3];
            uint8_t cmd = ((uint8_t*)myReadBuffer)[i*8 + 0];
            uint32_t data = ((uint32_t*)myReadBuffer)[2*i + 1];
            printf( "test transfer - cmd: 0x%x ack: 0x%x data: 0x%x\n", cmd, ack, data );
        }
    }*/
