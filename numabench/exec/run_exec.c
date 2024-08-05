#define _GNU_SOURCE

#include <numa.h>
#include <fcntl.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/wait.h>

struct Buf {
	size_t size;
	char data[0];
};

#define READSIZE (10 * 1024 * 1024)
#define PAGE_SIZE (size_t) 0x1000

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

// Défini l'affinité de la CPU sur un nœud spécifique
void set_cpu_affinity(int node)
{
	cpu_set_t cpu_set;
	CPU_ZERO(&cpu_set);

	// Ajout des CPU du nœud spécifié au set
	struct bitmask *bm = numa_allocate_cpumask();
	numa_node_to_cpus(node, bm);
	for (int i = 0; i < bm->size; i++)
		if (numa_bitmask_isbitset(bm, i))
			CPU_SET(i, &cpu_set);

	// Appliquer l'affinité de la CPU
	if (sched_setaffinity(0, sizeof(cpu_set_t), &cpu_set) != 0) {
		perror("sched_setaffinity");
	}
	numa_free_cpumask(bm);
}

// Change le mask pour tout les coeurs
void setaffinity_any()
{
	if (numa_run_on_node(-1) < 0) {
		perror("numa_run_on_node");
		exit(1);
	}
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

struct Buf *do_buffer(size_t size)
{
	struct Buf *buffer = (struct Buf *)mmap(NULL, size + sizeof(struct Buf),
		PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, 0, 0);

	for (unsigned int i = 0; i < size; i += PAGE_SIZE)
		buffer->data[i] = 0;
	buffer->size = size;
	return buffer;
}

void free_buffer(struct Buf *buffer)
{
	if (buffer != NULL)
		munmap(buffer, sizeof(struct Buf) + buffer->size);
}

int regenerate_pagecache(int placement_pc, char *file_name)
{
	sync_caches();
	drop_caches();

	if (fork() == 0) {
		setaffinity_node(placement_pc);

		struct Buf *read_buffer =
			do_buffer(sizeof(char) * (unsigned long)READSIZE);
		if (read_buffer == NULL) {
			perror("mmap");
			exit(1);
		}
		int fd = open(file_name, O_RDWR);
		int acc = 0;
		while (read(fd, read_buffer, PAGE_SIZE) > 0)
			acc += read_buffer->data[0];

		close(fd);
		free_buffer(read_buffer);
		exit(0);
	}
	return 0;
}

// Exécute une commande shell
void execute_command(const char *command)
{
	int ret = system(command);
	if (ret != 0)
		perror("system");
}

int main(void)
{
	for (int i = 0; i < 10; i++) {
		regenerate_pagecache(0, "testfile");
		set_cpu_affinity(0);
		setaffinity_any();
		execute_command("sudo ./numabench_exec -o read -i 50 -r -f testfile >> ../media/data/read_LL_4");

		regenerate_pagecache(1, "testfile");
		set_cpu_affinity(0);
		setaffinity_any();
		execute_command("sudo ./numabench_exec -o read -i 50 -r -f testfile >> ../media/data/read_DL_4");

		regenerate_pagecache(1, "testfile");
		set_cpu_affinity(1);
		setaffinity_any();
		execute_command("sudo ./numabench_exec -o read -i 50 -r -f testfile >> ../media/data/read_LL_4");

		regenerate_pagecache(0, "testfile");
		set_cpu_affinity(1);
		setaffinity_any();
		execute_command("sudo ./numabench_exec -o read -i 50 -r -f testfile >> ../media/data/read_DL_4");

		regenerate_pagecache(0, "testfile");
		set_cpu_affinity(0);
		setaffinity_any();
		execute_command("sudo ./numabench_exec -o write -i 50 -r -f testfile >> ../media/data/write_LL_4");

		regenerate_pagecache(1, "testfile");
		set_cpu_affinity(0);
		setaffinity_any();
		execute_command("sudo ./numabench_exec -o write -i 50 -r -f testfile >> ../media/data/write_DL_4");

		regenerate_pagecache(1, "testfile");
		set_cpu_affinity(1);
		setaffinity_any();
		execute_command("sudo ./numabench_exec -o write -i 50 -r -f testfile >> ../media/data/write_LL_4");

		regenerate_pagecache(0, "testfile");
		set_cpu_affinity(1);
		setaffinity_any();
		execute_command("sudo ./numabench_exec -o write -i 50 -r -f testfile >> ../media/data/write_DL_4");
	}
	return 0;
}