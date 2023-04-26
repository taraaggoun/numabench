#include "module.h"
#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void config_module(void *buf, int pagecache_node, int pid, size_t size) {
	int fd, n;
	char str[128];

	fd = open("/sys/module/dump_numa_balancing/parameters/addr", O_WRONLY);
	if (fd < 0) {
		perror("open");
		exit(1);
	}
	n = snprintf(str, 128, "%ld", (unsigned long)buf);
	write(fd, str, n + 1);
	close(fd);

	fd = open("/sys/module/dump_numa_balancing/parameters/pagecache_node",
	          O_WRONLY);
	if (fd < 0) {
		perror("open");
		exit(1);
	}
	n = snprintf(str, 128, "%d", pagecache_node);
	write(fd, str, n + 1);
	close(fd);

	fd = open("/sys/module/dump_numa_balancing/parameters/pid", O_WRONLY);
	if (fd < 0) {
		perror("open");
		exit(1);
	}
	n = snprintf(str, 128, "%d", pid);
	write(fd, str, n + 1);
	close(fd);

	fd = open("/sys/module/dump_numa_balancing/parameters/size", O_WRONLY);
	if (fd < 0) {
		perror("open");
		exit(1);
	}
	n = snprintf(str, 128, "%zu", size);
	write(fd, str, n + 1);
	close(fd);
}

void start_module() {
	int fd, n;
	char one[] = "1";

	fd = open("/sys/kernel/debug/numa_dump/start", O_WRONLY);
	if (fd < 0) {
		perror("open");
		exit(1);
	}
	n = write(fd, one, strlen(one));
	if (n < 0) {
		perror("write");
		exit(1);
	}
	close(fd);
}

void pause_module() {
	int fd, n;
	char one[] = "1";

	fd = open("/sys/kernel/debug/numa_dump/stop", O_WRONLY);
	if (fd < 0) {
		perror("open");
		exit(1);
	}
	n = write(fd, one, strlen(one));
	if (n < 0) {
		perror("write");
		exit(1);
	}
	close(fd);
}

void force_log_module() {
	int fd, n;
	char one[] = "1";

	fd = open("/sys/kernel/debug/numa_dump/manual_log", O_WRONLY);
	if (fd < 0) {
		perror("open");
		exit(1);
	}
	n = write(fd, one, strlen(one));
	if (n < 0) {
		perror("write");
		exit(1);
	}
	close(fd);
}
