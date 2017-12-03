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

static int pipe_size = 1; // pipe is size 1 by default
module_param(pipe_size, int, S_IRUGO); // set pipe size using parameter when inserting module

static int *pipe_buffer; // int array to store everything in pipe, pointer because not sure what array size is
static unsigned int pipe_elements; // number of elements currently in pipe

static DEFINE_MUTEX(lock); // define a mutex to lock over critcal region
static DEFINE_SEMAPHORE(empty); // semaphore to track empty slots in the pipe buffer
static DEFINE_SEMAPHORE(full); // semaphore to track used slots in the pipe buffer

static unsigned int used_by; // keep track of how programs are using numpipe
static int numpipe_open(struct inode *, struct file *);
static int numpipe_close(struct inode *, struct file *);
static ssize_t numpipe_read(struct file *, char __user *, size_t, loff_t *);
static ssize_t numpipe_write(struct file *, const char __user *, size_t, loff_t *);

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

	// register device and check for possible error
	printk(KERN_INFO "numpipe module init\n");
	device_number = misc_register(&numpipe_device);
	if(device_number < 0) {
		printk(KERN_ERR "numpipe module init failed\n");
		return device_number;
	}

	// allocate memory in kernel for an int array depending on pipe size
	pipe_buffer = (int*) kmalloc(pipe_size * sizeof(int), GFP_KERNEL);
	for(index = 0; index < pipe_size; index++) {
		pipe_buffer[index] = '\0'; // set everything to null initially
	}
	pipe_elements = 0; // when init there is nothing in the pipe

	mutex_init(&lock); // init the mutex
	sema_init(&empty, pipe_size); // empty is equal to the size of pipe
	sema_init(&full, 0); // always nothing in buffer initially

	used_by = 0; // initially used by 0 programs

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
/*
	// for debugging purpose, print out current pipe buffer before read
	int index;
	printk(KERN_INFO "Pipe buffer before reading");
	for(index = 0; index < pipe_size; index++) {
		printk(KERN_INFO "%d ", pipe_buffer[index]);
	}
	printk(KERN_INFO "\n");
*/
	int error_interruptible; // int to check if interruptible produced an error
	if(access_ok(VERIFY_WRITE, out, bytes_read)) { // verify if we can write to userspace
		// down
		error_interruptible = down_interruptible(&full); // preform down on full, if nothing in buffer sleep
		if(error_interruptible != 0) { // if down returns something besides a 0, that means there was error
			return -EINTR; // this error message means interrupted system call (ctrl + c)
		}
		error_interruptible = mutex_lock_interruptible(&lock); // attempt to lock the critcal region
		if(error_interruptible != 0) { // if it returns something beside 0 then an error occured
			return -EINTR;
		}

		// critcal region
		copy_to_user(out, &pipe_buffer[0], bytes_read); // copy to user first thing inserted to before
		for(index = 0; index < pipe_size - 1; index++) {
			pipe_buffer[index] = pipe_buffer[index + 1]; // move everything down one, like a queue
		}
		pipe_buffer[pipe_size - 1] = '\0'; // set last element as null since one thing was removed
		pipe_elements--; // whenever something is read from kernel one less element in pipe

		// up
		mutex_unlock(&lock); // unlock the critcal region
		up(&empty); // up on the empty to wake any sleeping producers

		return bytes_read; // return the amount of bytes read, should always be 4
	}
	else {
		return -EFAULT; // if for some reason isn't able to write, return EFAULT for bad address
	}
}

static ssize_t numpipe_write(struct file *file, const char __user *out, size_t size, loff_t *off) {
	unsigned long bytes_written = sizeof(int); // user writes 4 bytes, size of an int, to kernel
/*
	// for debugging purpose, print out current pipe buffer before writing
	int index;
	printk(KERN_INFO "Pipe buffer before writing");
	for(index = 0; index < pipe_size; index++) {
		printk(KERN_INFO "%d ", pipe_buffer[index]);
	}
	printk(KERN_INFO "\n");
*/
	int error_interruptible; // int to check if interruptible produced an error
	if(access_ok(VERIFY_READ, out, bytes_written)) { // verify if able to read from userspace
		// down
		error_interruptible = down_interruptible(&empty); // preform down on empty, if buffer is full then sleep
		if(error_interruptible != 0) { // if down returns something besides a 0, that means there was error
			return -EINTR; // this error message means interrupted system call (ctrl + c)
		}
		error_interruptible = mutex_lock_interruptible(&lock); // attempt to lock the critical region
		if(error_interruptible != 0) { // if it returns something beside 0 then an error occured
			return -EINTR;
		}

		// critcal region
		copy_from_user(&pipe_buffer[pipe_elements], out, bytes_written); // write to end of the pipe buffer
		pipe_elements++; // whenever something is written from userspace there is another thing in pipe

		// up
		mutex_unlock(&lock); // unlock the critcal region
		up(&full); // up on the full to wake any sleeping consumers

		return bytes_written;
	}
	else {
		return -EFAULT; // if for some reason isn't able to read, return error
	}
}

module_init(numpipe_init);
module_exit(numpipe_exit);
