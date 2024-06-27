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

/* generate a random seed based on nanosecond */
static void seed_random() {
	struct timespec a;
	if (clock_gettime(CLOCK_MONOTONIC_RAW, &a) < 0) {
		perror("clock a");
	}
	srandom(a.tv_nsec);
}

size_t file_size(const char *test_file_name) {
	struct stat st;
	if (stat(test_file_name, &st) != 0 || st.st_size < 0) {
		perror("stat");
		exit(1);
	}
	return st.st_size;
}

struct Buf *do_buffer(size_t size) {
	struct Buf *buffer = (struct Buf *)mmap(NULL, size + sizeof(struct Buf),
	                                        PROT_READ | PROT_WRITE,
	                                        MAP_ANONYMOUS | MAP_PRIVATE, 0, 0);
	for (unsigned int i = 0; i < size; i += PAGE_SIZE) {
		buffer->data[i] = 0;
	}
	buffer->size = size;
	return buffer;
}

void allocate_struct_result(struct Config *config, struct Results *results) {
	int len = results->iteration_nr;
	results->read_node = config->placement.thread;
	results->buffer_node = config->placement.buffer;
	results->pagecache_node = config->placement.pagecache;

	results->read_nodes = (unsigned int *)malloc(len * sizeof(unsigned int));
	results->buffer_nodes =
		(unsigned int *)malloc(len * sizeof(unsigned int) * get_num_nodes());
	memset(results->buffer_nodes, 0,
	       len * sizeof(unsigned int) * get_num_nodes());
	results->times_us = (double *)malloc(len * sizeof(double));
}

void free_struct_result(struct Results *results) {
	free(results->read_nodes);
	free(results->buffer_nodes);
	free(results->times_us);
}

void free_buffer(struct Buf *buffer) {
	if (buffer != NULL)
		munmap(buffer, sizeof(struct Buf) + buffer->size);
}

struct Buf *do_buffer_node(unsigned int node, size_t size) {
	setaffinity_node(node);
	return do_buffer(size);
}

inline static void print_help() {
	printf("numabench [-o operation] [-m migration] [-i iterations] [-t nid] "
	       "[-p nid] [-b nid] [ -r | -n ] [-h]\n\n");
	printf("\t--mode -o [read|write]\n");
	printf("\t--allow-migration -m [pages|thread]\n");
	printf("\t--thread nid\n");
	printf("\t--buffer nid\n");
	printf("\t--random -r\n");
	printf("\t--sequential -s\n");
	printf("\t--pagecache nid\n");
	printf("\t--iterations -i iterations\n");
	printf("\t--verbose -v\n\n");
	printf("example:\n");
	printf("numabench -o read -m pages -m thread -i 100 -t 0 -p 1 -b 0 -r\n");
}

static char *layout_to_string(enum Layout layout) {
	switch (layout) {
	case (LL):
		return "LL";
	case (LD):
		return "LD";
	case (DL):
		return "DL";
	case (DD):
		return "DD";
	default:
		if (!fprintf(stderr, "switch layout not found\n"))
			perror("fprintf");
		exit(1);
	}
}

enum Layout placement_to_layout(const struct Placement *placement) {
	const struct Placement *p = placement;
	if (p->thread == p->pagecache && p->thread == p->buffer) {
		return LL;
	} else if (p->thread == p->pagecache && p->thread != p->buffer) {
		return LD;
	} else if (p->thread != p->pagecache && p->thread == p->buffer) {
		return DL;
	} else {
		return DD;
	}
}

char *operation_to_string(const enum Operation operation) {
	switch (operation) {
	case Read:
		return "read";
	case Write:
		return "write";
	}
	return NULL;
}

void print_placement(const struct Placement *placement) {
	const struct Placement *p = placement;
	printf("(%d,%d,%d)\n", p->thread, p->pagecache, p->buffer);
}

void print_recap(const struct Config *config) {
	printf("pid is:              %d\n", getpid());
	printf("operation is:        %s %s\n",
	       config->random_operation ? "random" : "sequential",
	       operation_to_string(config->operation));
	printf("file:                %s\n", config->file_name);
	printf("page  migration:     %s\n",
	       config->pages_migration ? "allowed" : "not allowed");
	printf("thread migration:    %s\n",
	       config->thread_migration ? "allowed" : "not allowed");
	printf("starting in layout:  %s\n",
	       layout_to_string(placement_to_layout(&config->placement)));
	printf("placement is:        ");
	print_placement(&config->placement);
}

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

void setaffinity_any() {
	if (numa_run_on_node(-1) < 0) {
		perror("numa_run_on_node");
		exit(1);
	}
}

unsigned int get_num_nodes() {
	return numa_num_configured_nodes();
}

int regenerate_pagecache(const struct Config *config) {
	sync_caches();
	drop_caches();

	if (fork() == 0) {

		setaffinity_node(config->placement.pagecache);

		struct Buf *read_buffer =
			do_buffer(sizeof(char) * (unsigned long)READSIZE);
		if (read_buffer == NULL) {
			perror("mmap");
			exit(1);
		}
		int fd = open(config->file_name, O_RDWR);
		int acc = 0;
		while (read(fd, read_buffer, PAGE_SIZE) > 0) {
			acc += read_buffer->data[0];
		}
		close(fd);
		free_buffer(read_buffer);
		exit(0);
	}
	return 0;
}

size_t min(size_t a, size_t b) {
	if (a < b)
		return a;
	return b;
}

size_t min3(size_t a, size_t b, size_t c) {
	return min(min(a, b), c);
}

inline double diff_timespec_us(const struct timespec *time1,
                               const struct timespec *time0) {
	return (double)(time1->tv_sec - time0->tv_sec) * 1e6 +
	       (double)(time1->tv_nsec - time0->tv_nsec) / 1e3;
}

double file_operation(const char *file_name, struct Buf *buffer,
                      enum Operation op, const bool random_op) {
	size_t readChunkSize = (unsigned long)READSIZE;
	size_t filesize = file_size(file_name);

	if (!buffer || buffer->size != filesize)
		if (!fprintf(stderr, "incorrect buffer size"))
			perror("fprintf");

	if (filesize < readChunkSize)
		if (!fprintf(stderr, "file is too small for reads"))
			perror("fprintf");

	int fd = open(file_name, O_RDWR);
	if (fd < 0) {
		perror("open");
		exit(1);
	}
	size_t total = 0;
	size_t left = filesize;
	size_t sz;

	do {
		unsigned long offset = 0;

		if (random_op) {
			offset = ((random() << 32) | random()) % (filesize - 1);
			if (lseek(fd, (long)offset, SEEK_SET) == -1)
				perror("lseek");
		}

		switch (op) {
		case Read:
			sz = read(fd, buffer->data + total,
			          min(readChunkSize, filesize - total));
			break;
		case Write:
			sz =
				write(fd, buffer->data + total,
			          min3(readChunkSize, filesize - offset, filesize - total));
			break;
		}
		total += sz;
		left -= sz;

	} while (left > 0);
	if (total != filesize) {
		if (!fprintf(stderr, "error on size read/write %zd %zd\n", total,
		             buffer->size))
			perror("fprintf");
		exit(1);
	}
	close(fd);
	return 0;
}

double read_file(const char *file_name, struct Buf *buffer,
                 const bool random_op) {
	return file_operation(file_name, buffer, Read, random_op);
}

double write_file(const char *file_name, struct Buf *buffer,
                  const bool random_op) {
	return file_operation(file_name, buffer, Write, random_op);
}

void buffer_node_pages(char *ptr, size_t len, unsigned int *pages_per_node) {

	for (char *addr = ptr; addr < ptr + len; addr += PAGE_SIZE) {
		int node = 0;

		long ret = get_mempolicy(&node, NULL, 0, (void *)addr,
		                         MPOL_F_NODE | MPOL_F_ADDR);

		if (ret < 0)
			if (!fprintf(stderr, "get_mempolicy failed"))
				perror("fprintf");

		pages_per_node[node]++;
	}
}

unsigned int buffer_node_maxpage(char *ptr, size_t len) {
	const unsigned int num_nodes = (int)get_num_nodes();
	unsigned int nodes[ARRAY_SIZE] = {0};
	buffer_node_pages(ptr, len, nodes);

	unsigned int maxnid = 0;
	unsigned int maxnodes = 0;
	for (unsigned int i = 0; i < num_nodes; i++) {
		if (nodes[i] > maxnodes) {
			maxnodes = nodes[i];
			maxnid = i;
		}
	}
	return maxnid;
}

void compute_results(int i, double time, struct Buf *buffer,
                     struct Results *results) {
	unsigned int thread_node;
	unsigned int buffer_node = buffer_node_maxpage(buffer->data, buffer->size);
	long status;

	status = syscall(SYS_getcpu, NULL, &thread_node, NULL);
	if (status < 0) {
		perror("getcpu");
		exit(1);
	}

	results->times_us[i] = time;
	results->read_nodes[i] = thread_node;
	buffer_node_pages(buffer->data, buffer->size,
	                  &results->buffer_nodes[(size_t)get_num_nodes() * i]);

	if (!fprintf(stderr, "(%u,%u,%u): %f\n", thread_node,
	             results->pagecache_node, buffer_node, time))
		perror("fprintf");
}

void do_benchmark(const struct Config *config) {

	setaffinity_node(config->placement.thread);
	regenerate_pagecache(config);
	setaffinity_node(config->placement.thread);
	struct Buf *buffer = do_buffer_node((int)config->placement.buffer,
	                                    file_size(config->file_name));
	setaffinity_node(config->placement.thread);

	if (!config->pages_migration) {
		unsigned long nodemask = 1ul << config->placement.buffer;
		if (mbind(ALIGN_TO_PAGE(buffer->data), buffer->size, MPOL_BIND,
		          &nodemask, sizeof(unsigned long) * 8,
		          MPOL_MF_MOVE | MPOL_MF_STRICT) < 0) {
			perror("mbind");
			exit(1);
		}
	}

	if (config->thread_migration) {
		setaffinity_any();
	}

	for (int i = 0; i < config->iteration_nr; i++) {
		file_operation(config->file_name, buffer, config->operation,
		               config->random_operation);
		/* compute_results(i, time, buffer, results); */
		if (config->verbose) {
			struct timespec a;
			if (clock_gettime(CLOCK_MONOTONIC_RAW, &a) < 0) {
				perror("clock a");
			}
			printf("%lu %d\n",
			       (unsigned long)((double)a.tv_sec * 1e9 + (double)a.tv_nsec),
			       i);
		}
	}

	free_buffer(buffer);
}

static void parse_args(int argc, char *argv[], struct Config *config) {
	int c;
	bool sequential_set = false;
	bool random_set = false;
	while (1) {
		int option_index = 0;
		static struct option long_options[] = {
			{"allow-migration", required_argument, 0, 'm'},
			{"thread", required_argument, 0, 't'},
			{"buffer", required_argument, 0, 'b'},
			{"mode", required_argument, 0, 'o'},
			{"pagecache", required_argument, 0, 'p'},
			{"iterations", required_argument, 0, 'i'},
			{"file", required_argument, 0, 'f'},
			{"random", no_argument, 0, 'r'},
			{"sequential", no_argument, 0, 's'},
			{"help", no_argument, 0, 'h'},
			{"verbose", no_argument, 0, 'v'},
			{0, 0, 0, 0}};

		c = getopt_long(argc, argv, "rsnhvm:i:o:f:t:b:p:", long_options,
		                &option_index);
		if (c == -1)
			break;

		switch (c) {
		case 'm':
			if (strcmp(optarg, "thread") == 0) {
				config->thread_migration = true;
			} else if (strcmp(optarg, "pages") == 0) {
				config->pages_migration = true;
			} else {
				fprintf(stderr, "error: --allow-migration %s not recognized\n",
				        optarg);
				print_help();
				exit(1);
			}
			break;
		case 'r':
			random_set = true;
			config->random_operation = true;
			break;
		case 's':
			sequential_set = true;
			config->random_operation = false;
			break;
		case 'i':
			config->iteration_nr = atoi(optarg);
			break;

		case 't':
			config->placement.thread = atoi(optarg);
			break;
		case 'b':
			config->placement.buffer = atoi(optarg);
			break;
		case 'p':
			config->placement.pagecache = atoi(optarg);
			break;
		case 'o':
			if (strcmp(optarg, "read") == 0)
				config->operation = Read;
			else if (strcmp(optarg, "write") == 0)
				config->operation = Write;
			else {
				if (!fprintf(stderr, "operation %s is not supported\n", optarg))
					perror("fprintf");
				print_help();
				exit(1);
			}
			break;
		case 'f':
			config->file_name = optarg;
			break;
		case 'v':
			config->verbose = true;
			break;
		case 'h':
			print_help();
			exit(0);
		case '?':
			break;
		default:
			if (!fprintf(stderr, "?? getopt returned character code 0%o ??\n",
			             c))
				perror("fprintf");
		}
	}

	if (random_set && sequential_set) {
		print_help();
		if (!fprintf(stderr,
		             "--sequential and --random are mutually exclusive\n"))
			perror("fprintf");
		exit(1);
	}
}

int main(int argc, char *argv[]) {
	/* default config */
	struct Config config = {
		.placement =
			{
				.thread = 0,
				.buffer = 0,
				.pagecache = 0,
			},
		.file_name = "testfile",
		.operation = Read,
		.iteration_nr = 20,
		.pages_migration = false,
		.thread_migration = false,
		.random_operation = true,
		.verbose = false,
	};

	/* parse the command arguments */
	parse_args(argc, argv, &config);

	/* exit if numa is not available on this machine */
	if (numa_available() != 0) {
		perror("numa_available");
		if (!fprintf(stderr,
		             "No support for NUMA is available in this system\n"))
			perror("fprintf");
		exit(1);
	}
	print_recap(&config);

	/* random seed for random operations */
	if (config.random_operation)
		seed_random();

	struct timespec a, b;
	clock_gettime(CLOCK_MONOTONIC_RAW, &a);

	/* run the benchmark */
	do_benchmark(&config);

	clock_gettime(CLOCK_MONOTONIC_RAW, &b);
	printf("run took %fms\n\n", (double)(b.tv_sec - a.tv_sec) * 1e3 +
	                                (double)(b.tv_nsec - a.tv_nsec) * 1e-6);
}
