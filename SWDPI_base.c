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

#include <linux/spinlock.h>

//#include <linux/flex_array.h>	//allocate non-continuous arrays...
//0x1 0x2ba01477
//#include <linux/gpio/driver.h>
#include "SWDPI_base.h"
#include "SWDPI_SWD.h"
#include "SWDPI.h"	//public header

MODULE_DESCRIPTION("SWD GPIO bitbang driver: SWDGPIOBBD");
MODULE_AUTHOR("dw42");
MODULE_LICENSE("GPL");
MODULE_VERSION("0:1.0");

//https://qcentlabs.com/posts/swd_banger/
// clock idles high. --> corrected to LOW for now
// bit is read at the beginning of the low-high clock cycle (at the initial falling clock)
// bit it written also at the beginning o fthe clock cycle, so that it has defined value at the rising clock in the middle of the cycle.
// pins can always pull down or none... it does not seem to matter... leave at none for now...\

/* regarding timing - useful comments from chatGPT:
To minimize interruptions and avoid long stall times in a kernel module on a Raspberry Pi, consider the following strategies:
1. Real-Time Scheduling: Use real-time scheduling policies like SCHED_FIFO or SCHED_RR to give your kernel module higher priority over other processes. This can reduce the likelihood of being preempted by other tasks.
2. Interrupt Handling: Ensure that your module efficiently handles interrupts. Use interrupt handlers to manage time-critical tasks and defer less critical work to bottom halves or work queues.
3. Preemption and Latency: Enable kernel preemption and configure low-latency settings if they are available in your kernel configuration. This can help reduce the time your module is stalled by other kernel activities.
4. Isolate CPUs: If your application allows, you can isolate one or more CPUs for your real-time tasks using CPU affinity. This can prevent other processes from running on those CPUs.
5. Optimize Code: Ensure that your kernel module code is optimized for performance. Avoid long-running loops and minimize the use of blocking calls.
6. Use High-Resolution Timers: If your kernel supports it, use high-resolution timers (hrtimer) for more precise timing control.
*/

// start a thread --> SCHED_FIFO

/* struct task_struct *task;
task = kthread_create(your_thread_function, data, "your_thread_name");
if (!IS_ERR(task)) {
    kthread_bind(task, 0); // Bind to CPU 0
    wake_up_process(task);
}
*/


uint8_t SWDGPIOBBD_lock;

uint8_t SWDGPIOBBD_readwritelock;

uint64_t *sendBuffer;
uint32_t sendBuffer_size = 64;
uint64_t *receiveBuffer;
uint32_t receiveBuffer_size = 64;

uint32_t receiveBuffer_level = 0;
uint32_t receiveBuffer_levelRead = 0;


//device ID (major/minor)
dev_t SWDGPIOBBD_dev_id = 0;
//device class
static struct class *SWDGPIOBBD_dev_class;
//character device in /dev:
static struct cdev SWDGPIOBBD_cdev;




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
		pr_warn("SWDGPIOBBD_open() failure: already locked \n");
		return -1;
	}

	//correct hardware has been selected already, when kernel module was loaded
	SWDPI_gpio_interface.init();

	sendBuffer = (uint64_t*)kcalloc(sendBuffer_size, sizeof(uint64_t), GFP_KERNEL);
	receiveBuffer = (uint64_t*)kcalloc(receiveBuffer_size, sizeof(uint64_t), GFP_KERNEL);
	if( sendBuffer == NULL || receiveBuffer == NULL )
	{
		/* abort */
		pr_warn("SWDGPIOBBD_open() failure: kcalloc() failed \n");
		SWDPI_gpio_interface.cleanup();
		SWDGPIOBBD_lock = 0;
		if( sendBuffer != NULL )
		{
			kfree( sendBuffer );
		}
		if( receiveBuffer != NULL )
		{
			kfree( receiveBuffer );
		}
		SWDGPIOBBD_readwritelock = 0;
		return -1;
	}

	receiveBuffer_level = 0;
	receiveBuffer_levelRead = 0;
	SWDGPIOBBD_readwritelock = 0;


	return 0;
}

//need to write exactly 64bits (8bytes) or multiples...
static ssize_t SWDGPIOBBD_write(struct file *filp, const char *buf, size_t len, loff_t *off)
{
	pr_info("\n *** Driver Write Function Called...!!!\n");
	pr_info("len: %d\n", len);
	pr_info("off: %d\n", *off);
	if( SWDGPIOBBD_readwritelock == 1 )
	{
		pr_warn("write fail - currently in read action \n");
		return -1;
	}

	if( (len % 8) != 0 ) // only accept single command or list of commands (multiples of 64 bits (8 bytes))
	{
		pr_warn("write fail - wrong size \n");
		return SWDPI_ERR_WRONG_RW_SIZE;
	}

	if( receiveBuffer_level != 0 ) //can only write if receive buffer is empty
	{
		pr_warn("write fail - receive buffer not empty: %d \n", receiveBuffer_level );
		return SWDPI_ERR_RECBUF_NOTEMPTY;
	}

	//1. copy to temp buffer
	//2. check command
	//3. add to main buffer
	//4. execute... only reply buffer is empty, we receive new commands...
	int count = len / 8;
	int realCount = 0;
	uint64_t tempBuffer = 0;
	//uint32_t *tempBuffer32 = (uint32_t*)&tempBuffer;

	if( count > sendBuffer_size )
	{
		pr_warn("write fail - buffer full \n");
		return -1;
	}

	for (size_t i = 0; i < count; i++)
	{
		//tempBuffer = *((uint64_t*)buf);
		copy_from_user(&tempBuffer, &buf[8*i], 8);	//copy 64bits per loop

		if( tempBuffer == 0 )
		{
			//reset!
			pr_warn("tempBuffer zero error.\n");
			//return -1;
		}
		else
		{
			//add to buffer:
			sendBuffer[i] = tempBuffer;
			realCount++;
		}

		/* at least check if Parity is correct?
		if( checkCmd( tempBuffer ) < 0 )
		{
			empty tempBuffer!
			pr_warn("write fail - command pre-check fault \n");
			return -1;
		}
		*/
	}

	count = realCount;

	pr_info("write %d commands \n", count );

	//start actually sending (whole sequence with one jtag2swd first):
	int reply = 0;

	SWDGPIOBBD_sequence( swd_sequence_jtag2swd, swd_sequence_jtag2swd_length );

	for( int i = 0; i < count; i++ )
	{
		//pr_info("send buffer before transfer: 0x%x data: 0x%x\n", ((uint32_t*)sendBuffer)[2*i], ((uint32_t*)sendBuffer)[2*i+1]);
		reply = SWDGPIOBBD_transfer( &sendBuffer[i] );
		pr_info("send buffer after transfer: 0x%x data: 0x%x\n", ((uint32_t*)sendBuffer)[2*i], ((uint32_t*)sendBuffer)[2*i+1]);
		if( reply < 0 )
		{
			pr_info("error %d aborting \n\n", reply);

			receiveBuffer[receiveBuffer_level] = sendBuffer[i];		//so that we get the error back
			receiveBuffer_level++;

			SWDGPIOBBD_abort();

			return receiveBuffer_level;	//abort
		}
		//pr_info("\n");
		receiveBuffer[receiveBuffer_level] = sendBuffer[i];
		receiveBuffer_level++;
	}

	return receiveBuffer_level;
}

//needs to return number of bytes or 0 for EOF
static ssize_t SWDGPIOBBD_read(struct file *filp, char __user *buf, size_t len, loff_t * off)		//only returns ack and IDCODE value
{
	pr_info("\n *** Driver Read Function Called...!!!\n");
	// e.g. copy_to_user(buf, kernel_buffer, mem_size);
	pr_info("receiveBuffer_level: %d\n", receiveBuffer_level );
	pr_info("receiveBuffer_levelRead: %d\n", receiveBuffer_levelRead );
	pr_info("len: %d\n", len );

	SWDGPIOBBD_readwritelock = 1;

	if ( (len % 8) != 0 ) // only accept full command or list of full commands
	{
		pr_warn("read fail - wrong size \n");
		return SWDPI_ERR_WRONG_RW_SIZE;
	}

	int count = len / 8;
	pr_info("count: %d\n", count );

	if( receiveBuffer_level == 0 )
	{
		//nothing to read
		pr_warn("receiveBuffer_level is zero! \n");
		return -1;
	}

	if( receiveBuffer_level == receiveBuffer_levelRead )		//all has been read but somehow levels have not been reset to zero
	{
		receiveBuffer_level = 0;
		receiveBuffer_levelRead = 0;
		pr_warn("receiveBuffer_level is receiveBuffer_levelRead! \n");
		return -1;
	}

	if( count > (receiveBuffer_level - receiveBuffer_levelRead ))		//want to read more than is available
	{
		pr_warn("trying to read more than available \n");
		count = (receiveBuffer_level - receiveBuffer_levelRead );	//only returns available data
		//return -1;
	}


	//rest if receive buffer is empty
	if( receiveBuffer_level == receiveBuffer_levelRead )
	{
		receiveBuffer_level = 0;
		receiveBuffer_levelRead = 0;
		pr_info("reset levels to zero. \n" );
	}



	copy_to_user(buf, receiveBuffer, len);

	receiveBuffer_levelRead += len/8;	//we count in 64bit words, but len is in 8bit bytes

	//rest if receive buffer is empty
	if( receiveBuffer_level == receiveBuffer_levelRead )
	{
		receiveBuffer_level = 0;
		receiveBuffer_levelRead = 0;
		pr_info("reset levels to zero. \n" );
	}

	SWDGPIOBBD_readwritelock = 0;
	return receiveBuffer_level - receiveBuffer_levelRead;
}


/*	uint32_t	kernel_buffer[2] = {0x0, };
	uint32_t 	mem_size = 8;

	//1. >= 50 cycles high:

	uint32_t	data = 0;
	uint8_t		ack = 0;

	//SWDGPIOBBD_print_command( SWD_CMD_READ_IDCODE );

	//1. connection sequence
	SWDGPIOBBD_sequence( swd_sequence_jtag2swd, swd_sequence_jtag2swd_length );

	//2. get IDCODE
	SWDGPIOBBD_command( SWD_CMD_READ_IDCODE );
	SWDGPIOBBD_cycleTurnaround2Read();
	SWDGPIOBBD_receiveAck( &ack );
	SWDGPIOBBD_receiveData( &data );
	SWDGPIOBBD_cycleTurnaround2Write();

	kernel_buffer[0] = ack;
	kernel_buffer[1] = data;

	//copy_to_user(buf, &kernel_buffer, mem_size);

	copy_to_user(buf, &kernel_buffer, mem_size);*/


/* working sequence:
//use spinlock to not get interrupted during the transfer
//does not seem to be necessary:
//static spinlock_t lock;
//unsigned long flags;
//spin_lock_init(&lock);
//spin_lock_irqsave( &lock, flags);

//1. connection sequence
SWDGPIOBBD_sequence( swd_sequence_jtag2swd, swd_sequence_jtag2swd_length );

//2. get IDCODE
SWDGPIOBBD_command( SWD_CMD_READ_IDCODE );
SWDGPIOBBD_cycleTurnaround2Read();
SWDGPIOBBD_receiveAck( &ack );
SWDGPIOBBD_receiveData( &data );
SWDGPIOBBD_cycleTurnaround2Write();

//does not seem to be necessary:
//spin_unlock_irqrestore( &lock, flags);
*/

static int SWDGPIOBBD_release(struct inode *inode, struct file *file)
{
	pr_info("SWDGPIOBBD_release()\n");
	SWDGPIOBBD_lock = 0;
	SWDPI_gpio_interface.cleanup();

	if( sendBuffer != NULL )
	{
		kfree( sendBuffer );
	}
	if( receiveBuffer != NULL )
	{
		kfree( receiveBuffer );
	}

	return 0;
}


//maybe not needed for now....
static long SWDGPIOBBD_ioctl( struct file *file, unsigned int cmd, unsigned long arg )
{
	return 0;
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

		int result = 0;
		//access correct function for each platform:
		if( of_machine_is_compatible( "brcm,bcm2712" ) > 0 )
		{
			pr_info("is bcm2712 \n");
			result = SWDGPIOBBD_initRaspi( 5 );
		}
		else if( of_machine_is_compatible( "brcm,bcm2711" ) > 0 )
		{
			pr_info("is bcm2711 \n");
			result = SWDGPIOBBD_initRaspi( 4 );
		}
		else if( of_machine_is_compatible( "brcm,bcm2837" ) > 0 )
		{
			pr_info("is bcm2837 \n");
			result = SWDGPIOBBD_initRaspi( 3 );
		}
		else if( of_machine_is_compatible( "brcm,bcm2836" ) > 0 )
		{
			pr_info("is bcm2836 \n");
			result = SWDGPIOBBD_initRaspi( 2 );
		}
		else if( of_machine_is_compatible( "brcm,bcm2835" ) > 0 )
		{
			pr_info("is bcm2835 \n");
			result = SWDGPIOBBD_initRaspi( 1 );
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


// PARAMETER PARAMETER PARAMETER  --  we do not need this here I think
//define what is happening in /sys/module/.../parameters
//param to be set in sysfs
/*int myTestParam = 0;
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
}*/



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

	//could return current status...

	copy_to_user( user_buffer, greeting, length_of_greeting );
	*offset = length_of_greeting;

	return length_of_greeting;
}


module_init( SWDGPIOBBD_init );
module_exit( SWDGPIOBBD_exit );
