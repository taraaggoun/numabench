#include <numa.h>
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

#define BUFFER_SIZE 4096
char dir_path[] = "linux-6.6.21";
int node_local = 0;
int node_remote = 1;

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

/**
 * Return time1 - time0
 */
inline static double diff_timespec_us(const struct timespec *time1, 
					const struct timespec *time0)
{
	return 	(double)(time1->tv_sec - time0->tv_sec) * 1e3 +
		(double)(time1->tv_nsec - time0->tv_nsec) * 1e-6;
}

static void parse_arg(int argc, char *argv[])
{
	int opt;
	while ((opt = getopt(argc, argv, "p:l:r:")) != -1) {
		switch (opt) {
		case 'p':
			strncpy(dir_path, optarg, sizeof(dir_path) - 1);
			dir_path[sizeof(dir_path) - 1] = '\0'; // Ensure null-termination
			break;
		case 'l':
			node_local = atoi(optarg);
			break;
		case 'r':
			node_remote = atoi(optarg);
			break;
		default:
			fprintf(stderr, "Usage: %s [-p directory] [-l node_local] [-r node_remote]\n", argv[0]);
			exit(EXIT_FAILURE);
		}
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

void tar_directory(char * file_name) {
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
	double time = diff_timespec_us(&b, &a);

	char val[BUFFER_SIZE] = { 0 };
	int fd = open(file_name, O_CREAT | O_WRONLY | O_APPEND, 0644);
	if (fd == -1) {
		perror("open");
		exit(EXIT_FAILURE);
	}
	snprintf(val, BUFFER_SIZE, "%f\n", time);
	int bytes_written = write(fd, val, strlen(val));
	if (bytes_written == -1) {
		perror("write");
		close(fd);
		exit(EXIT_FAILURE);
	}
	close(fd);
	remove("archive.tar");
}

void do_benchmark(int node_pg, int node_exec, char *file_name)
{
	sync_caches();
	drop_caches();
	setaffinity_node(node_pg);
	load_memory();
	setaffinity_node(node_exec);
	tar_directory(file_name);
}

int main(int argc, char *argv[]) {
	parse_arg(argc, argv);
	/* exit if numa is not available on this machine */
	if (numa_available() != 0) {
		perror("numa_available");
		if (!fprintf(stderr,
			"No support for NUMA is available in this system\n"))
			perror("fprintf");
		exit(1);
	}
	write_numa_balencing('0');

	for (int i = 0; i < 10; i++) {
		do_benchmark(node_local, node_local, "local");
		do_benchmark(node_remote, node_remote, "local");
		do_benchmark(node_local, node_remote, "remote");
		do_benchmark(node_remote, node_local, "remote");
	}

	return 0;
}