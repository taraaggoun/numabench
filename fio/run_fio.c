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
#define TEST_FILE "./media/testfile"
#define LOCAL_RESULTS "./media/local"
#define REMOTE_RESULTS "./media/remote"
int num_config = 0;

#include <sys/ioctl.h>
#define IOCTL_MAGIC 'N'
#define PIDW _IOW(IOCTL_MAGIC, 0, char *)

/**
 * Insert the pid in the kernel array with ioctl
 */
static void insert_pid_ioctl(pid_t pid)
{
	int fd = open("/dev/openctl", O_WRONLY);
	if (fd == -1)
		perror("open openctl");

	char buf[10] = { 0 };
	snprintf(buf, 10, "%d", pid);
	if (ioctl(fd, PIDW, buf) == -1)
		perror("ioctl");

	close(fd);
}

/**
 * Synchronise and empty caches
 */
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

/**
 * Create media directory and Create the file that will be read
 */
void create_test_file()
{
	if (mkdir("media", 0755) == -1) {
		if (errno != EEXIST) {
			perror("Erreur lors de la création du répertoire");
			exit(1);
		}
	}

	if (access(TEST_FILE, F_OK) != 0) {
		printf("Creating test file %s...\n", TEST_FILE);
		char cmd[512];
		snprintf(cmd, sizeof(cmd), "dd if=/dev/urandom of=%s bs=1M count=7168", TEST_FILE);
		system(cmd);
	}
}

/**
 * Enable or disable the numabalancing
 */
void set_numa_balancing(char state)
{
	const char *numa_balancing_path = "/proc/sys/kernel/numa_balancing";

	int fd = open(numa_balancing_path, O_WRONLY);
	if (fd == -1) {
		perror("Erreur lors de l'ouverture du fichier /proc/sys/kernel/numa_balancing");
		exit(1);
	}

	if (state != '0' && state != '1') {
		fprintf(stderr, "État invalide : %c. Utilisez '0' pour désactiver et '1' pour activer.\n", state);
		close(fd);
		exit(1);
	}
	
	if (write(fd, &state, 1) == -1) {
		perror("Erreur lors de l'écriture dans /proc/sys/kernel/numa_balancing");
		close(fd);
		exit(1);
	}

	close(fd);
	printf("NUMA balancing %s.\n", state == '1' ? "enable" : "disable");
}

/**
 * Set the processus on the node node
 */
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

/**
 * The proceessus can be executed on every node
 */
void setaffinity_any()
{
	if (numa_run_on_node(-1) < 0) {
		perror("numa_run_on_node");
		exit(1);
	}
}

/**
 * Run the commandline to execute fio
 */
void run_fio(const char *result_file)
{
	char cmd[512];
	snprintf(cmd, sizeof(cmd), "fio --name=read_test --filename=%s --size=%s --bs=2M --rw=read --ioengine=sync --runtime=60 --time_based --numjobs=1 --group_reporting >> %s_%d ", TEST_FILE, FILE_SIZE, result_file, num_config);
	system(cmd);
}

/**
 * Read the file to load it in memory
 */
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

int main(int argc, char *argv[])
{
	if (geteuid() != 0) {
		fprintf(stderr, "This program should be executed in root mode\n");
		exit(1);
	}

	if (numa_available() == -1) {
		fprintf(stderr, "NUMA is not available on this system.\n");
		exit(1);
	}

	if (argc > 2) {
		fprintf(stderr, "Error : too many argument\n");
		exit(1);
	}

	if (argc == 2)
		num_config = atoi(argv[1]);

	if (num_config == 3 || num_config == 5)
		set_numa_balancing('1');
	else
		set_numa_balancing('0');

	if (num_config == 2 || num_config == 3)
		insert_pid_ioctl(getpid());

	create_test_file();

	for (int i = 1; i <= ITERATIONS; i++) {
		printf("Iteration %d\n", i);
		// Test with the file loaded on node 0
		clear_cache();
		set_cpu_affinity_by_node(0);
		read_file();
		if (num_config != 0)
			setaffinity_any();
		run_fio(LOCAL_RESULTS);

		// Test with the file loaded on node 0 but reading from node 1
		clear_cache();
		set_cpu_affinity_by_node(0);
		read_file();
		set_cpu_affinity_by_node(1);
		if (num_config != 0)
			setaffinity_any();
		run_fio(REMOTE_RESULTS);
	}
	printf("Tests completed\n");
	return 0;
}