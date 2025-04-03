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

#include <linux/delay.h>

#include "SWDPI.h"	//public header

#define RASPI_5_GPIO_BASE  0x0000001f000d0000
#define RASPI_5_PADS_BASE  0x0000001f000f0000

#define RASPI_4_GPIO_BASE  0x0


MODULE_DESCRIPTION("SWD GPIO bitbang driver: SWDGPIOBBD");
MODULE_AUTHOR("dw42");
MODULE_LICENSE("GPL");
MODULE_VERSION("0:1.0");


void SWDGPIOBBD_connectionSequence(void);
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
uint8_t clockPinState;
uint8_t dataPin;
uint8_t dataPinState;

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



// /proc/read
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

	struct device_node_ptr *soc_node;
	soc_node = of_find_node_by_name(struct device_node *from, const char *name)



	//set some efault settings
	//need 2 default pins (gpio5 and gpio6) for clock and reserve some memory for transfers?

	clockPin = 5;
	clockPinState = 0;
	dataPin = 6;
	dataPinState = 0;


	speed_hz = 100000;	//100 kHz
	period_us = 1000000/speed_hz;
	half_period_us = period_us/2;

	//need 8 bits for command, 8 bits for respones and 32 bits (multiple?) for data.




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

	//// simply reads the idcode register
	//command: SWD_CMD_READ_IDCODE



	uint32_t	kernel_buffer = 0x0;
	uint32_t 	mem_size = 4;

	//1. >= 50 cycles high:
	SWDGPIOBBD_connectionSequence();

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

void SWDGPIOBBD_connectionSequence(void)
{
	for( size_t i = 0; i < 60; i++ )
	{
		SWDGPIOBBD_cycleWrite( 1 );
	}
}

uint8_t SWDGPIOBBD_cycleRead(void)
{
	return 0;
}

void SWDGPIOBBD_cycleWrite( uint8_t bit )
{
	//set clock pin
	//set data pin
	udelay(half_period_us);
	//unset clock pin
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
