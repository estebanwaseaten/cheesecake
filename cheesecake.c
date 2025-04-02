#include <linux/init.h>		//used for __init
#include <linux/module.h>
#include <linux/kernel.h>

#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>

MODULE_DESCRIPTION("cheesecake GPIO bitbang");
MODULE_AUTHOR("dw42");
MODULE_LICENSE("GPL");
MODULE_VERSION("0:1.0");

int notify_callback( const char *val, const struct kernel_param *kp );
static ssize_t custom_read( struct file* file, char __user* user_buffer, size_t count, loff_t* offset );

int cb_valueETX = 0;

static struct proc_dir_entry* my_proc_dir_entry;
static const struct proc_ops pops =		//replace struct file_operations with struct proc_ops for kernel version 5.6 or later
{
	//.owner = THIS_MODULE,
	.proc_read = custom_read		//what happens when you read /proc/cheesecakedrivercd
};

const struct kernel_param_ops my_param_ops =
{
        .set = &notify_callback, // Use our setter ...
        .get = &param_get_int, // .. and standard getter
};

module_param_cb(cb_valueETX, &my_param_ops, &cb_valueETX, S_IRUGO|S_IWUSR );

int notify_callback( const char *val, const struct kernel_param *kp )
{
	int res = param_set_int(val, kp);
	if( res == 0 )
	{
			pr_info("callback called!");
			return 0;
	}
	return -1;
}

static ssize_t custom_read( struct file* file, char __user* user_buffer, size_t count, loff_t* offset )
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



static int __init cheesecake_init(void)
{
		pr_info("Installed Cheesecake :)\n");		//pr_emerg(), pr_alert(), pr_crit(), pr_err(), pr_warn(), pr_notice(), pr_info(), pr_debug()
		my_proc_dir_entry = proc_create("cheesePI", 0666, NULL, &pops );



		return 0;
}
static void __init cheesecake_exit(void)
{
		remove_proc_entry( "cheesePI", NULL );
		pr_info("Uninstalled Cheesecake :(\n");
}
module_init(cheesecake_init);
module_exit(cheesecake_exit);
