//#include <iostream>
#include <stdint.h>
#include <fcntl.h>      // O_RDWR | O_SYNC etc
#include <unistd.h>     //close()


int main( void )
{
    int cake =  open("/dev/SWDPI", O_RDWR | O_SYNC);

    if( cake < 0 )
    {
        printf( "failed to connect to driver!\n" );
    }


    uint32_t myReadBuffer[2];

    read(cake, &myReadBuffer, 8);   //read 4 bytes

    printf( "test read: 0x%x 0x%x \n", myReadBuffer[0], myReadBuffer[1] );

    close( cake );

}
