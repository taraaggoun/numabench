#include "numabench.h"
#include <fcntl.h>
#include <getopt.h>
#include <numa.h>
#include <numaif.h>
#include <sched.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

/* write all the dirty pages from the pagecache */
static void sync_caches() {
	int pid, status;

	pid = fork();
	if (pid == 0) {
		if (execl("/bin/sync", "sync", NULL) < 0) {
			perror("execv");
		}
		exit(1);
	} else if (pid < 0) {
		perror("fork");
		exit(1);
	}
	wait(&status);
}

void drop_caches() {
	int fd = open("/proc/sys/vm/drop_caches", O_WRONLY);
	if (write(fd, "1", 1) <= 0) {
		perror("write");
		if (!fprintf(stderr, "drop cache requires root permission\n"))
			perror("fprintf");
		exit(1);
	}
	close(fd);
}

void setaffinity_node(unsigned int node) {
	struct bitmask *bmp = numa_allocate_nodemask();
	numa_bitmask_setbit(bmp, node);
	if (numa_run_on_node_mask(bmp) < 0) {
		perror("numa_run_on_node_mask");
		exit(1);
	}
	numa_free_nodemask(bmp);
}

void free_buffer(struct Buf *buffer) {
	if (buffer != NULL)
		munmap(buffer, sizeof(struct Buf) + buffer->size);
}

int main()
{
	sync_caches();
	drop_caches();

	setaffinity_node(0);

	struct Buf *read_buffer =
		do_buffer(sizeof(char) * (unsigned long)READSIZE);
	if (read_buffer == NULL) {
		perror("mmap");
		exit(1);
	}
	int fd = open("testfile", O_RDWR);
	int acc = 0;
	while (read(fd, read_buffer, PAGE_SIZE) > 0) {
		acc += read_buffer->data[0];
	}
	close(fd);
	free_buffer(read_buffer);
}