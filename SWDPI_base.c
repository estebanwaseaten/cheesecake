// SPDX-License-Identifier: GPL-2.0-or-later
/*
 *
 *
 * Copyright (C) 2025 Daniel Wegkamp
 */

#include <linux/init.h>		//used for __init
#include <linux/module.h>
#include <linux/kernel.h>

#include <linux/uaccess.h>
#include <linux/fs.h>		//file operations
#include <linux/proc_fs.h>

#include <linux/device.h>	//for /dev/
#include <linux/cdev.h>
#include <linux/kdev_t.h>

#include <linux/of.h>	//device tree access

#include <linux/delay.h>
#include <linux/spinlock.h>


//#include <linux/gpio/driver.h>
#include "SWDPI_raspi4.h"
#include "SWDPI_raspi5.h"
#include "SWDPI_base.h"	//public header

MODULE_DESCRIPTION("SWD GPIO bitbang driver: SWDGPIOBBD");
MODULE_AUTHOR("dw42");
MODULE_LICENSE("GPL");
MODULE_VERSION("0:1.0");


static const uint8_t swd_sequence_jtag2swd[] =
{
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0x9E, 0xE7,			//LSB
	//0x79, 0xE7,		//MSB
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0x00
};

static const uint32_t swd_sequence_jtag2swd_length = 17; //17 bytes



void SWDGPIOBBD_sequence( uint8_t *seq, uint32_t seqLength );
void SWDGPIOBBD_print_command( uint8_t cmd );
void SWDGPIOBBD_command( uint8_t cmd );				//LSB!
void SWDGPIOBBD_receiveData( uint32_t *data );
void SWDGPIOBBD_receiveAck( uint8_t *ack );
void SWDGPIOBBD_cycleTurnaround2Read(void);
void SWDGPIOBBD_cycleTurnaround2Write(void);
uint8_t SWDGPIOBBD_cycleRead(void);
void SWDGPIOBBD_cycleWrite( uint8_t bit );

//device ID (major/minor)
dev_t SWDGPIOBBD_dev_id = 0;
//device class
static struct class *SWDGPIOBBD_dev_class;
//character device in /dev:
static struct cdev SWDGPIOBBD_cdev;

uint8_t SWDGPIOBBD_lock;

uint8_t	clockPin;
//uint8_t clockPinState;
uint8_t dataPin;
//uint8_t dataPinState;

uint32_t period_us;
uint32_t half_period_us;
uint32_t speed_hz;

// PROCESS PROCESS PROCESS
// define what is in /proc/...
//function executed when /proc/... is read...
static ssize_t SWDGPIOBBD_ProcRead( struct file* file, char __user* user_buffer, size_t count, loff_t* offset );
static struct proc_dir_entry* SWDGPIOBBD_proc_dir_entry;
static const struct proc_ops SWDGPIOBBD_pops =		//replace struct file_operations with struct proc_ops for kernel version 5.6 or later
{
	//.owner = THIS_MODULE,
	.proc_read = SWDGPIOBBD_ProcRead		//what happens when you read /proc/cheesecakedrivercd
};

// DEVICE DEVICE DEVICE
//define what happens with /dev/...
static int      SWDGPIOBBD_open(struct inode *inode, struct file *file);
static int      SWDGPIOBBD_release(struct inode *inode, struct file *file);
static ssize_t  SWDGPIOBBD_read(struct file *filp, char __user *buf, size_t len,loff_t * off);
static ssize_t  SWDGPIOBBD_write(struct file *filp, const char *buf, size_t len, loff_t * off);

static long SWDGPIOBBD_ioctl( struct file *file, unsigned int cmd, unsigned long arg );

static struct file_operations SWDGPIOBBD_fops =
{
	.owner          = THIS_MODULE,
	.read           = SWDGPIOBBD_read,
	.write          = SWDGPIOBBD_write,
	.open           = SWDGPIOBBD_open,
	.unlocked_ioctl = SWDGPIOBBD_ioctl,		//unlocked_ioctl was introduced. It lets each driver writer choose what lock to use instead
	.release        = SWDGPIOBBD_release,
};

struct gpio_interface
{
	int (*init) ( void );
	int (*configPinPull) (uint8_t, uint32_t);
	int (*setPinOutput) (uint8_t);
	int (*setPinInput) (uint8_t);
	int (*readPin) (uint8_t);
	int (*setPin) (uint8_t);
	int (*unsetPin) (uint8_t);
	int (*cleanup) (void);
};

struct gpio_interface SWDPI_gpio_interface =
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

// PARAMETER PARAMETER PARAMETER
//define what is happening in /sys/module/.../parameters
//param to be set in sysfs
int myTestParam = 0;
int testParam_callback( const char *val, const struct kernel_param *kp );
const struct kernel_param_ops my_param_ops =
{
        .set = &testParam_callback, // Use our setter ...
        .get = &param_get_int, // .. and standard getter
};
//generates parameter in /sys/module/.../parameters/...
module_param_cb(myTestParam, &my_param_ops, &myTestParam, S_IRUGO|S_IWUSR );		//whenever cb_valueETX is modified, the .set callback of my_param_ops is called
int testParam_callback( const char *val, const struct kernel_param *kp )
{
	int res = param_set_int(val, kp);
	if( res == 0 )
	{
			pr_info("callback called!");
			return 0;
	}
	return -1;
}



// /proc/read - not sure if we need this...
static ssize_t SWDGPIOBBD_ProcRead( struct file* file, char __user* user_buffer, size_t count, loff_t* offset )		// called via: cat /proc/cheesePI
{
	pr_info("Calling custom_read( struct file* file, char __user* user_buffer, size_t count, loff_t* offset ).");

	char greeting[] = "Hello You!\n";
	int  length_of_greeting = strlen( greeting );
	if( *offset > 0 )
	{
		return 0;
	}

	copy_to_user( user_buffer, greeting, length_of_greeting );
	*offset = length_of_greeting;

	return length_of_greeting;
}


// /dev/... access functions
static int SWDGPIOBBD_open(struct inode *inode, struct file *file)
{
	pr_info("SWDGPIOBBD_open()\n");

	if( SWDGPIOBBD_lock == 0 )
	{
		SWDGPIOBBD_lock = 1;

	}
	else
	{
		pr_info("SWDGPIOBBD_open() failure: already locked \n");
		return -1;
	}

	//correct chip has been selected
	SWDPI_gpio_interface.init();





/*	struct device_node *root_node = NULL;
	root_node = of_find_all_nodes(NULL);
	if( root_node != NULL )
	{
		pr_info("Found root node!!! \n");

		//char *propValue;
		int size;
		of_property_read_string()
		const char *propValue = of_get_property(root_node, "compatible", &size);
		if ( propValue == NULL)
		{
			pr_info("could not find property\n");
			return -1;
		}
		/*else
		{
			for (size_t i = 0; i < size; i++)
			{
				printk( "%c", *(char*)(propValue + i));
			}
		}
	}*/

	//set some default settings
	//need 2 default pins (gpio5 and gpio6) for clock and reserve some memory for transfers?
	clockPin = 5;
	//clockPinState = 0;
	dataPin = 6;
	//dataPinState = 0;

	speed_hz = 100000;	//100 kHz
	period_us = 1000000/speed_hz;
	half_period_us = period_us/2;



//    kmalloc(mem_size , GFP_KERNEL)	//other flags possible
//    kfree()
//    copy_from_user()
//    copy_to_user()
	return 0;
}

static int SWDGPIOBBD_release(struct inode *inode, struct file *file)
{
	pr_info("SWDGPIOBBD_release()\n");
	SWDGPIOBBD_lock = 0;



	return 0;
}

static ssize_t SWDGPIOBBD_read(struct file *filp, char __user *buf, size_t len, loff_t * off)
{
	pr_info("Driver Read Function Called...!!!\n");
	// e.g. copy_to_user(buf, kernel_buffer, mem_size);

	uint32_t	kernel_buffer[2] = {0x0, };
	uint32_t 	mem_size = 8;

	//1. >= 50 cycles high:

	uint32_t	data = 0;
	uint8_t		ack = 0;

	//use spinlock to not get interrupted during the transfer
	static spinlock_t lock;
	unsigned long flags;
	spin_lock_init(&lock);

	spin_lock_irqsave( &lock, flags);
	//read_lock_irqsave()
	//write_lock_irqsave()

	//SWDGPIOBBD_print_command( SWD_CMD_READ_IDCODE );

	SWDGPIOBBD_sequence( swd_sequence_jtag2swd, swd_sequence_jtag2swd_length );
	//SWDGPIOBBD_command( 0x79 );	// 0x9E
	//SWDGPIOBBD_command( 0xE7 );
	//SWDGPIOBBD_connectionSequence();	//if we were already in SWD state??
	//SWDGPIOBBD_command( SWD_CMD_READ_IDCODE );SWD_CMD_TARGETSEL
	SWDGPIOBBD_command( SWD_CMD_READ_IDCODE );
	SWDGPIOBBD_cycleTurnaround2Read();
	SWDGPIOBBD_receiveAck( &ack );
	SWDGPIOBBD_receiveData( &data );
	SWDGPIOBBD_cycleTurnaround2Write();
	//SWDGPIOBBD_send();

	spin_unlock_irqrestore( &lock, flags);
	//read_unlock_irqrestore()
	//write_unlock_irqrestore()

	/* After completing a transaction, the host must either insert idle cycles or continue immediately with the start bit of
	 * a new transaction. The host clocks the SWD interface with the line LOW to insert idle cycles.*/


	kernel_buffer[0] = ack;
	kernel_buffer[1] = data;


	copy_to_user(buf, &kernel_buffer, mem_size);
	return 0;
}

static ssize_t SWDGPIOBBD_write(struct file *filp, const char *buf, size_t len, loff_t * off)
{
	pr_info("Driver Write Function Called...!!!\n");
	// e.g. copy_from_user(kernel_buffer, buf, len);
	return len;
}

static long SWDGPIOBBD_ioctl( struct file *file, unsigned int cmd, unsigned long arg )
{
	return 0;
}

void SWDGPIOBBD_sequence( uint8_t *seq, uint32_t seqLength )
{

	for (int j = 0; j < seqLength; j++)
	{
		uint8_t mask = 0x80;
		for (int i = 0; i < 8; i++)
		{
			SWDGPIOBBD_cycleWrite( seq[j] & mask );
			mask >>= 1;
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
	pr_info("ack order on wire: ");
	for (int i = 0; i < 3; i++)		//on the wire: LSB first
	{
		ackreply |= (SWDGPIOBBD_cycleRead() << i);
		pr_info("%d", (ackreply & (1 << i)));
	}
	pr_info("\n");
	*ack = ackreply;
}

void SWDGPIOBBD_receiveData( uint32_t *data )
{
	uint32_t	bitRead;
	uint32_t datareply = 0;
	uint32_t parityCount = 0;

	for (int i = 31; i >= 0; i--)
	{
		bitRead = SWDGPIOBBD_cycleRead();
		if( bitRead == 1 )
		{
			datareply |= (bitRead << i);
			parityCount++;
		}
	}
	*data = datareply;
}




//these should be correct:
//https://developer.arm.com/documentation/dui0499/d/ARM-DSTREAM-Target-Interface-Connections/SWD-timing-requirements
uint8_t SWDGPIOBBD_cycleRead(void)
{
	//set data pin
	//SWDPI_gpio_interface.unsetPin( dataPin );	//just in case - should be unset anyways
	//unsset clock pin
	SWDPI_gpio_interface.unsetPin( clockPin );
	//read data pin
	udelay(half_period_us);

	uint8_t value = SWDPI_gpio_interface.readPin( dataPin );
	//set clock pin
	SWDPI_gpio_interface.setPin( clockPin );
	//udelay(1);	//to be safe???	now its too late
	value += SWDPI_gpio_interface.readPin( dataPin );
	//read pin


	udelay(half_period_us);

	return (value == 2);
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
	SWDPI_gpio_interface.unsetPin( clockPin );    //config 2 read:
	SWDPI_gpio_interface.unsetPin( dataPin );		//do not drive
	SWDPI_gpio_interface.configPinPull( dataPin, GPIO_PULL_DOWN );     //not sure what is correct here
	SWDPI_gpio_interface.setPinInput( dataPin );

	//none---> reply is full height, DOWN---> reply is half.
	udelay(half_period_us);
	//unset data pin
	//set clock pin
	SWDPI_gpio_interface.setPin( clockPin );
	udelay(half_period_us);
	SWDPI_gpio_interface.configPinPull( dataPin, GPIO_PULL_NONE );
}

void SWDGPIOBBD_cycleTurnaround2Write(void)
{
	//unset clock pin
	SWDPI_gpio_interface.unsetPin( clockPin );    //config 2 read:

	//unset data pin
    SWDPI_gpio_interface.unsetPin( dataPin );
	SWDPI_gpio_interface.setPinOutput( dataPin );
    SWDPI_gpio_interface.configPinPull( dataPin, GPIO_PULL_DOWN );     //not sure what is correct here
	udelay(half_period_us);

	//set clock pin
	SWDPI_gpio_interface.setPin( clockPin );
	udelay(half_period_us);
}


static int __init SWDGPIOBBD_init(void)
{
		pr_info("Installed SWDGPIOBBD :)\n");		//pr_emerg(), pr_alert(), pr_crit(), pr_err(), pr_warn(), pr_notice(), pr_info(), pr_debug()

		//allocate major and minor number(s) for the device
		alloc_chrdev_region( &SWDGPIOBBD_dev_id, 0, 1, "SWDPI" );
		pr_info( "Major = %d Minor = %d \n", MAJOR( SWDGPIOBBD_dev_id ), MINOR( SWDGPIOBBD_dev_id ) );

		//generate an entry in /proc that one can read from (and potentially write to --> this is connected to the custom_read() function, which is supposed to mimick a file read...)
		SWDGPIOBBD_proc_dir_entry = proc_create("SWDPI", 0666, NULL, &SWDGPIOBBD_pops );

		cdev_init( &SWDGPIOBBD_cdev, &SWDGPIOBBD_fops );
		cdev_add( &SWDGPIOBBD_cdev, SWDGPIOBBD_dev_id, 1 );

		//create class of devices
		SWDGPIOBBD_dev_class = class_create( "SWDPI_class" );
		//create device in /dev
		device_create( SWDGPIOBBD_dev_class, NULL, SWDGPIOBBD_dev_id, NULL, "SWDPI" );



		//access correct function for each platform:
		if( of_machine_is_compatible( "brcm,bcm2712" ) > 0 )
		{
			pr_info("is bcm2712 \n");
			initRaspi5();
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
		}
		else if( of_machine_is_compatible( "brcm,bcm2711" ) > 0 )
		{
			pr_info("is bcm2711 \n");
			initRaspi4();
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
		}
		else if( of_machine_is_compatible( "brcm,bcm2837" ) > 0 )
		{
			pr_info("is bcm2837 \n");
		}
		else if( of_machine_is_compatible( "brcm,bcm2836" ) > 0 )
		{
			pr_info("is bcm2836 \n");
		}
		else if( of_machine_is_compatible( "brcm,bcm2835" ) > 0 )
		{
			pr_info("is bcm2835 \n");
		}




		return 0;
}
static void __exit SWDGPIOBBD_exit(void)
{
		pr_info( "Uninstalled SWDGPIOBBD :(\n" );

		device_destroy( SWDGPIOBBD_dev_class, SWDGPIOBBD_dev_id );
		class_destroy( SWDGPIOBBD_dev_class );

		cdev_del(&SWDGPIOBBD_cdev);

		remove_proc_entry( "SWDPI", NULL );

		//dealloc minor&major numbers
		unregister_chrdev_region( SWDGPIOBBD_dev_id, 1 );
}

module_init( SWDGPIOBBD_init );
module_exit( SWDGPIOBBD_exit );
