#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>

#define N 80

int main() {
	struct timeval gtodTimes[N];
	char procClockTime[N];

	/* allocate memory for character buffers HERE before you use them */

	int fd = open("/dev/mytime", O_RDONLY);
	/* check for errors HERE */

	int i;
	int bytes_read;
	for( i=0; i < N; i++)
	{
		gettimeofday(&gtodTimes[i], 0);
		bytes_read = read(fd, procClockTime[i], 80);
		/* check for errors HERE */
	}

//	close(fd);

	for(i=0; i < N; i++) {
//		printf("...", gtodTimes[i], procClockTime[i]);
		printf("HI");
		/* fix the output format appropriately in the above line */
	}
	return 0;
}
