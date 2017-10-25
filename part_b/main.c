#include <stdio.h>
#include <unistd.h>
#include <linux/unistd.h>
#include <linux/time.h>

#define _NR_sys_my_xtime 326

int main() {
	int success;
	struct timespec ts;
	success = syscall(_NR_sys_my_xtime, &ts);
	if(success == 0) printf("Current time in seconds: %ld\nCurrent time in nanoseconds: %ld\n", ts.tv_sec, ts.tv_nsec);
	printf("syscall return value: %d\n", success); // 0 means call successful
	return 0;
}
