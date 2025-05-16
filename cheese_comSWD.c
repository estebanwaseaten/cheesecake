// SPDX-License-Identifier: GPL-2.0-or-later
/*
*
*
* Copyright (C) 2025 Daniel Wegkamp
*/

#include <fcntl.h>      // O_RDWR | O_SYNC etc
#include <stdlib.h>
#include <unistd.h>     //close()
#include <stdio.h>      //printf()
#include <string.h>              //memset()

#include "cheese_comSWD.h"
#include "cheese_registers.h"


void comArrayInit( comArray *myComArray )
{
    if( myComArray->initDone == 0 )
    {
        myComArray->cmdArray32 = calloc( MAX_COMS, 8 );         //8 bytes each command
        myComArray->replyArray32 = calloc( MAX_COMS, 8 );
        myComArray->initDone = 1;
    }
    myComArray->filling = 0;
}

void comArrayDestroy( comArray *myComArray )
{
    if( myComArray->initDone != 0 )
    {
        free( myComArray->cmdArray32 );
        free( myComArray->replyArray32 );
        myComArray->initDone = 0;
    }
    myComArray->filling = 0;
}

int comArrayClear( comArray *myComArray )
{
    if( (myComArray == NULL) || (myComArray->initDone == 0) )
    {
        printf( "comArrayClear() NULL or not inited\n" );
        return -1;
    }
    myComArray->cmdArray32 = memset( myComArray->cmdArray32, 0x0, MAX_COMS*8 );
    myComArray->replyArray32 = memset( myComArray->cmdArray32, 0x0, MAX_COMS*8 );
    myComArray->initDone = 1;
    myComArray->filling = 0;

    return 0;
}

//prepare communication with access point (memory access)
int comArray_prepAPaccess( comArray *myComArray, uint8_t accessPort, uint8_t accessPortBank )    //selects AP [31:24] = 0x0, bank [7:4] = 0xF
{
    uint32_t selectRegister = (((uint32_t)accessPort) << 24) | (((uint32_t)accessPortBank) << 4);

    //printf("comArray_prepAPaccess(): selectRegister: 0x%08X\n", selectRegister);
    //clear com array
    comArrayClear( myComArray );

    comArrayAdd( myComArray, DP_IDCODE_CMD, 0x0 );                      //first command always idcode
    comArrayAdd( myComArray, DP_CTRLSTAT_R_CMD, 0x0 );                  //read potential sticky errors
    comArrayAdd( myComArray, DP_ABORT_CMD, 0x1E );                      //clean sticky errors

    //power up system and debug system:
    comArrayAdd( myComArray, DP_CTRLSTAT_W_CMD, 0x54000000 );           //system power-up requests [30], Debug power-up request [28], Debug reset request [26]. [30,28] = 0x50000000; [30,28,26] = 0x54000000

    //select Access Port and its Memory Bank
    return comArrayAdd( myComArray, DP_SELECT_CMD, selectRegister );
}

int comArray_prepMemAccess( comArray *myComArray, uint8_t accessPort, uint32_t memBase )
{
    //aceessing memory happens in the first register bank of the access port:
    comArray_prepAPaccess( myComArray, accessPort, 0 );

    //configure AP Control/Status Word register (CSW) register:
    comArrayAdd( myComArray, AP_WRITE0_CMD, 0x22000012 );               // CSW --> auto increment addr

    //write starting Transfer Address Register (TAR):
    int reply = comArrayAdd( myComArray, AP_WRITE1_CMD, memBase );

    return reply;
}

int comArrayAdd( comArray *myComArray, uint32_t cmd, uint32_t val )
{
    if( (myComArray == NULL) || (myComArray->initDone == 0) )
    {
        printf( "comArrayAdd() NULL or not inited\n" );
        return -1;
    }
    if ( myComArray->filling >= MAX_COMS )
    {
        printf( "comArrayAdd() full\n" );
        return -2;
    }
    myComArray->cmdArray32[myComArray->filling*2] = cmd;
    myComArray->cmdArray32[myComArray->filling*2 + 1] = val;
    myComArray->filling++;

    return myComArray->filling;
}

uint32_t comArrayRead( comArray *myComArray, uint32_t index )       //index starts at 0
{
    if( (myComArray == NULL) || (myComArray->initDone == 0) )
    {
        printf( "comArrayRead() NULL or not inited\n" );
        return -1;
    }
    if ( index >= myComArray->filling )
    {
        printf( "comArrayRead() index not available\n" );
        return -2;
    }

    return myComArray->replyArray32[index*2 + 1];
}

int comArrayTransfer( comArray *myComArray )
{
    //printf("transfer!\n");
    int SWDPIfile = open("/dev/SWDPI", O_RDWR | O_SYNC);
    write( SWDPIfile, myComArray->cmdArray32, myComArray->filling*2*4 );             //in bytes. each commandsDone has 4 bytes
    read( SWDPIfile, myComArray->replyArray32,  myComArray->filling*2*4 );

    //check for errors:
    for (size_t i = 0; i < myComArray->filling; i++)
    {
        if( (myComArray->replyArray32[i*2] >> 24) == 2 ) //if ACK not OK
        {
            printf( "SWD transfer error, received WAIT on transfer #%d\n", i);
            close( SWDPIfile );
            return -2;
        }
        else if( (myComArray->replyArray32[i*2] >> 24) == 4 ) //if ACK not OK
        {
            printf( "SWD transfer error, received FAIL on transfer #%d\n", i);
            close( SWDPIfile );
            return -4;
        }
    }

    close( SWDPIfile );
    return 0;
}
