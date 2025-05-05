//#include <iostream>
#include <stdint.h>
#include <fcntl.h>      // O_RDWR | O_SYNC etc
#include <unistd.h>     //close()
#include <stdio.h>
#include <stdbool.h>

#include "SWDPI_base.h"
#include "registers.h"

//for comparing:
// hexyl
// xxd -
// vbindiff


uint8_t cmd_helper( uint8_t ap, uint8_t read, uint8_t addr )
{
    addr >>= 2;

    uint8_t paritysum = ap + read + (((addr == 0) || (addr == 3 )) ? 0 : 1);

    // 0b10000001 |
    return 0x81 | (ap << 6) | (read << 5) | ((addr & 0x1) << 4) | ((addr & 0x2) << 2) | ((paritysum & 0x1) << 2);
}

uint64_t whole_cmd( uint8_t cmd, uint32_t data )
{
    uint64_t result = data | (((uint64_t)cmd) << 56);
    return result;
}


// read memory
void read_mem( int file, int addr, int length )
{

}


void read_ids( int file )       //reads some registers
{
    uint8_t cmdArray[7*8] = {
    // CMD: [7-0(cmd)], [15-8], [23-16], [31-24(ACK)],        DATA: [7-0], [15-8], [23-16], [31-24],
                          DP_IDCODE_CMD,        0x00, 0x00, 0x00,   0x00,  0x00,   0x00,    0x00,       //low bytes send first...
                          DP_CTRLSTAT_W_CMD,    0x00, 0x00, 0x00,   0x00,  0x00,   0x00,    0x50,       //bit 28 and 30  --> power up system & debug. seems to work    & seems to be necessary to read out MEMAP_READ3_CMD etc in the NEXT RUN
                          DP_CTRLSTAT_R_CMD,    0x00, 0x00, 0x00,   0x00,  0x00,   0x00,    0x00,
                          DP_SELECT_CMD,        0x00, 0x00, 0x00,   0xF0,  0x00,   0x00,    0x00,       // [7...4]   APBANKSEL  --> selects active 4 word register bank on current AP...

                          MEMAP_READ3_CMD,      0x00, 0x00, 0x00,   0x00,  0x00,   0x00,    0x00,       //send read cmd
                          MEMAP_READ2_CMD,      0x00, 0x00, 0x00,   0x00,  0x00,   0x00,    0x00,
                          DP_READBUF_CMD,       0x00, 0x00, 0x00,   0x00,  0x00,   0x00,    0x00,       //actually reads the AP ID
                      };


    //uint32_t cmdArray[6*2] =


    int32_t myReadBuffer[7*2];   //8 bytes per command --> 6*8=48 bytes is 12 32bit words
    int reply = 0;

    reply = write( file, cmdArray, 7*8 );                //write list of commands
    //printf( "write response: %d\n", reply );

    reply = read( file, &myReadBuffer, 7*8);   //read 4 bytes -->> always reads IDCODE
    //printf( "read response: %d\n", reply );

    printf( "IDCODE: 0x%08x (0x%08x)\n", myReadBuffer[2*0 + 1], myReadBuffer[2*0]);     //IDCODE
    printf( "MEM-AP info: \n");
    printf( "DEBUG BASE: 0x%08x (0x%08x)\n", myReadBuffer[2*5 + 1], myReadBuffer[2*5]);
    printf( "AP-IDR: 0x%08x (0x%08x)\n", myReadBuffer[2*6 + 1], myReadBuffer[2*6]);     //AP ID
}

//this could be implemented in the driver as some higher level CMD resulting in the whole memory read portion terminated by a 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00
void read_mcu_id( int file )
{
    uint8_t cmdArray[8*8] = {
    // CMD: [7-0(cmd)], [15-8], [23-16], [31-24(ACK)],        DATA: [7-0], [15-8], [23-16], [31-24],
                          DP_IDCODE_CMD,        0x00, 0x00, 0x00,   0x00,  0x00,   0x00,    0x00,       //low bytes send first...
                          DP_CTRLSTAT_W_CMD,    0x00, 0x00, 0x00,   0x00,  0x00,   0x00,    0x54,       //bit 26, 28 and 30  --> power up system & debug. seems to work    & seems to be necessary to read out MEMAP_READ3_CMD etc in the NEXT RUN
                          DP_CTRLSTAT_R_CMD,    0x00, 0x00, 0x00,   0x00,  0x00,   0x00,    0x00,       // 0x50 or 0x54
                          DP_SELECT_CMD,        0x00, 0x00, 0x00,   0x00,  0x00,   0x00,    0x00,       // [7...4]   APBANKSEL  --> selects active 4 word register bank on current AP...
//is his always correct:
                          MEMAP_WRITE0_CMD,     0x00, 0x00, 0x00,   0x12,  0x00,   0x00,    0x22,       //cmdArray32[8] = MEMAP_WRITE0_CMD; cmdArray32[9] = 0x22000012;
                          // write address:
                          MEMAP_WRITE1_CMD,     0x00, 0x00, 0x00,   0x00,  0x58,   0x01,    0x40,       // --> addr 0x40015800 is address of MCU device ID (maybe)
// preivous seems to fail...
                          MEMAP_READ3_CMD,      0x00, 0x00, 0x00,   0x00,  0x00,   0x00,    0x00,
// here we get a wait cmd
                          DP_READBUF_CMD,       0x00, 0x00, 0x00,   0x00,  0x00,   0x00,    0x00,
                      };

      int32_t myReadBuffer[8*2];   //8 bytes per command --> 6*8=48 bytes is 12 32bit words
      int reply = 0;

      reply = write( file, cmdArray, 8*8 );                //write list of commands
     // printf( "write response: %d\n", reply );

      reply = read( file, &myReadBuffer, 8*8);
      //printf( "read response: %d\n", reply );

      printf( "MCU ID: 0x%08x (0x%08x)\n", myReadBuffer[2*7 + 1], myReadBuffer[2*7]);     //AP ID
}

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

    printf("\n0x%08x: ", 0);
    while( !done )
    {
        freadReply = fread( byteBuffer, sizeof(byteBuffer), 1 , binFilePtr );

        if( freadReply == 1 )   //all fine
        {
            printf("%08x ", wordBuffer[0]);
        }
        else
        {
            done = true;
        }

        wordsRead++;

        if( (wordsRead % 4) == 0 )
        {
            printf("\n0x%08x: ", wordsRead*4);
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
void stmprint( int baseAddr, int wordCount )        //wordCount is the number of words that are supposed to be displayed
{
    printf( "stmdump: base: 0x%x, words: %d\n", baseAddr, wordCount );
    int SWDPIfile =  open("/dev/SWDPI", O_RDWR | O_SYNC);

    //need 6 commands for setup + wordcount reads + 1 (because read is delayed) = 7 + wordcount reads
    //maximum commands that SWDPI supports: 64 --> 64-7 is 57 words can be potentially read from the MCU in one running
    //BUT auto address increment only works in bunches of 1024 bits --> only extract 32 words per request --> we align withe the 1024 boundary automatically --
    // 7+ 32 = 39
    if( (baseAddr % 4) != 0 )
    {
        printf( "base Address has to by word aligned!\n");
        return;
    }

    //base Addr must be multiple of 0x80, otherwise the auto addr increase fails
    uint32_t baseOffset = (baseAddr % 0x80);
    baseAddr -= baseOffset;     //now it is aligned
    wordCount += baseOffset/4;  //add words otherwise we wont reach...
    if( baseOffset != 0 )
    {
        printf( "automatically shifted base address by -0x%08x\n", baseOffset);
    }


    uint32_t *cmdArray32 = malloc( 39*2*sizeof(uint32_t) );      //64bits * 64 transfers
    uint32_t *replyArray32 = malloc( 39*2*sizeof(uint32_t) );
    uint32_t addressCounter = 0;
    uint32_t wordsDone = 0;
    uint32_t commandsDone;
    int reply = 0;

    while( wordsDone < wordCount )      //read ONE word (4 bytes, 32bit) per transfer command. a transfer command has 64bit though.
    {
        printf( "bunch:\n" );
        cmdArray32[0] = DP_IDCODE_CMD; cmdArray32[1] = 0;
        cmdArray32[2] = DP_CTRLSTAT_W_CMD; cmdArray32[3] = 0x50000000;
        cmdArray32[4] = DP_CTRLSTAT_R_CMD; cmdArray32[5] = 0;
        cmdArray32[6] = DP_SELECT_CMD; cmdArray32[7] = 0;
        cmdArray32[8] = MEMAP_WRITE0_CMD; cmdArray32[9] = 0x22000012;
        cmdArray32[10] = MEMAP_WRITE1_CMD; cmdArray32[11] = baseAddr + addressCounter*4;                //this seems to be ok
        cmdArray32[12] = MEMAP_READ3_CMD; cmdArray32[13] = 0;   //initial read (no reply expected)
        commandsDone = 14;

        //fill in remaining read commands:
        while( (wordsDone < wordCount) && (commandsDone < 78) )   //aligns with 1024 bit boundary of the address increase... what if w e choose stupid offset???
        {
            cmdArray32[commandsDone] = MEMAP_READ3_CMD;
            commandsDone++;
            cmdArray32[commandsDone] = 0;       //we read
            commandsDone++;
            wordsDone ++;
        }
        //send and receive
        reply = write( SWDPIfile, cmdArray32, commandsDone*4 );             //in bytes. each commandsDone has 4 bytes
        reply = read( SWDPIfile, replyArray32, commandsDone*4 );

        //print:
        for( int i = 14; i < commandsDone; i+=2 )
        {
            if( addressCounter >= baseOffset/4 )
            {
                if( ( addressCounter % 4 ) == 0 )
                {
                    printf( "0x%08x: ", addressCounter*4 + baseAddr );
                }
                printf( "%08x(%08x) ", replyArray32[i+1], replyArray32[i] );      //should be +1

                if( ( ( addressCounter + 1 ) % 4) == 0 )
                {
                    printf( "\n" );
                }
            }
            addressCounter++;
        }
    }
    printf( "\n" );
    free( cmdArray32 );
    free( replyArray32 );

    close( SWDPIfile );
}

void stmdump( int baseAddr, int wordCount )        //wordCount is the number of words that are supposed to be displayed
{
    printf( "stmdump: base: 0x%x, words: %d\n", baseAddr, wordCount );
    int SWDPIfile =  open("/dev/SWDPI", O_RDWR | O_SYNC);

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
        return 0;
    }

    //file does not exist (OR we cannot read it...)
    file = fopen( filenamestr, "wb");

    if( (baseAddr % 4) != 0 )
    {
        printf( "base Address has to by word aligned!\n");
        return;
    }

    //base Addr must be multiple of 0x80, otherwise the auto addr increase fails
    uint32_t baseOffset = (baseAddr % 0x80);
    baseAddr -= baseOffset;     //now it is aligned
    wordCount += baseOffset/4;  //add words otherwise we wont reach...
    if( baseOffset != 0 )
    {
        printf( "automatically shifted base address by -0x%08x\n", baseOffset);
    }

    uint32_t *cmdArray32 = malloc( 2*64*sizeof(uint32_t) );      //64bits * 64
    uint32_t *replyArray32 = malloc( 2*64*sizeof(uint32_t) );
    uint32_t addressCounter = 0;
    uint32_t wordsDone = 0;
    uint32_t commandsDone;
    int reply = 0;

    while( wordsDone < wordCount )      //read ONE word (4 bytes, 32bit) per transfer command. a transfer command has 64bit though.
    {
        cmdArray32[0] = DP_IDCODE_CMD; cmdArray32[1] = 0;
        cmdArray32[2] = DP_CTRLSTAT_W_CMD; cmdArray32[3] = 0x50000000;
        cmdArray32[4] = DP_CTRLSTAT_R_CMD; cmdArray32[5] = 0;
        cmdArray32[6] = DP_SELECT_CMD; cmdArray32[7] = 0;
        cmdArray32[8] = MEMAP_WRITE0_CMD; cmdArray32[9] = 0x22000012;
        cmdArray32[10] = MEMAP_WRITE1_CMD; cmdArray32[11] = baseAddr + addressCounter*4;
        cmdArray32[12] = MEMAP_READ3_CMD; cmdArray32[13] = 0;   //initial read (no reply expected)
        commandsDone = 14;

        //fill in remaining reads:
        while( (wordsDone < wordCount) && (commandsDone < 78) )
        {
            cmdArray32[commandsDone] = MEMAP_READ3_CMD;
            commandsDone++;
            cmdArray32[commandsDone] = 0;       //we read
            commandsDone++;
            wordsDone ++;
        }
        //send and receive
        reply = write( SWDPIfile, cmdArray32, commandsDone*4 );             //in bytes. each commandsDone has 4 bytes
        reply = read( SWDPIfile, replyArray32, commandsDone*4 );

        //write to file:
        for( int i = 14; i < commandsDone; i+=2 )
        {
            if( addressCounter >= baseOffset/4 )
            {
                fwrite( &replyArray32[i+1], sizeof(replyArray32[i+1]), 1, file); // write 10 bytes from our buffer
            }

            addressCounter++;
        }
    }
    printf( "\n" );
    free( cmdArray32 );
    free( replyArray32 );

    fclose( file );

    close( SWDPIfile );
}


int main( int argc, char *argv[] )
{
    //printf("param1: %s\n", argv[1]);
    //printf("param2: %s\n", argv[2]);
    //printf("param3: %s\n", argv[3]);

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

    //printf("didit! %s, %s, %d, %d\n", argstr2, argstr3, (int)param2, (int)param3);

    if( strcmp(argstr1, "") == 0 )
    {
        int cake =  open("/dev/SWDPI", O_RDWR | O_SYNC);
        //read_ids( cake );
        read_mcu_id( cake );
    }
    else if( strcmp(argv[1], "-fileprint") == 0 ) // -filedump filename #ofWords
    {
        printf( "-fileprint\n");
        fileprint( argstr2, param3 );
    }
    else if ( strcmp(argv[1], "-stmprint") == 0 ) // -stmdump base-addr #ofWords
    {
        printf( "-stmprint\n");
        stmprint( param2, param3 );
    }
    else if ( strcmp(argv[1], "-stmdump") == 0 ) // -stmdump base-addr #ofWords
    {
        printf( "-stmdump\n");
        stmdump( param2, param3 );
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
