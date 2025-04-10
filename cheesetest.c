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




    uint32_t myReadBuffer[2];

    read(cake, &myReadBuffer, 8);   //read 4 bytes -->> always reads IDCODE

    printf( "test read: 0x%x 0x%x \n", myReadBuffer[0], myReadBuffer[1] );

    close( cake );

}
