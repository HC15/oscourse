#include <linux/linkage.h>
#include <linux/time.h>
#include <asm/uaccess.h>
#include <linux/printk.h>
#include <linux/slab.h>

asmlinkage int sys_my_xtime(struct timespec *current_time) {
	if(access_ok(VERIFY_WRITE, current_time, sizeof(current_time))) {
		*current_time = current_kernel_time();
		printk(KERN_DEFAULT "Current time in nanoseconds: %lu", current_time->tv_nsec);
		return 0;
	}
	else{
		return -EFAULT;
	}
}

EXPORT_SYMBOL(sys_my_xtime);
