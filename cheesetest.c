//#include <iostream>
#include <stdint.h>
#include <fcntl.h>      // O_RDWR | O_SYNC etc
#include <unistd.h>     //close()

#include "SWDPI_base.h"



int main( void )
{
    int cake =  open("/dev/SWDPI", O_RDWR | O_SYNC);

    if( cake < 0 )
    {
        printf( "failed to connect to driver!\n" );
    }

    //idea:
    // 1. write 32 bit cmd + 32 bit data (or 0x0 for reads) into the file repeatedly
    // 2. read same amount of lines --> a) triggers communication and b) returns ack + data/0x0

    uint8_t test[16] = { 0xA5, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0xA5, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

    //uint64_t test = 0xFFFFFFFFFFFFFFFE; //0xA5 << 56; //shift all to the left (left 8 bits are for direct commands)
    int reply = 0;
    uint32_t myReadBuffer[4];

    reply = write( cake, test, 16 );                //write a single cmd
    printf( "write response: %d\n", reply );



    reply = read(cake, &myReadBuffer, 16);   //read 4 bytes -->> always reads IDCODE
    if ( reply < 0)
    {
        printf( "read error response: %d\n", reply );
    }
    else
    {
        for( size_t i = 0; i < 2; i++ )
        {
            uint8_t ack = ((uint8_t*)myReadBuffer)[i*8 + 3];
            uint8_t cmd = ((uint8_t*)myReadBuffer)[i*8 + 0];
            uint32_t data = ((uint32_t*)myReadBuffer)[2*i + 1];
            printf( "test read data: 0x%x cmd: 0x%x ack: 0x%x \n", data, cmd, ack );
        }
    }



/*    myReadBuffer[0] = 0;
    myReadBuffer[1] = 0;

    reply = write( cake, test, 8 );                //write a single cmd
    printf( "write response: %d\n", reply );
    reply = read(cake, &myReadBuffer, 16);   //read 4 bytes -->> always reads IDCODE
    printf( "test read: %d 0x%x 0x%x \n", reply, myReadBuffer[0], myReadBuffer[1] );  */

    close( cake );

}
