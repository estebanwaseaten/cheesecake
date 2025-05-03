//#include <iostream>
#include <stdint.h>
#include <fcntl.h>      // O_RDWR | O_SYNC etc
#include <unistd.h>     //close()
#include <stdio.h>

#include "SWDPI_base.h"
#include "registers.h"

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


void read_ids( int file )
{
    const uint8_t commandCount = 6;

    uint8_t cmdArray[6*8] = {
    // CMD: [7-0(cmd)], [15-8], [23-16], [31-24(ACK)],        DATA: [7-0], [15-8], [23-16], [31-24],
                          DP_IDCODE_CMD,        0x00, 0x00, 0x00,   0x00,  0x00,   0x00,    0x00,       //low bytes send first...
                          DP_CTRLSTAT_W_CMD,    0x00, 0x00, 0x00,   0x00,  0x00,   0x00,    0x50,       //bit 28 and 30  --> power up system & debug. seems to work    & seems to be necessary to read out MEMAP_READ3_CMD etc in the NEXT RUN
                          DP_CTRLSTAT_R_CMD,    0x00, 0x00, 0x00,   0x00,  0x00,   0x00,    0x00,
                          DP_SELECT_CMD,        0x00, 0x00, 0x00,   0xF0,  0x00,   0x00,    0x00,       // [7...4]   APBANKSEL  --> selects active 4 word register bank on current AP...

                          MEMAP_READ3_CMD,      0x00, 0x00, 0x00,   0x00,  0x00,   0x00,    0x00,       //send read cmd
                          DP_READBUF_CMD,       0x00, 0x00, 0x00,   0x00,  0x00,   0x00,    0x00,       //actually reads the AP ID
                      };


    int32_t myReadBuffer[6*2];   //8 bytes per command --> 6*8=48 bytes is 12 32bit words
    int reply = 0;

    reply = write( file, cmdArray, commandCount*8 );                //write list of commands
    //printf( "write response: %d\n", reply );

    reply = read( file, &myReadBuffer, commandCount*8);   //read 4 bytes -->> always reads IDCODE
    //printf( "read response: %d\n", reply );

    printf( "IDCODE: 0x%x\n", myReadBuffer[2*0 + 1]);     //IDCODE
    printf( "AP=ID: 0x%x\n", myReadBuffer[2*5 + 1]);     //AP ID

}

//this could be implemented in the driver as some higher level CMD resulting in the whole memory read portion terminated by a 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00
void read_mcu_id( int file )
{
    const uint8_t commandCount = 7;

    uint8_t cmdArray[7*8] = {
    // CMD: [7-0(cmd)], [15-8], [23-16], [31-24(ACK)],        DATA: [7-0], [15-8], [23-16], [31-24],
                          DP_IDCODE_CMD,        0x00, 0x00, 0x00,   0x00,  0x00,   0x00,    0x00,       //low bytes send first...
                          DP_CTRLSTAT_W_CMD,    0x00, 0x00, 0x00,   0x00,  0x00,   0x00,    0x50,       //bit 28 and 30  --> power up system & debug. seems to work    & seems to be necessary to read out MEMAP_READ3_CMD etc in the NEXT RUN
                          DP_CTRLSTAT_R_CMD,    0x00, 0x00, 0x00,   0x00,  0x00,   0x00,    0x00,
                          DP_SELECT_CMD,        0x00, 0x00, 0x00,   0x00,  0x00,   0x00,    0x00,       // [7...4]   APBANKSEL  --> selects active 4 word register bank on current AP...
                          // set auto increase?
                          // write address:
                          MEMAP_WRITE1_CMD,     0x00, 0x00, 0x00,   0x00,  0x58,   0x01,    0x40,       // --> addr 0x40015800 is address of MCU device ID

                          MEMAP_READ3_CMD,      0x00, 0x00, 0x00,   0x00,  0x00,   0x00,    0x00,
                          DP_READBUF_CMD,       0x00, 0x00, 0x00,   0x00,  0x00,   0x00,    0x00,
                      };

      int32_t myReadBuffer[7*2];   //8 bytes per command --> 6*8=48 bytes is 12 32bit words
      int reply = 0;

      reply = write( file, cmdArray, commandCount*8 );                //write list of commands
     // printf( "write response: %d\n", reply );

      reply = read( file, &myReadBuffer, commandCount*8);   //read 4 bytes -->> always reads IDCODE
      //printf( "read response: %d\n", reply );

      printf( "MCU ID: 0x%x\n", myReadBuffer[2*6 + 1]);     //AP ID
}

int main( void )
{
    int cake =  open("/dev/SWDPI", O_RDWR | O_SYNC);

    if( cake < 0 )
    {
        printf( "failed to connect to driver!\n" );
        close( cake );

        return -1;
    }

    read_ids( cake );
    printf( "\n" );
    read_mcu_id( cake );

    close( cake );

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
