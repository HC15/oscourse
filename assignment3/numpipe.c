#include <linux/init.h>
#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/semaphore.h>
#include <asm/uaccess.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Harvey Chen");

static int numpipe_open(struct inode *, struct file *);
static int numpipe_close(struct inode *, struct file *);
static ssize_t numpipe_read(struct file *, char __user *, size_t, loff_t *);
static ssize_t numpipe_write(struct file *, const char __user *, size_t, loff_t *);

static int pipe_size = 1;
module_param(pipe_size, int, S_IRUGO);

char** pipe_buffer;

static struct semaphore mutex;
static struct semaphore empty;
static struct semaphore full;

static struct file_operations my_fops = {
	.owner = THIS_MODULE,
	.open = numpipe_open,
	.release = numpipe_close,
	.read = numpipe_read,
	.write = numpipe_write
};

static struct miscdevice numpipe_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name  = "numpipe",
	.fops = &my_fops
};

static int __init numpipe_init(void) {
	int device_number;
	int pipe_index;
	printk(KERN_INFO "numpipe module init\n");
	device_number = misc_register(&numpipe_device);
	if(device_number < 0) {
		printk(KERN_ERR "numpipe module init failed\n");
		return device_number;
	}

	pipe_buffer = (char**) kmalloc(pipe_size * sizeof(char*), GFP_KERNEL);
	for(pipe_index = 0; pipe_index < pipe_size; pipe_index++) {
		pipe_buffer[pipe_index] = (char*) kmalloc(40 * sizeof(char), GFP_KERNEL);
		pipe_buffer[pipe_index] = '\0';
		printk(KERN_INFO "%s", pipe_buffer[pipe_index]);
	}

	sema_init(&mutex, 1);
	sema_init(&empty, 0);
	sema_init(&full, pipe_size);



	printk(KERN_INFO "numpipe module init success\n");
	return 0;
}

static int numpipe_open(struct inode *inode, struct file *file) {
	printk(KERN_INFO "numpipe device opened\n");
	return 0;
}

static int numpipe_close(struct inode *inode, struct file *file) {
	printk(KERN_INFO "numpipe device closed\n");
	return 0;
}

static ssize_t numpipe_read(struct file *file, char __user *out, size_t size, loff_t *off) {
	if(access_ok(VERIFY_WRITE, out, size)) {
		return 0;
	}
	else{
		return -EFAULT;
	}
}

static ssize_t numpipe_write(struct file *file, const char __user *out, size_t size, loff_t *off) {
	return 0;
}

static void __exit numpipe_exit(void) {
	misc_deregister(&numpipe_device);
	printk(KERN_INFO "numpipe module exit\n");
}

module_init(numpipe_init);
module_exit(numpipe_exit);
