#include <linux/kernel.h>
#include <linux/delay.h>

#include "SWDPI_SWD.h"
#include "SWDPI_raspi4.h"
#include "SWDPI_raspi5.h"

uint8_t	 clockPin;
uint8_t  dataPin;
uint32_t period_us;
uint32_t half_period_us;
uint32_t speed_hz;

struct gpio_interface SWDPI_gpio_interface =
{
	.init = NULL,
	.configPinPull = NULL,
	.setPinOutput = NULL,
	.setPinInput = NULL,
	.readPin = NULL,
	.setPin = NULL,
	.unsetPin = NULL,
	.cleanup = NULL
};

void SWDGPIOBBD_openDevice( void )
{

}

int SWDGPIOBBD_transfer( uint64_t *cmd )		//high level or low level transfer
{
	uint8_t lowLevelCmd = (((uint8_t*)cmd)[0]);
	uint8_t *ack = &(((uint8_t*)cmd)[3]);
	uint32_t *data = &(((uint32_t*)cmd)[1]);

	//1. connection sequence
	//SWDGPIOBBD_sequence( swd_sequence_jtag2swd, swd_sequence_jtag2swd_length );

	//pr_info("SWDGPIOBBD_transfer() before cmd: %x data: %x", (uint32_t)lowLevelCmd, *data );
	if( lowLevelCmd > 0 )	//low level transfer
	{
		if( lowLevelCmd & 0x4 )	//0 for write, 1 for read
		{
			//read
			pr_info("read operation!\n");
			SWDGPIOBBD_command( lowLevelCmd );
			SWDGPIOBBD_cycleTurnaround2Read();
			SWDGPIOBBD_receiveAck( ack );
			SWDGPIOBBD_receiveData( data );
			SWDGPIOBBD_cycleTurnaround2Write();
		}
		else
		{
			//write
			pr_info("write operation!\n");
			SWDGPIOBBD_command( lowLevelCmd );
			SWDGPIOBBD_cycleTurnaround2Read();
			SWDGPIOBBD_receiveAck( ack );
			SWDGPIOBBD_cycleTurnaround2Write();
			SWDGPIOBBD_sendData( data );	//write data!!!
		}
		//pr_info("SWDGPIOBBD_transfer() after cmd: %x data %x", (uint32_t)lowLevelCmd, *data );
	}
	else
	{
		return -100;	//whatever
	}
	return 0;
}




int SWDGPIOBBD_initRaspi( uint8_t raspi )
{
	int response = 0;
    switch ( raspi )
    {
        case 5:
            SWDPI_gpio_interface = (struct gpio_interface)
            {
                .init = initRaspi5,
                .configPinPull = configPinPullRaspi5,
                .setPinOutput = setPinOutputRaspi5,
                .setPinInput = setPinInputRaspi5,
                .readPin = readPinRaspi5,
                .setPin = setPinRaspi5,
                .unsetPin = unsetPinRaspi5,
                .cleanup = cleanupRaspi5
            };
			response = initRaspi5();
            break;
        case 4:
            SWDPI_gpio_interface = (struct gpio_interface)
            {
                .init = initRaspi4,
                .configPinPull = configPinPullRaspi4,
                .setPinOutput = setPinOutputRaspi4,
                .setPinInput = setPinInputRaspi4,
                .readPin = readPinRaspi4,
                .setPin = setPinRaspi4,
                .unsetPin = unsetPinRaspi4,
                .cleanup = cleanupRaspi4
            };
			response = initRaspi4();
            break;
        default:
            break;
    }
	return response;
}

void SWDGPIOBBD_sequence( uint8_t *seq, uint32_t seqLength )
{

   for (int j = 0; j < seqLength; j++)
   {
       //uint8_t mask = 0x80;
       for (int i = 0; i < 8; i++)
       {
           SWDGPIOBBD_cycleWrite( seq[j] & (1 << i) );
       }
   }
}

void SWDGPIOBBD_command( uint8_t cmd )
{
   for (size_t i = 0; i < 8; i++)
   {
       SWDGPIOBBD_cycleWrite( cmd & (1 << i));		//LSB first
   }
}

void SWDGPIOBBD_print_command( uint8_t cmd )
{
   pr_info("command order on wire: ");
   for (size_t i = 0; i < 8; i++)
   {
       pr_info("%d", (cmd & (1 << i)));
   }
   pr_info("\n");
}

void SWDGPIOBBD_receiveAck( uint8_t *ack )
{
   uint8_t ackreply = 0;
   //pr_info("ack order on wire: ");
   for (int i = 0; i < 3; i++)		//on the wire: LSB first
   {
       ackreply |= (SWDGPIOBBD_cycleRead() << i);
       //pr_info("%d", (ackreply & (1 << i)));
   }
   //pr_info("\n");
   *ack = ackreply;
}

//LSB first...
int SWDGPIOBBD_receiveData( uint32_t *data )
{
   uint32_t	bitRead;
   uint32_t datareply = 0;
   uint32_t parityCount = 0;

   for (int i = 0; i < 32; i++)
   {
       bitRead = SWDGPIOBBD_cycleRead();
       if( bitRead == 1 )
       {
           datareply |= (bitRead << i);
           parityCount++;
       }
   }
   bitRead = SWDGPIOBBD_cycleRead();	//parity bit

   *data = datareply;

   //pr_info("parityCount: %d parityBit: %d", parityCount, bitRead );
   if( !((parityCount & 1) ^ bitRead) ) //is odd and 1 or even and 0
   {
       //pr_info("parity matches!");
       return 0;
   }

   return -1;	//parity mismatch
}

//LSB first
int SWDGPIOBBD_sendData( uint32_t *data )
{
   uint32_t	bitWrite;
   uint32_t datareply = 0;
   uint32_t parityCount = 0;

   for (int i = 0; i < 32; i++)
   {
	   bitWrite = (*data >> i) & 0x1;

	   SWDGPIOBBD_cycleWrite( bitWrite );

	   parityCount += bitWrite;
   }
   SWDGPIOBBD_cycleWrite( parityCount & 0x1 );	//parity bit - 1 for odd

	return 0;
}



//// try a few things. 1. clock idles high, one clock cycle is off-on, change output on beginning of off-on cycle
// 			--> when clock is transitioning to high, the target can read
//			--> read at the bginning of the clock cycle (right after we transition low)


//these should be correct:
//https://developer.arm.com/documentation/dui0499/d/ARM-DSTREAM-Target-Interface-Connections/SWD-timing-requirements
uint8_t SWDGPIOBBD_cycleRead(void)
{
   //set data pin
   //SWDPI_gpio_interface.unsetPin( dataPin );	//just in case - should be unset anyways
   //unsset clock pin
   SWDPI_gpio_interface.unsetPin( clockPin );

   //read data pin
   uint8_t value = SWDPI_gpio_interface.readPin( dataPin );

   udelay(half_period_us);

   //set clock pin
   SWDPI_gpio_interface.setPin( clockPin );
   //udelay(1);	//to be safe???	now its too late
   //value += SWDPI_gpio_interface.readPin( dataPin );
   //read pin


   udelay(half_period_us);

   return (value != 0);
}

void SWDGPIOBBD_cycleWrite( uint8_t bit )
{
   //1. set data pin
   if( bit == 0 )
       SWDPI_gpio_interface.unsetPin( dataPin );
   else
       SWDPI_gpio_interface.setPin( dataPin );

   //1. unset clock pin
   SWDPI_gpio_interface.unsetPin( clockPin );
   //max delay here Tos = 5ns

   udelay(half_period_us);

   //set clock pin
   SWDPI_gpio_interface.setPin( clockPin );
   udelay(half_period_us);
}

void SWDGPIOBBD_cycleTurnaround2Read(void)
{
   //unset clock pin
   SWDPI_gpio_interface.unsetPin( dataPin );		//do not drive
   SWDPI_gpio_interface.unsetPin( clockPin );    //config 2 read:
   SWDPI_gpio_interface.setPinInput( dataPin );										//is this working??

//	SWDPI_gpio_interface.configPinPull( dataPin, GPIO_PULL_DOWN );     //not sure what is correct here
//	SWDPI_gpio_interface.configPinPull( dataPin, GPIO_PULL_DOWN );	//NONE

   //none---> reply is full height, DOWN---> reply is half.
   udelay(half_period_us);

   //set clock pin
   SWDPI_gpio_interface.setPin( clockPin );
   udelay(half_period_us);

}

void SWDGPIOBBD_cycleTurnaround2Write(void)
{
   //unset clock pin
   SWDPI_gpio_interface.unsetPin( dataPin );
   SWDPI_gpio_interface.unsetPin( clockPin );    //config 2 read:
   SWDPI_gpio_interface.setPinOutput( dataPin );

//    SWDPI_gpio_interface.configPinPull( dataPin, GPIO_PULL_DOWN );     //not sure what is correct here
   udelay(half_period_us);

   //set clock pin
   SWDPI_gpio_interface.setPin( clockPin );
   udelay(half_period_us);
}
