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

int node_file = 0;
int node_exec = 1;

/** 
 * write all the dirty pages from the pagecache
 */
static void sync_caches() {
	int pid, status;
	pid = fork();
	if (pid == 0) {
		if (execl("/bin/sync", "sync", NULL) < 0)
			perror("execv");
		exit(1);
	}
	else if (pid < 0) {
		perror("fork");
		exit(1);
	}
	wait(&status);
}

/**
 * Empty pagecache
 */
static void drop_caches() {
	int fd = open("/proc/sys/vm/drop_caches", O_WRONLY);
	if (write(fd, "1", 1) <= 0) {
		perror("write");
		if (!fprintf(stderr, "drop cache requires root permission\n"))
			perror("fprintf");
		exit(1);
	}
	close(fd);
}

/**
 * If c equal 0 disable numa balencing
 * If c equal 1 enable numa balencing
 */
static void write_numa_balencing(char c)
{
	int fd = open("/proc/sys/kernel/numa_balancing", O_WRONLY);
	if (fd == -1) {
		perror("open numa balencing");
		exit(1);
	}
	if (write(fd, &c, 1) < 0) {
		perror("write in numa balencing");
		exit(1);
	}
	close(fd);
}

/**
 * Process can run on node "node"
 */
static void setaffinity_node(unsigned int node)
{
	struct bitmask *bmp = numa_allocate_nodemask();
	numa_bitmask_setbit(bmp, node);
	if (numa_run_on_node_mask(bmp) < 0) {
		perror("numa_run_on_node_mask");
		exit(1);
	}
	numa_free_nodemask(bmp);
}

// Change le mask pour tout les coeurs
static void setaffinity_any()
{
	if (numa_run_on_node(-1) < 0) {
		perror("numa_run_on_node");
		exit(1);
	}
}

static void load_memory(void)
{
	DIR *dir;
	struct dirent *entry;
	char filepath[BUFFER_SIZE];
	char buffer[BUFFER_SIZE];
	ssize_t bytes_read;

	if ((dir = opendir(dir_path)) == NULL) {
		perror("opendir");
		return;
	}
	while ((entry = readdir(dir)) != NULL) {
		if (entry->d_type == DT_REG) {  // Si c'est un fichier régulier
			snprintf(filepath, sizeof(filepath), "%s/%s", dir_path, entry->d_name);
			int fd = open(filepath, O_RDONLY);
			if (fd == -1) {
				perror("open");
				continue;
			}
			while ((bytes_read = read(fd, buffer, BUFFER_SIZE)) > 0) {
			}
			close(fd);
		}
	}
	closedir(dir);
}

void regenerate_pagecache(int node_pg)
{
	sync_caches();
	drop_caches();
	setaffinity_node(node_pg);
	load_memory();
}

// Exécute une commande shell
void execute_command(const char *command)
{
	int ret = system(command);
	if (ret != 0)
		perror("system");
}

int main(int argc, char *argv[])
{
	if (argc != 2) {
		printf("Error : Invalid number of argument, patch number require\n");
		exit(1);
	}
	int patch = atoi(argv[1]);
	if (patch != 5)
		write_numa_balencing('0');
	else
		write_numa_balencing('1');
	/* exit if numa is not available on this machine */
	if (numa_available() != 0) {
		perror("numa_available");
		if (!fprintf(stderr,
			"No support for NUMA is available in this system\n"))
			perror("fprintf");
		exit(1);
	}
	for (int i = 0; i < 10; i++) {
		regenerate_pagecache(0);
		set_cpu_affinity(0);
		setaffinity_any();
		execute_command("sudo ./compression >> media/data/local_0");

		regenerate_pagecache(1);
		set_cpu_affinity(0);
		setaffinity_any();
		execute_command("sudo ./compression >> media/data/remote_0");

		regenerate_pagecache(1);
		set_cpu_affinity(1);
		setaffinity_any();
		execute_command("sudo ./compression >> media/data/local_1");

		regenerate_pagecache(0);
		set_cpu_affinity(1);
		setaffinity_any();
		execute_command("sudo ./compression >> media/data/remote_1");
	}
}