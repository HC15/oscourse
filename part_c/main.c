#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>

#define N 80

int main() {
	struct timeval timeOfDay[N];

	int fd;
	fd = open("/dev/mytime", O_RDONLY);
	if(fd < 0) {
		perror("Failed to open: ");
		return errno;
	}

	char kernel_time[N];
	int bytes_read;
	bytes_read = read(fd, kernel_time, N);
	if(bytes_read < 0) {
		perror("Failed to read: ");
		return errno;
	}

	int success;
	success = gettimeofday(&timeOfDay, NULL);
	if(success < 0) {
		perror("Time of day failed: ");
		return errno;
	}

	printf("Kernel module\n%s\n", kernel_time);

	printf("User level\n");
	printf("Time of day in seconds: %ld\n", timeOfDay->tv_sec);
	printf("Time of day in microseconds: %ld\n", timeOfDay->tv_usec);

	return 0;
}
