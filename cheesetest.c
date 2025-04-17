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


int main( void )
{
    int cake =  open("/dev/SWDPI", O_RDWR | O_SYNC);

    if( cake < 0 )
    {
        printf( "failed to connect to driver!\n" );
    }


    //DATA is stored LSB ... but each byte is transmitted in order. To transmit a value of 0xFF00FF00, we have to write the value backwards here! 00FF00FF

    //idea:
    // 1. write 32 bit cmd + 32 bit data (or 0x0 for reads) into the file repeatedly
    // 2. read same amount of lines --> a) triggers communication and b) returns ack + data/0x0
    // CMD: 32bits, DATA: 32bits
    // CMD: [7-0(cmd)], [15-8], [23-16], [31-24(ACK)], DATA: [7-0], [15-8], [23-16], [31-24],               a bit of a weird ordering for byte arrays
    uint8_t test[128] = { DP_IDCODE_CMD,        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,                    //low bytes send first...
                          DP_CTRLSTAT_W_CMD,    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x50,        //bit 28 and 30   --> seems to work    & seems to be necessary to read out MEMAP_READ3_CMD etc in the NEXT RUN
                         // DP_CTRLSTAT_R_CMD,    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                          DP_SELECT_CMD, 0x00, 0x00, 0x00, 0xF0, 0x00, 0x00, 0x00,              // [7...4]   APBANKSEL  --> selects active 4 word register bank on current AP...
                          MEMAP_READ3_CMD, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,            //only works if DP_CTRLSTAT_W_CMD was written as above once before...
                          //MEMAP_READ0_CMD, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                          //DP_READBUF_CMD, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                          //DP_READRE_CMD, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                          //DP_READBUF_CMD, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                          DP_IDCODE_CMD, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                          DP_IDCODE_CMD, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                          DP_IDCODE_CMD, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                          DP_IDCODE_CMD, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                          DP_IDCODE_CMD, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00

                        };

    uint8_t commandCount = 5;

    //uint64_t test = 0xFFFFFFFFFFFFFFFE; //0xA5 << 56; //shift all to the left (left 8 bits are for direct commands)
    int reply = 0;
    uint32_t myReadBuffer[32];

    reply = write( cake, test, commandCount*8 );                //write list of commands
    printf( "write response: %d\n", reply );

    reply = read(cake, &myReadBuffer, commandCount*8);   //read 4 bytes -->> always reads IDCODE
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
    }


    //IDCODE



    close( cake );

}
