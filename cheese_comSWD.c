// SPDX-License-Identifier: GPL-2.0-or-later
/*
*
*
* Copyright (C) 2025 Daniel Wegkamp
*/

#include <fcntl.h>      // O_RDWR | O_SYNC etc
#include <stdlib.h>

#include <unistd.h>     //close() sleep()
#include <stdio.h>      //printf()
#include <string.h>              //memset()

#include "cheese_comSWD.h"
#include "cheese_registers.h"


void comArrayInit( comArray *myComArray )
{
    if( myComArray->initDone != 0 )
    {
        //printf( "Warning: com array initDone was 1 already\n");       //seems to happen quite frequently
    }

    myComArray->cmdArray32 = calloc( MAX_COMS, 8 );         //8 bytes each command
    myComArray->replyArray32 = calloc( MAX_COMS, 8 );
    myComArray->initDone = 1;
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
    //printf( "comArrayClear...%d, %d\n", myComArray->cmdArray32,  myComArray->replyArray32 );

    if( (myComArray == NULL) || (myComArray->initDone == 0) )
    {
        printf( "comArrayClear() NULL or not inited\n" );
        return -1;
    }
    //printf( "comArrayClear...before memset\n");
    memset( myComArray->cmdArray32, 0x0, MAX_COMS*8 );         //SEGMENTATION FAULT (sometimes)
    //printf( "comArrayClear...between memset\n");
    memset( myComArray->replyArray32, 0x0, MAX_COMS*8 );
    //printf( "comArrayClear...after memset\n");

    myComArray->initDone = 1;
    myComArray->filling = 0;

    return 0;
}




//prepare communication with access point (memory access)
int comArray_prepAPaccess( comArray *myComArray, uint8_t accessPort, uint8_t accessPortBank )    //selects AP [31:24] = 0x0, bank [7:4] = 0xF
{
    uint32_t selectRegister = (((uint32_t)accessPort) << 24) | (((uint32_t)accessPortBank) << 4);

    comArrayClear( myComArray );
    comArrayAdd( myComArray, DP_IDCODE_CMD, 0x0 );                      //first command always idcode
    comArrayAdd( myComArray, DP_CTRLSTAT_R_CMD, 0x0 );                  //read potential sticky errors
    comArrayAdd( myComArray, DP_ABORT_CMD, 0x1E );                      //clean sticky errors

    //power up system and debug system:
    comArrayAdd( myComArray, DP_CTRLSTAT_W_CMD, 0x50000000 );           //system power-up requests [30], Debug power-up request [28], Debug reset request [26]. [30,28] = 0x50000000; [30,28,26] = 0x54000000

    //select Access Port and its Memory Bank
    return comArrayAdd( myComArray, DP_SELECT_CMD, selectRegister );
}

int comArray_prepMemAccess( comArray *myComArray, uint8_t accessPort, uint32_t memBase )
{
    //aceessing memory happens in the first register bank of the access port:
    comArray_prepAPaccess( myComArray, accessPort, 0 );

    //configure AP Control/Status Word register (CSW) register:
    comArrayAdd( myComArray, AP_WRITE0_CMD, 0x22000012 );               // CSW --> auto increment addr 0x22000012 or 0xA2000012

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

int com_transferSequence( uint32_t *sequence )
{
    int SWDPIfile = open("/dev/SWDPI", O_RDWR | O_SYNC);
    if( SWDPIfile < 0 )
    {
        return -1;
    }

    if (sequence[0] == 0)
    {
        printf( "com_transferSequence() error: sequence empty!\n");
    }

    write( SWDPIfile, &sequence[1], sequence[0]*2*4 );             //in bytes. each commandsDone has 4 bytes
    read( SWDPIfile, &sequence[1], sequence[0]*2*4 );

    //check for errors:
    for (size_t i = 0; i < sequence[0]; i++)
    {
        if( (sequence[i*2 + 1] >> 24) == 2 ) //if ACK = wait
        {
            printf( "SWD transfer error, received WAIT on transfer #%d\n", i);
            close( SWDPIfile );
            return -2;
        }
        else if( (sequence[i*2 + 1] >> 24) == 4 ) //if ACK = fail
        {
            printf( "SWD transfer error, received FAIL on transfer #%d\n", i);
            close( SWDPIfile );
            return -4;
        }
    }

    close( SWDPIfile );
    return 0;
}

int comArrayTransfer( comArray *myComArray )
{

    //printf("transfer!\n");
    int SWDPIfile = open("/dev/SWDPI", O_RDWR | O_SYNC);
    if( SWDPIfile < 0 )
    {
        return -1;
    }

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

int comArray_getSWDerr()
{
    comArray myComArray;
    comArrayInit( &myComArray );
    //comArrayClear( &myComArray );
    comArrayAdd( &myComArray, DP_IDCODE_CMD, 0x0 );                      //first command always idcode
    comArrayAdd( &myComArray, DP_CTRLSTAT_R_CMD, 0x0 );                  //read potential sticky errors
    comArrayAdd( &myComArray, DP_ABORT_CMD, 0x1E );                      //reset
    comArrayAdd( &myComArray, DP_CTRLSTAT_W_CMD, 0x50000000 );           // power up debug and ... domain
    comArrayAdd( &myComArray, DP_SELECT_CMD, 0x0 );                      // select AP 0, bank 0
    comArrayAdd( &myComArray, AP_READ0_CMD, 0x0 );                       //read CSW
    comArrayAdd( &myComArray, AP_READ0_CMD, 0x0 );                       //twice?

    comArrayTransfer( &myComArray );

    printf( "CTRL/STAT: 0x%08X\n", comArrayRead( &myComArray, 1 ) );
    printf( "AP CSW rg: 0x%08X\n", comArrayRead( &myComArray, 6 ) );

    uint32_t result = comArrayRead( &myComArray, 1 );

    comArrayDestroy( &myComArray );
    return result;
}

uint32_t comArray_readWord( uint32_t addr )
{
    comArray myComArray;
    comArrayInit( &myComArray );

    comArrayAdd( &myComArray, DP_IDCODE_CMD, 0x0 );
    comArrayAdd( &myComArray, DP_CTRLSTAT_R_CMD, 0x0 );                  //read potential sticky errors
    comArrayAdd( &myComArray, DP_ABORT_CMD, 0x1E );
    comArrayAdd( &myComArray, DP_CTRLSTAT_W_CMD, 0x50000000 );  //write CTRL/STAT
    comArrayAdd( &myComArray, DP_SELECT_CMD, 0x0 );             //select AP0, bank0
    comArrayAdd( &myComArray, AP_WRITE0_CMD, 0x22000012 );       //write CSW

    comArrayAdd( &myComArray, AP_WRITE1_CMD, addr );
    comArrayAdd( &myComArray, AP_READ3_CMD, 0x0 );
    comArrayAdd( &myComArray, DP_READBUF_CMD, 0x0 );

    comArrayTransfer( &myComArray );
    uint32_t result = comArrayRead( &myComArray, 8 );
    comArrayDestroy( &myComArray );

    return result;
}

void comArray_writeWord( uint32_t addr, uint32_t word )
{
    comArray myComArray;
    comArrayInit( &myComArray );

    comArrayAdd( &myComArray, DP_IDCODE_CMD, 0x0 );
    comArrayAdd( &myComArray, DP_CTRLSTAT_R_CMD, 0x0 );                  //read potential sticky errors
    comArrayAdd( &myComArray, DP_ABORT_CMD, 0x1E );
    comArrayAdd( &myComArray, DP_CTRLSTAT_W_CMD, 0x50000000 );  //write CTRL/STAT
    comArrayAdd( &myComArray, DP_SELECT_CMD, 0x0 );             //select AP0, bank0
    comArrayAdd( &myComArray, AP_WRITE0_CMD, 0x22000012 );       //write CSW

    comArrayAdd( &myComArray, AP_WRITE1_CMD, addr );
    comArrayAdd( &myComArray, AP_WRITE3_CMD, word );
    comArrayAdd( &myComArray, AP_WRITE3_CMD, word );

    //anything more???
    comArrayTransfer( &myComArray );

    comArrayDestroy( &myComArray );

}
