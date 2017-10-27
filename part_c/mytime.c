#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/time.h>
#include <asm/uaccess.h>

MODULE_LICENSE("DUAL BSD/GPL");
MODULE_AUTHOR("Harvey Chen");

#define NAME "mytime"
static int deviceNumber;

static int mytime_open(struct inode *, struct file *);
static ssize_t mytime_read(struct file *file, char __user * out, size_t size, loff_t * off);

static struct file_operations my_fops = {
	.open = mytime_open,
	.read = mytime_read
};

static int __init mytime_init(void) {
	printk(KERN_ALERT "mytime module init\n");
	deviceNumber = register_chrdev(0, NAME, &my_fops);
	return 0;
}

static int mytime_open(struct inode *inode, struct file *file) {
	printk(KERN_ALERT "mytime device opened");
	return 0;
}

static ssize_t mytime_read(struct file *file, char __user * out, size_t size, loff_t * off) {
	if(access_ok(VERIFY_WRITE, out, size)) {
		char* buffer = (char*) kmalloc(sizeof(char) * 75, GFP_KERNEL);

		int success;
		success = sprintf(buffer, "current_kernel_time: ");

		struct timespec kernel_time = current_kernel_time();
		int length;

		length = snprintf(NULL, 0, "%ld", kernel_time.tv_nsec);
		char* kernel_time_nsec = (char*) kmalloc(length + 1, GFP_KERNEL);
		success += sprintf(buffer + success, kernel_time_nsec);
		kfree(kernel_time_nsec);

		length = snprintf(NULL, 0, "%ld", kernel_time.tv_sec);
		char* kernel_time_sec = (char*) kmalloc(length + 1, GFP_KERNEL);
		success += sprintf(buffer + success, kernel_time_sec);
		kfree(kernel_time_sec);


		struct timespec timeofday;
		getnstimeofday(&timeofday);

		printk(KERN_DEFAULT "current_kernel_time: %ld %ld \ngetnstimeofday: %ld %ld", kernel_time.tv_sec, kernel_time.tv_nsec, timeofday.tv_sec, timeofday.tv_nsec);
		copy_to_user(out, &buffer, size);
		return 1;
	}
	else{
		return -EFAULT;
	}
}

static void __exit mytime_exit(void) {
	unregister_chrdev(deviceNumber, NAME);
	printk(KERN_ALERT "mytime module exit\n");
}

module_init(mytime_init);
module_exit(mytime_exit);
