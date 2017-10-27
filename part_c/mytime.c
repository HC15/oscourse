#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/time.h>
#include <asm/uaccess.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Harvey Chen");

#define NAME "mytime"
static int deviceNumber;

static int mytime_open(struct inode *, struct file *);
static int mytime_close(struct inode *, struct file *);
static ssize_t mytime_read(struct file *, char __user *, size_t, loff_t *);

static struct file_operations my_fops = {
	.owner = THIS_MODULE,
	.open = mytime_open,
	.release = mytime_close,
	.read = mytime_read
};

static int __init mytime_init(void) {
	printk(KERN_ALERT "mytime module init\n");
	deviceNumber = register_chrdev(0, NAME, &my_fops);
	if(deviceNumber < 0) {
		printk(KERN_ALERT "mytime module init failed\n");
		return deviceNumber;
	}
	printk(KERN_ALERT "mytime module init success\n");
	return 0;
}

static int mytime_open(struct inode *inode, struct file *file) {
	printk(KERN_ALERT "mytime device opened\n");
	return 0;
}

static int mytime_close(struct inode *inode, struct file *file) {
	printk(KERN_ALERT "mytime device closed\n");
	return 0;
}

static ssize_t mytime_read(struct file *file, char __user * out, size_t size, loff_t * off) {
	if(access_ok(VERIFY_WRITE, out, size)) {
		char* buffer = (char*) kmalloc(sizeof(char) * size, GFP_KERNEL);

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

		success += sprintf(buffer + success, "\n");

		struct timespec timeofday;
		getnstimeofday(&timeofday);

		length = snprintf(NULL, 0, "%ld", timeofday.tv_nsec);
		char* timeofday_nsec = (char*) kmalloc(length + 1, GFP_KERNEL);
		success += sprintf(buffer + success, timeofday_nsec);
		kfree(timeofday_nsec);

		length = snprintf(NULL, 0, "%ld", timeofday.tv_sec);
		char* timeofday_sec = (char*) kmalloc(length + 1, GFP_KERNEL);
		success += sprintf(buffer + success, timeofday_sec);
		kfree(timeofday_sec);

		success += sprintf(buffer + success, "\n");

		printk(KERN_DEFAULT "current_kernel_time: %ld %ld \ngetnstimeofday: %ld %ld", kernel_time.tv_sec, kernel_time.tv_nsec, timeofday.tv_sec, timeofday.tv_nsec);

		copy_to_user(out, &buffer, size);
		return success;
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

