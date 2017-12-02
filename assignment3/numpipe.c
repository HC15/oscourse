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

static unsigned int used_by;
static int numpipe_open(struct inode *, struct file *);
static int numpipe_close(struct inode *, struct file *);
static ssize_t numpipe_read(struct file *, char __user *, size_t, loff_t *);
static ssize_t numpipe_write(struct file *, const char __user *, size_t, loff_t *);

static int pipe_size = 1;
module_param(pipe_size, int, S_IRUGO);

static int *pipe_buffer;
static unsigned int pipe_elements;

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
	int index;

	printk(KERN_INFO "numpipe module init\n");
	device_number = misc_register(&numpipe_device);
	if(device_number < 0) {
		printk(KERN_ERR "numpipe module init failed\n");
		return device_number;
	}
	used_by = 0;

	pipe_buffer = (int*) kmalloc(pipe_size * sizeof(int), GFP_KERNEL);
	for(index = 0; index < pipe_size; index++) {
		pipe_buffer[index] = '\0';;
		printk(KERN_INFO "%d", pipe_buffer[index]);
	}
	pipe_elements = 0;

	sema_init(&mutex, 1);
	sema_init(&empty, pipe_size);
	sema_init(&full, 0);

	printk(KERN_INFO "numpipe module init success\n");
	return 0;
}

static void __exit numpipe_exit(void) {
	if(used_by == 0) {
		misc_deregister(&numpipe_device);
		kfree(pipe_buffer);
		printk(KERN_INFO "numpipe module exit\n");
	}
	else {
		printk(KERN_INFO "numpipe module exit failed, device still in use");
	}
}

static int numpipe_open(struct inode *inode, struct file *file) {
	used_by++;
	printk(KERN_INFO "numpipe opened, currently used by %d devices\n", used_by);
	return 0;
}

static int numpipe_close(struct inode *inode, struct file *file) {
	if(used_by != 0) {
		used_by--;
		printk(KERN_INFO "numpipe closed, currently used by %d devices\n", used_by);
	}
	else {
		printk(KERN_INFO "numpipe isn't be used, so can't be closed");
	}
	return 0;
}

static ssize_t numpipe_read(struct file *file, char __user *out, size_t size, loff_t *off) {
	unsigned long bytes_read = sizeof(int);

	int index;
	for(index = 0; index < pipe_size; index++) {
		printk(KERN_INFO "%d ", pipe_buffer[index]);
	}
	printk(KERN_INFO "\n");

	if(access_ok(VERIFY_WRITE, out, bytes_read)) {
		down_interruptible(&full);
		down_interruptible(&mutex);

		copy_to_user(out, &pipe_buffer[0], bytes_read);
		for(index = 0; index < pipe_size - 1; index++) {
			pipe_buffer[index] = pipe_buffer[index + 1];
		}
		pipe_buffer[pipe_size - 1] = '\0';
		pipe_elements--;
		
		up(&mutex);
		up(&empty);
		return bytes_read;
	}
	else {
		return -EFAULT;
	}
}

static ssize_t numpipe_write(struct file *file, const char __user *out, size_t size, loff_t *off) {
	unsigned long bytes_written = sizeof(int);
	int index;
	for(index = 0; index < pipe_size; index++) {
		printk(KERN_INFO "%d ", pipe_buffer[index]);
	}
	printk(KERN_INFO "\n");

	if(access_ok(VERIFY_READ, out, bytes_written)) {
		down_interruptible(&empty);
		down_interruptible(&mutex);

		copy_from_user(&pipe_buffer[pipe_elements], out, bytes_written);
		pipe_elements++;
		
		up(&mutex);
		up(&full);
		return bytes_written;
	}
	else {
		return -EFAULT;
	}
}

module_init(numpipe_init);
module_exit(numpipe_exit);
