#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sched.h>
#include <string.h>
#include <errno.h>
#include <numa.h>
#include <numaif.h>

#define FILE_SIZE "7G"
#define ITERATIONS 10
#define TEST_FILE "/tmp/testfile"
#define LOCAL_RESULTS "local_results"
#define REMOTE_RESULTS "remote_results"

void clear_cache()
{
	sync();
	int fd = open("/proc/sys/vm/drop_caches", O_WRONLY);
	if (fd == -1) {
		perror("Failed to open /proc/sys/vm/drop_caches");
		exit(1);
	}
	if (write(fd, "3", 1) == -1) {
		perror("Failed to write to /proc/sys/vm/drop_caches");
		close(fd);
		exit(1);
	}
	close(fd);
}

void create_test_file()
{
	if (access(TEST_FILE, F_OK) != 0) {
		printf("Creating test file %s...\n", TEST_FILE);
		char cmd[512];
		snprintf(cmd, sizeof(cmd), "dd if=/dev/urandom of=%s bs=1M count=7168", TEST_FILE);
		system(cmd);
	}
}

void set_cpu_affinity_by_node(int node)
{
	struct bitmask *cpus = numa_allocate_cpumask();
	if (numa_node_to_cpus(node, cpus) != 0) {
		perror("numa_node_to_cpus");
		exit(1);
	}

	cpu_set_t mask;
	CPU_ZERO(&mask);
	for (int i = 0; i < cpus->size; i++) {
		if (numa_bitmask_isbitset(cpus, i)) {
			CPU_SET(i, &mask);
		}
	}

	if (sched_setaffinity(0, sizeof(cpu_set_t), &mask) == -1) {
		perror("sched_setaffinity");
		exit(1);
	}

	numa_free_cpumask(cpus);
}

// Change le mask pour tout les coeurs
void setaffinity_any()
{
	if (numa_run_on_node(-1) < 0) {
		perror("numa_run_on_node");
		exit(1);
	}
}

void run_fio(const char *result_file)
{
	char cmd[512];
	snprintf(cmd, sizeof(cmd), "fio --name=read_test --filename=%s --size=%s --bs=2M --rw=read --ioengine=sync --runtime=60 --time_based --numjobs=1 --group_reporting >> %s", TEST_FILE, FILE_SIZE, result_file);
	system(cmd);
}

void read_file()
{
	int fd = open(TEST_FILE, O_RDONLY);
	if (fd == -1) {
		perror("Failed to open test file");
		exit(1);
	}

	char buffer[4096];
	while (read(fd, buffer, sizeof(buffer)) > 0);
	close(fd);
}

int main() {
	if (numa_available() == -1) {
		fprintf(stderr, "NUMA is not available on this system.\n");
		exit(1);
	}

	int num_nodes = numa_max_node() + 1;

	create_test_file();

	for (int i = 1; i <= ITERATIONS; i++) {
		printf("Iteration %d\n", i);
		// Test with the file loaded on node 0
		clear_cache();
		set_cpu_affinity_by_node(0);
		read_file();
		// setaffinity_any();
		run_fio(LOCAL_RESULTS);

		// Test with the file loaded on node 0 but reading from node 1
		clear_cache();
		set_cpu_affinity_by_node(0);
		read_file();
		set_cpu_affinity_by_node(1);
		// setaffinity_any();
		run_fio(REMOTE_RESULTS);
	}
	printf("Tests completed. Results are stored in respective files.\n");
	return 0;
}