#include <linux/init.h>
#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/time.h>
#include <asm/uaccess.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Harvey Chen");

static int mytime_open(struct inode *, struct file *);
static int mytime_close(struct inode *, struct file *);
static ssize_t mytime_read(struct file *, char __user *, size_t, loff_t *);

static struct file_operations my_fops = {
	.owner = THIS_MODULE,
	.open = mytime_open,
	.release = mytime_close,
	.read = mytime_read
};

static struct miscdevice mytime_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name  = "mytime",
	.fops = &my_fops

};

static int __init mytime_init(void) {
	printk(KERN_ALERT "mytime module init\n");
	int deviceNumber = misc_register(&mytime_device);
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

		struct timespec kernel_time = current_kernel_time();
		struct timespec timeofday;
		getnstimeofday(&timeofday);

		printk(KERN_ALERT "current_kernel_time: %ld %ld \ngetnstimeofday: %ld %ld", kernel_time.tv_sec, kernel_time.tv_nsec, timeofday.tv_sec, timeofday.tv_nsec);
		int bytes_read;
		bytes_read = sprintf(buffer, "current_kernel_time: %ld %ld \ngetnstimeofday: %ld %ld", kernel_time.tv_sec, kernel_time.tv_nsec, timeofday.tv_sec, timeofday.tv_nsec);
		copy_to_user(out, &buffer, size);
		return bytes_read;
	}
	else{
		return -EFAULT;
	}
}

static void __exit mytime_exit(void) {
	misc_deregister(&mytime_device);
//	unregister_chrdev(deviceNumber, NAME);
	printk(KERN_ALERT "mytime module exit\n");
}

module_init(mytime_init);
module_exit(mytime_exit);

