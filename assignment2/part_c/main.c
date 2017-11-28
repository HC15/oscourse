#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>
#include <string.h>

#define N 10
#define MAX_LENGTH 100

int main() {
	struct timeval timeOfDay[N];
	char kernel_time[N][MAX_LENGTH];

	int fd;
	fd = open("/dev/mytime", O_RDONLY);
	if(fd < 0) {
		perror("Failed to open: ");
		return errno;
	}

	int success;
	int bytes_read;
	int i;
	for(i = 0; i < N; i++) {
		success = gettimeofday(&timeOfDay[i], NULL);
		if(success < 0) {
			perror("Time of day failed: ");
			return errno;
		}
		bytes_read = read(fd, &kernel_time[i], MAX_LENGTH);
		if(bytes_read < 0) {
			perror("Failed to read: ");
			return errno;
		}
	}

	for(i = 0; i < N; i++) {
		printf("Kernel module\n%s\n", kernel_time[i]);

		printf("User level\n");
		printf("Time of day in seconds: %ld\n", timeOfDay[i].tv_sec);
		printf("Time of day in microseconds: %ld\n\n", timeOfDay[i].tv_usec);
	}
	return 0;
}
