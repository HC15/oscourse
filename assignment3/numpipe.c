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

//char** pipe_buffer;
static int *pipe_buffer;
static unsigned int pipe_index;
static unsigned int pipe_read;
static unsigned int pipe_write;

static struct semaphore mutex;
static struct semaphore empty;
static struct semaphore full;

static unsigned int used_by;

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
/*
	pipe_buffer = (char**) kmalloc(pipe_size * sizeof(char*), GFP_KERNEL);
	for(index = 0; index < pipe_size; index++) {
		pipe_buffer[index] = (char*) kmalloc(10 * sizeof(char), GFP_KERNEL);
		pipe_buffer[index] = '\0';
		printk(KERN_INFO "%s", pipe_buffer[pipe_index]);
	}
*/
	pipe_buffer = (int*) kmalloc(pipe_size * sizeof(int), GFP_KERNEL);
	for(index = 0; index < pipe_size; index++) {
		pipe_buffer[index] = '\0';;
		printk(KERN_INFO "%d", pipe_buffer[index]);
	}
	pipe_index = 0;
	pipe_read = 0;
	pipe_write = 0;

	sema_init(&mutex, 1);
	sema_init(&empty, pipe_size);
	sema_init(&full, 0);

	used_by = 0;

	printk(KERN_INFO "numpipe module init success\n");
	return 0;
}

static int numpipe_open(struct inode *inode, struct file *file) {
	used_by++;
	printk(KERN_INFO "numpipe device opened\n");
	printk(KERN_INFO "numpipe has %d devices open\n", used_by);
	return 0;
}

static int numpipe_close(struct inode *inode, struct file *file) {
	if(used_by != 0) {
		used_by--;
		printk(KERN_INFO "numpipe device closed\n");
	}
	printk(KERN_INFO "numpipe has %d devices open\n", used_by);
	return 0;
}

static ssize_t numpipe_read(struct file *file, char __user *out, size_t size, loff_t *off) {
/*
	int pipe_index;
	if(access_ok(VERIFY_WRITE, out, sizeof(char) * 40) {
		down_interruptible(&mutex);
		down_interruptible(&full);

		up(&empty);
		up(&mutex);
		return 0;
	}
	else{
		return -EFAULT;
	}
*/
	unsigned long bytes_read = sizeof(int);
//	down_interruptible(&mutex);
	int index;
	for(index = 0; index < pipe_size; index++) {
		printk(KERN_INFO "%d ", pipe_buffer[index]);
	}
	printk(KERN_INFO "\n");

	down_interruptible(&full);
	down_interruptible(&mutex);
//	printk(KERN_INFO "Read before %d\n", pipe_buffer[pipe_read]);
	copy_to_user(out, &pipe_buffer[0], bytes_read);
	for(index = 0; index < pipe_size - 1; index++) {
		pipe_buffer[index] = pipe_buffer[index + 1];
	}
	pipe_buffer[pipe_size - 1] = '\0';
//	printk(KERN_INFO "Read after %d\n", pipe_buffer[pipe_read]);
	pipe_index--;
	up(&mutex);
	up(&empty);
//	up(&mutex);
	return bytes_read;
}

static ssize_t numpipe_write(struct file *file, const char __user *out, size_t size, loff_t *off) {
/*
	if(access_ok(VERIFY_READ, out, size)) {
		return 0;
	}
	else{
		return -EFAULT;
	}
*/
	unsigned long bytes_written = sizeof(int);
	int index;
	for(index = 0; index < pipe_size; index++) {
		printk(KERN_INFO "%d ", pipe_buffer[index]);
	}
	printk(KERN_INFO "\n");

//	down_interruptible(&mutex);
	down_interruptible(&empty);
	down_interruptible(&mutex);
	printk(KERN_INFO "Write before %d\n", pipe_buffer[pipe_write]);
	copy_from_user(&pipe_buffer[pipe_index], out, bytes_written);
	printk(KERN_INFO "Write after %d\n", pipe_buffer[pipe_write]);
	pipe_index++;
	up(&mutex);
	up(&full);
//	up(&mutex);
	return bytes_written;
}

static void __exit numpipe_exit(void) {
//	if(used_by == 0) {
		misc_deregister(&numpipe_device);
		kfree(pipe_buffer);
		printk(KERN_INFO "numpipe module exit\n");
//	}
}

module_init(numpipe_init);
module_exit(numpipe_exit);

