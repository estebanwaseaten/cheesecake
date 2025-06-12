// SPDX-License-Identifier: GPL-2.0-or-later
/*
*
*
* Copyright (C) 2025 Daniel Wegkamp
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "cheese_memoryaccess.h"
#include "cheese_registers.h"
#include "cheese_comSWD.h"
#include "cheese_devices.h"

extern int reply;

void fileprint( char *path, int wordsToRead )
{
    //binary dump:
    printf( "\ndump binary file:\n" );

    FILE *binFilePtr = NULL;

    binFilePtr = fopen( path, "rb");
    if( binFilePtr == NULL )
    {
        printf( "file does not exist!\n" );
    }

    //read complete words only
    uint8_t byteBuffer[4];
    uint32_t *wordBuffer = (uint32_t*)byteBuffer;
    int freadReply;

    bool done = false;
    printf( "words to read: %d\n", wordsToRead);
    int wordsRead = 0;

    if( wordsToRead == 0 )
    {
        wordsToRead = -1;
    }

    printf("\n0x%08X: ", 0);
    while( !done )
    {
        freadReply = fread( byteBuffer, sizeof(byteBuffer), 1 , binFilePtr );

        if( freadReply == 1 )   //all fine
        {
            printf("%08X ", wordBuffer[0]);
        }
        else
        {
            done = true;
        }

        wordsRead++;

        if( (wordsRead % 4) == 0 )
        {
            printf("\n0x%08X: ", wordsRead*4);
        }

        if( wordsRead == wordsToRead )      //max given
        {
            done = true;
        }
    }
}


//https://developer.arm.com/documentation/ka001375/latest/
/*
IMPORTANT NOTE FROM ARM:
Automatic address increment is only guaranteed to operate on the bottom 10-bits of the address held in the TAR. Auto address incrementing of bit [10] and beyond is implementation defined. This means that auto address incrementing at a 1KB boundary is implementation defined. For example, if TAR[31:0] is set to 0x14A4, and the access size is word, successive accesses to the DRW increment TAR to 0x14A8, 0x14A, and so on, up to the end of the 1KB range at 0x17FC. At this point, the auto-increment behavior on the next DRW access is implementation defined.

BUT THIS SHOUDL BE FINE... *(it fits to 400 though... )

M1: Increments and wraps within a 1KB address boundary, for example, for word incrementing from 0x1400-0x17FC.
If the start is at 0x14A0, then the counter increments to 0x17FC, wraps to 0x1400, then continues incrementing to 0x149C.

M3: same but with 4KB boundary

apparently this wraps... somehow...

do we have to somehow reset the auto increment... ??? so strange!
*/


int stmWriteAligned( uint32_t baseAddr, uint32_t wordCount, uint32_t *data )
{
    printf("stmWriteAligned()\n");
    if ( (baseAddr % 0x80) != 0 )
    {
        printf( "stmFetchAligned() error: baseAddr not aligned.\n");
        return -11;
    }
    comArray localCom;
    uint32_t wordsTransferred = 0;
    uint32_t wordsTransferredThisRun = 0;

    comArrayInit( &localCom );

    while( wordsTransferred < wordCount )      //loop until all words have been received
    {
        wordsTransferredThisRun = 0;    //important: when we are done, this variable is not updated anymore, so infinite loop would result if we do not set i t to zero.
        comArray_prepMemAccess( &localCom, 0x0, baseAddr + wordsTransferred * 4 );        //prepare transaction
        //comArrayAdd( &localCom, AP_WRITE3_CMD, 0x1 );          //do we need a dummy write?

        while( (wordsTransferred < wordCount) && ( wordsTransferredThisRun < 32 ) )   //?????aligns with 1024 bit boundary of the address increase... what if w e choose stupid offset???
        {
            comArrayAdd( &localCom, AP_WRITE3_CMD, data[wordsTransferred] );
            wordsTransferredThisRun++;
            wordsTransferred++;
        }

        reply = comArrayTransfer( &localCom );
        if( reply < 0 )
        {
            printf( "com Error: %d\nAborting!\n", reply );
            return reply;
        }
    }
    return 0;
}

//some weird overlapping reads

// base offset needs to be aligned to 128 (0x80) bytes (corresponds to 32 words which is the smallest mutliple that the device driver will transfer (when subtracting the initialisiation data))
// this ensures, that we stay within a region where the automatic address increase works
int stmReadAligned( uint32_t baseAddr, uint32_t wordCount, uint32_t *returnBuffer )      //*baseOffset = ( *baseAddr % 0x80 );
{
    //printf("stmReadAligned()\n");
    if ( (baseAddr % 0x80) != 0 )
    {
        printf( "stmFetchAligned() error: baseAddr not aligned.\n");
        return -11;
    }

    comArray localCom;
    uint32_t wordsTransferred = 0;
    uint32_t wordsTransferredThisRun = 0;
    uint32_t wordsStored = 0;

    uint8_t  progressIndicated = 0;

    comArrayInit( &localCom );

    while( wordsTransferred < wordCount )       //loop until all words have been received
    {
        wordsTransferredThisRun = 0;            //important: when we are done, this variable is not updated anymore, so infinite loop would result if we do not set it to zero.
        comArray_prepMemAccess( &localCom, 0x0, baseAddr + wordsStored * 4 );        //prepare transaction
        uint32_t startIndex = comArrayAdd( &localCom, AP_READ3_CMD, 0x0 );           //dummy read - does not count yet

        //actual transferred data has to be aligned to 1024 bit boundary
        while( (wordsTransferred < wordCount) && ( wordsTransferredThisRun < 32 ) )   //?????aligns with 1024 bit boundary of the address increase... what if w e choose stupid offset???
        {
            //wordsTransferredThisRun = comArrayAdd( &localCom, AP_READ3_CMD, 0x0 );
            comArrayAdd( &localCom, AP_READ3_CMD, 0x0 );
            wordsTransferredThisRun++;                      //only count actual data transfers, otherwise 32 does not align with the boundary
            wordsTransferred++;
        }

        //printf( "trabsfer prepared: currentIndex: %d, currentStart Index: %d\n",wordsTransferredThisRun,startIndex );

        reply = comArrayTransfer( &localCom );
        if( reply < 0 )
        {
            printf( "com Error: %d\nAborting!\n", reply );
            return reply;
        }

        for( int i = startIndex; i < wordsTransferredThisRun + startIndex; i++ )         //loop from this runs indices
        {
            returnBuffer[wordsStored] = comArrayRead( &localCom, i );
            wordsStored++;
        }

        float progress = 100.0*(float)wordsTransferred/(float)wordCount;
        if( progress > progressIndicated )
        {
            printf(".");
            progressIndicated++;
        }
    }

    printf("\n");

    return 0;
}

//there is some memory issue where 0x1000 and 0x1010
// are overwritten with 0x0000 and 0x0010
// 0x2000 - 0x2030 are overwritten with 0x1000 - 0x1030

void align2mem( uint32_t *baseAddr, uint32_t *wordCount, uint32_t *baseOffset )
{
    //printf("align2mem()\n");

	*baseOffset = (*baseAddr % 0x80 );
	*baseAddr -= *baseOffset;
	*wordCount += *baseOffset / 4;

	if( *baseOffset != 0 )
    {
        printf( "automatically shifted base address by -0x%08X\n", *baseOffset);
        printf( "increased wordCount by %d\n", (*baseOffset/4));
    }
}

int stmPrint( uint32_t baseAddr, uint32_t wordCount )
{
    printf( "stmPrint: base: 0x%x, words: %d\n", baseAddr, wordCount );

    uint32_t newWordCount = wordCount;
    uint32_t newBaseAddr = baseAddr;
    uint32_t baseOffset = 0;

    align2mem( &newBaseAddr, &newWordCount, &baseOffset );    // shifts base addr to new location: baseAddr - baseOffset and increases wordCount to account for the shift

    uint32_t wordOffset = baseOffset / 4;

    uint32_t *dataArray = calloc( newWordCount, sizeof(uint32_t) );      //64bits * 64 transfers
    //nt32_t *debugArray = calloc( newWordCount, sizeof(uint32_t) );

    printf( "newBaseAddr: %d newWordCount: %d \n", newBaseAddr, newWordCount);
    reply = stmReadAligned( newBaseAddr, newWordCount, dataArray );
    if( reply < 0 )
    {
        printf( "STM Read Error: %d\nAborting!\n", reply );
        return reply;
    }

    for (int i = wordOffset; i < newWordCount; i++)
    {
        if( ( i % 4 ) == 0 )
        {
            printf( "0x%08X: ", i*4 + newBaseAddr );
        }
        printf( "%08X ", dataArray[i] );
        //printf( "%08X ", dataArray[i] );                                   //do not display ACK

        if( ( ( i + 1 ) % 4) == 0 )
        {
            printf( "\n" );
        }
    }

    free( dataArray );
    return 0;
}

int stmFetch( uint32_t baseAddr, uint32_t wordCount, uint32_t *words )
{
    //printf("stmFetch()\n");
    uint32_t newWordCount = wordCount;
    uint32_t newBaseAddr = baseAddr;
    uint32_t baseOffset = 0;
    align2mem( &newBaseAddr, &newWordCount, &baseOffset );    // shifts base addr to new location: baseAddr - baseOffset and increases wordCount to account for the shift

    uint32_t wordOffset = baseOffset / 4;

    uint32_t *dataArray = calloc( newWordCount, sizeof(uint32_t) );      //64bits * 64 transfers
    //uint32_t *debugArray = calloc( newWordCount, sizeof(uint32_t) );

    reply = stmReadAligned( newBaseAddr, newWordCount, dataArray );
    if( reply < 0 )
    {
        printf( "STM Read Error: %d\nAborting!\n", reply );
        return reply;
    }

    int wordCounter = 0;
    for( int i = wordOffset; i < newWordCount; i++ )
    {
        words[wordCounter] = dataArray[i];
        wordCounter++;
    }

    free( dataArray );
    //free( debugArray );
    return 0;
}


int stmDump( uint32_t baseAddr, uint32_t wordCount )        //wordCount is the number of words that are supposed to be displayed
{
    printf( "stmDump: base: 0x%x, words: %d\n", baseAddr, wordCount );

    //need 6 commands for setup + wordcount reads + 1 (because read is delayed) = 7 + wordcount reads
    //maximum commands that SWDPI supports: 64 --> 64-7 is 57 words can be read from the MCU
    char filenamestr[128];
    printf( "please enter filename: " );
    scanf( "%127[^\n]", filenamestr );

    //check if file exists
    FILE *file = fopen( filenamestr, "r");
    if( file )
    {
        fclose(file);
        printf( "file already exists - abort!\n" );
        return -1;
    }

    //file does not exist (OR we cannot read it...)
    file = fopen( filenamestr, "wb");

    uint32_t newWordCount = wordCount;
    uint32_t newBaseAddr = baseAddr;
    uint32_t baseOffset = 0;
    align2mem( &newBaseAddr, &newWordCount, &baseOffset );    // shifts base addr to new location: baseAddr - baseOffset and increases wordCount to account for the shift

    uint32_t wordOffset = baseOffset / 4;

    uint32_t *dataArray = calloc( newWordCount, sizeof(uint32_t) );      //64bits * 64 transfers
    //uint32_t *debugArray = calloc( newWordCount, sizeof(uint32_t) );

    printf( "newBaseAddr: %d newWordCount: %d \n", newBaseAddr, newWordCount);
    reply = stmReadAligned( newBaseAddr, newWordCount, dataArray );
    if( reply < 0 )
    {
        printf( "STM Read Error: %d\nAborting!\n", reply );
        return reply;
    }

    for( int i = wordOffset; i < newWordCount; i++ )
    {
        //printf( "%08X (%08X) ", dataArray[i], debugArray[i] );
        fwrite( &dataArray[i], sizeof(dataArray[i]), 1, file);
    }

    free( dataArray );
    //free( debugArray );
    fclose( file );
    return 0;
}

int stmWrite( uint32_t address, char* filenamestr )     //writing in RAM is ok. Writing in flash does not work like this...
{
    if( filenamestr == NULL )
    {
        return -1;
    }

    FILE *file = fopen( filenamestr, "r");
    if( !file )
    {
        printf( "file does not exist - abort!\n" );
        return -1;
    }

    fseek( file, 0, SEEK_END );
    int lengthBytes = ftell( file );
    fseek( file, 0, SEEK_SET );

    int lengthWords = ceil( lengthBytes/4 );

    printf( "%s contains %d bytes (%0.0f words)!\n", filenamestr, lengthBytes, 1.0*lengthBytes/4 );
    //uint8_t binary
    uint32_t *fileDataArray = calloc( lengthWords, sizeof(uint32_t) );
    int freadReply = fread( fileDataArray, sizeof(uint32_t), lengthWords, file );

    // find system
    initDevice();

    // 1. stop core:
    currentDeviceExecuteSequence( SEQ_HALT_ON_RESET );      //this sequences depend on the processor
    currentDeviceExecuteSequence( SEQ_RESET );

    sleep(1);
    // 2. write to RAM
    reply = stmWriteAligned( address, lengthWords, fileDataArray );


    free( fileDataArray );
    fclose( file );
}

int stmExecute( uint32_t address, char* filenamestr )     //writing in RAM is ok and executing...
{
    if( filenamestr == NULL )
    {
        return -1;
    }

    FILE *file = fopen( filenamestr, "r");
    if( !file )
    {
        printf( "file does not exist - abort!\n" );
        return -1;
    }

    fseek( file, 0, SEEK_END );
    int lengthBytes = ftell( file );
    fseek( file, 0, SEEK_SET );

    int lengthWords = ceil( lengthBytes/4 );

    printf( "%s contains %d bytes (%0.0f words)!\n", filenamestr, lengthBytes, 1.0*lengthBytes/4 );
    //uint8_t binary
    uint32_t *fileDataArray = calloc( lengthWords, sizeof(uint32_t) );
    int freadReply = fread( fileDataArray, sizeof(uint32_t), lengthWords, file );

    // find system
    initDevice();

    // 1. stop core:
    currentDeviceExecuteSequence( SEQ_HALT_ON_RESET );      //this sequences depend on the processor
    currentDeviceExecuteSequence( SEQ_RESET );

    //sleep(1);
    // 2. write to RAM
    reply = stmWriteAligned( address, lengthWords, fileDataArray );


    // 3. set program counter and start core:
    // a) Update vector table entry in 0xe000ed08 to SRAM start position 0x20000000.    //after reset vector table is at 0x0
    // b) Update R15(PC) with reset vector address. It locates at second word position in firmware.
    //      set Program Counter to vector_table[1] ?
    // c) Update R13(SP) with stack address defined in first word in firmware.
    //      set stack pointer to vector_table[0] maybe
    //in my words:
    // a) make VTOR point to start of RAM (where we just wrote our binary data to - the binary data starts with the vector table)
    currentDeviceWriteDeviceRegister( DEV_REG_VTOR, 0x20000000 );
    // b) set SP register to first word of vector table (index 0)
    currentDeviceWriteCoreRegister( CORE_REG_SP, fileDataArray[0] );
    // c) set PC register to second word of vector table (index 1)
    currentDeviceWriteCoreRegister( CORE_REG_DBG_RET, fileDataArray[1] );   //that is where we return to after leaving debug?


    currentDeviceExecuteSequence( SEQ_UNHALT );


    free( fileDataArray );
    fclose( file );
}

int stmErase( uint32_t baseAddr, uint32_t wordCount )
{
    uint32_t *emptyWords;

    emptyWords = (uint32_t*)calloc( wordCount, sizeof( uint32_t ) );

    initDevice();
    currentDeviceExecuteSequence( SEQ_HALT_ON_RESET );      //this sequences depend on the processor
    currentDeviceExecuteSequence( SEQ_RESET );

    reply = stmWriteAligned( baseAddr, wordCount, emptyWords );

    currentDeviceExecuteSequence( SEQ_UNHALT_ON_RESET );      //this sequences depend on the processor
    currentDeviceExecuteSequence( SEQ_RESET );

    free( emptyWords );
}
