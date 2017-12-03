#include <linux/init.h>
#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/semaphore.h>
#include <linux/mutex.h>
#include <asm/uaccess.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Harvey Chen");

static unsigned int used_by; // keep track of how programs are using numpipe
static int numpipe_open(struct inode *, struct file *);
static int numpipe_close(struct inode *, struct file *);
static ssize_t numpipe_read(struct file *, char __user *, size_t, loff_t *);
static ssize_t numpipe_write(struct file *, const char __user *, size_t, loff_t *);

static int pipe_size = 1; // pipe is size 1 by default
module_param(pipe_size, int, S_IRUGO); // set pipe size using parameter when inserting module

static int *pipe_buffer; // int array to store everything in pipe
static unsigned int pipe_elements; // number of elements currently in pipe
/*
//static struct semaphore mutex; // binary semaphore to lock critical sections
static struct semaphore empty; // semaphore to track empty slots in pipe buffer
static struct semaphore full; // semaphore to track used slots in pipe buffer
*/
static DEFINE_MUTEX(lock);
static DEFINE_SEMAPHORE(empty);
static DEFINE_SEMAPHORE(full);

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

	used_by = 0; // initially used by 0 programs

	// allocate memory in kernel for an int array depending on pipe size
	pipe_buffer = (int*) kmalloc(pipe_size * sizeof(int), GFP_KERNEL);
	for(index = 0; index < pipe_size; index++) {
		pipe_buffer[index] = '\0'; // set everything to null initially
	}
	pipe_elements = 0; // when init there is nothing in the pipe

//	sema_init(&mutex, 1); // 1 for binary semaphore
	mutex_init(&lock);
	sema_init(&empty, pipe_size); // empty is equal to the size of pipe
	sema_init(&full, 0); // always nothing full initially

	printk(KERN_INFO "numpipe module init success\n");
	return 0;
}

static void __exit numpipe_exit(void) {
	if(used_by == 0) { // don't want to remove module if currently being used
		misc_deregister(&numpipe_device); // deregister the device
		kfree(pipe_buffer); // free the memory in kernel that allocated earlier for pipe buffer
		printk(KERN_INFO "numpipe module exit\n");
	}
	else {
		printk(KERN_INFO "numpipe module exit failed, device still in use");
	}
}

static int numpipe_open(struct inode *inode, struct file *file) {
	used_by++; // whenever numpipe is opened it is being used
	printk(KERN_INFO "numpipe opened, currently used by %d devices\n", used_by);
	return 0;
}

static int numpipe_close(struct inode *inode, struct file *file) {
	if(used_by != 0) { // can't close something if nothing is opened
		used_by--; // whenever numpipe it isn't be used anymore
		printk(KERN_INFO "numpipe closed, currently used by %d devices\n", used_by);
	}
	else {
		printk(KERN_INFO "numpipe isn't be used, so can't be closed");
	}
	return 0;
}

static ssize_t numpipe_read(struct file *file, char __user *out, size_t size, loff_t *off) {
	unsigned long bytes_read = sizeof(int); // user reads 4 bytes, size of an int, from kernel

	// for debugging purpose, print out current pipe buffer before read
	int index;
	printk(KERN_INFO "Pipe buffer before reading");
	for(index = 0; index < pipe_size; index++) {
		printk(KERN_INFO "%d ", pipe_buffer[index]);
	}
	printk(KERN_INFO "\n");

	if(access_ok(VERIFY_WRITE, out, bytes_read)) { // verify if we can write to userspace
		down_interruptible(&full);
//		down_interruptible(&mutex);
		mutex_lock_interruptible(&lock);

		copy_to_user(out, &pipe_buffer[0], bytes_read); // copy to user first thing inserted to before
		for(index = 0; index < pipe_size - 1; index++) { // move everything down one like a queue
			pipe_buffer[index] = pipe_buffer[index + 1];
		}
		pipe_buffer[pipe_size - 1] = '\0'; // set last element as null since one thing was removed
		pipe_elements--; // whenever something is read from kernel one less element in pipe

		mutex_unlock(&lock);
//		up(&mutex);
		up(&empty);
		return bytes_read; // return the amount of bytes read, should always be 4
	}
	else {
		return -EFAULT; // if for some reason isn't able to write, return error
	}
}

static ssize_t numpipe_write(struct file *file, const char __user *out, size_t size, loff_t *off) {
	unsigned long bytes_written = sizeof(int); // user writes 4 bytes, size of an int, to kernel

	// for debugging purpose, print out current pipe buffer before writing
	int index;
	printk(KERN_INFO "Pipe buffer before writing");
	for(index = 0; index < pipe_size; index++) {
		printk(KERN_INFO "%d ", pipe_buffer[index]);
	}
	printk(KERN_INFO "\n");

	if(access_ok(VERIFY_READ, out, bytes_written)) { // verify if able to read from userspace
		down_interruptible(&empty);
//		down_interruptible(&mutex);
		mutex_lock_interruptible(&lock);

		copy_from_user(&pipe_buffer[pipe_elements], out, bytes_written); // write to end of the pipe buffer
		pipe_elements++; // whenever something is written from userspace there is another thing in pipe

		mutex_unlock(&lock);
//		up(&mutex);
		up(&full);
		return bytes_written;
	}
	else {
		return -EFAULT; // if for some reason isn't able to read, return error
	}
}

module_init(numpipe_init);
module_exit(numpipe_exit);
