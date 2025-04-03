#include <linux/init.h>		//used for __init
#include <linux/module.h>
#include <linux/kernel.h>

#include <linux/uaccess.h>
#include <linux/fs.h>		//file operations
#include <linux/proc_fs.h>

#include <linux/device.h>	//for /dev/
#include <linux/cdev.h>
#include <linux/kdev_t.h>

MODULE_DESCRIPTION("cheesecake GPIO bitbang");
MODULE_AUTHOR("dw42");
MODULE_LICENSE("GPL");
MODULE_VERSION("0:1.0");

int testParam_callback( const char *val, const struct kernel_param *kp );
static ssize_t myProcRead( struct file* file, char __user* user_buffer, size_t count, loff_t* offset );

int myTestParam = 0;

dev_t myDev = 0;
static struct class *dev_class;
//character device in /dev:
static struct cdev cheesecake_cdev;

// PROCESS PROCESS PROCESS
// define what is in /proc/...
static struct proc_dir_entry* my_proc_dir_entry;
static const struct proc_ops pops =		//replace struct file_operations with struct proc_ops for kernel version 5.6 or later
{
	//.owner = THIS_MODULE,
	.proc_read = myProcRead		//what happens when you read /proc/cheesecakedrivercd
};

// DEVICE DEVICE DEVICE
//define what happens with /dev/...
static int      cheesecake_open(struct inode *inode, struct file *file);
static int      cheesecake_release(struct inode *inode, struct file *file);
static ssize_t  cheesecake_read(struct file *filp, char __user *buf, size_t len,loff_t * off);
static ssize_t  cheesecake_write(struct file *filp, const char *buf, size_t len, loff_t * off);

static long cheesecake_ioctl( struct file *file, unsigned int cmd, unsigned long arg );


static struct file_operations fops =
{
.owner          = THIS_MODULE,
.read           = cheesecake_read,
.write          = cheesecake_write,
.open           = cheesecake_open,
.unlocked_ioctl = cheesecake_ioctl,
.release        = cheesecake_release,
};

// PARAMETER PARAMETER PARAMETER
//define what is happening in /sys/module/.../parameters
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

static ssize_t myProcRead( struct file* file, char __user* user_buffer, size_t count, loff_t* offset )		// called via: cat /proc/cheesePI
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



static int cheesecake_open(struct inode *inode, struct file *file)
{
	pr_info("Driver Open Function Called...!!!\n");
	return 0;
}

static int cheesecake_release(struct inode *inode, struct file *file)
{
	pr_info("Driver Release Function Called...!!!\n");
	return 0;
}

static ssize_t cheesecake_read(struct file *filp, char __user *buf, size_t len,loff_t * off)
{
	pr_info("Driver Read Function Called...!!!\n");
	return 0;
}

static ssize_t cheesecake_write(struct file *filp, const char *buf, size_t len, loff_t * off)
{
	pr_info("Driver Write Function Called...!!!\n");
	return len;
}

static long cheesecake_ioctl( struct file *file, unsigned int cmd, unsigned long arg )
{
	return 0;
}


static int __init cheesecake_init(void)
{
		pr_info("Installed Cheesecake :)\n");		//pr_emerg(), pr_alert(), pr_crit(), pr_err(), pr_warn(), pr_notice(), pr_info(), pr_debug()

		//allocate major and minor number(s) for the device
		alloc_chrdev_region( &myDev, 0, 1, "cheesePI" );
		pr_info( "Major = %d Minor = %d \n", MAJOR( myDev ), MINOR( myDev ) );

		//generate an entry in /proc that one can read from (and potentially write to --> this is connected to the custom_read() function, which is supposed to mimick a file read...)
		my_proc_dir_entry = proc_create("cheesePI", 0666, NULL, &pops );

		cdev_init( &cheesecake_cdev, &fops );
		cdev_add( &cheesecake_cdev, myDev, 1 );

		//create class of devices
		dev_class = class_create( "cheesecake_class" );
		//create device in /dev
		device_create( dev_class, NULL, myDev, NULL, "cheesePI_device" );

		return 0;
}
static void __exit cheesecake_exit(void)
{
		pr_info( "Uninstalled Cheesecake :(\n" );

		device_destroy( dev_class, myDev );
		class_destroy( dev_class );

		cdev_del(&cheesecake_cdev);

		remove_proc_entry( "cheesePI", NULL );

		//dealloc minor&major numbers
		unregister_chrdev_region( myDev, 1 );
}

module_init( cheesecake_init );
module_exit( cheesecake_exit );
