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

// Ecrit toute les pages dirty du pagecache
static void sync_caches()
{
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

// Vide le page cache
void drop_caches()
{
	int fd = open("/proc/sys/vm/drop_caches", O_WRONLY);
	if (write(fd, "1", 1) <= 0) {
		perror("write");
		if (!fprintf(stderr, "drop cache requires root permission\n"))
			perror("fprintf");
		exit(1);
	}
	close(fd);
}

// Change le mask pour tout les coeurs
void setaffinity_node(unsigned int node)
{
	struct bitmask *bmp = numa_allocate_nodemask();
	numa_bitmask_setbit(bmp, node);
	if (numa_run_on_node_mask(bmp) < 0) {
		perror("numa_run_on_node_mask");
		exit(1);
	}
	numa_free_nodemask(bmp);
}

int main()
{
	sync_caches();
	drop_caches();

	setaffinity_node(0);

	struct Buf *read_buffer =
		 (struct Buf *)mmap(NULL, sizeof(char) * (unsigned long) READSIZE + sizeof(struct Buf), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, 0, 0);
	
	if (read_buffer == NULL) {
		perror("mmap");
		exit(1);
	}

	int fd = open("testfile", O_RDWR);
	int acc = 0;
	while (read(fd, read_buffer, PAGE_SIZE) > 0)
		acc += read_buffer->data[0];

	close(fd);
	munmap(read_buffer, sizeof(char) * (unsigned long) READSIZE + sizeof(struct Buf));
}