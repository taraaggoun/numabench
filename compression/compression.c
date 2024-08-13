#include <time.h>
#include <fcntl.h>
#include <stdio.h>
#include <dirent.h>
#include <numaif.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h>

char dir_path[] = "linux-6.6.21";

/**
 * Return time1 - time0
 */
inline static double diff_timespec_us(const struct timespec *time1, 
					const struct timespec *time0)
{
	return 	(double)(time1->tv_sec - time0->tv_sec) * 1e3 +
		(double)(time1->tv_nsec - time0->tv_nsec) * 1e-6;
}

void tar_directory(void) {
	pid_t pid = fork();
	struct timespec a, b;
	clock_gettime(CLOCK_MONOTONIC_RAW, &a);
	if (pid == -1) {
		perror("fork");
		exit(EXIT_FAILURE);
	} else if (pid == 0) { // Child
		execlp("tar", "tar", "-cf", "archive.tar", dir_path, NULL);
		perror("execlp");
		exit(EXIT_FAILURE);
	}
	waitpid(pid, NULL, 0);

	clock_gettime(CLOCK_MONOTONIC_RAW, &b);
	printf("%f\n", diff_timespec_us(&b, &a));

	remove("archive.tar");
}

int main(void) {
	tar_directory();
	return 0;
}