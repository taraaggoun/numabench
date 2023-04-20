#include "numabench.h"
#include "json.h"
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

struct Buf *do_buffer_node(int node, size_t size) {
	setaffinity_node(node);
	return do_buffer(size);
}

inline static void print_help(int err_code) {
	printf("pagecache [-o operation] [-m migration] [-i iterations] [-t nid] "
	       "[-p nid] [-b "
	       "nid] [-h]\n");
	printf("\t--mode -o [read|write]\n");
	printf("\t--allow-migration -m [pages|thread]\n");
	printf("\t--thread nid\n");
	printf("\t--buffer nid\n");
	printf("\t--pagecache nid\n");
	printf("\t--iterations -i iterations\n");
	exit(err_code);
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
		fprintf(stderr, "switch layout not found\n");
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
	fprintf(stderr, "(%d,%d,%d)\n", p->thread, p->pagecache, p->buffer);
}

void print_recap(const struct Config *config) {
	fprintf(stderr, "operation is %s\n",
	        operation_to_string(config->operation));
	fprintf(stderr, "file :               %s\n", config->file_name);
	fprintf(stderr, "page  migration:     %s\n",
	        config->pages_migration ? "allowed" : "not allowed");
	fprintf(stderr, "thread migration:    %s\n",
	        config->thread_migration ? "allowed" : "not allowed");
	fprintf(stderr, "starting in layout:  %s\n",
	        layout_to_string(placement_to_layout(&config->placement)));
	fprintf(stderr, "placement is :       ");
	print_placement(&config->placement);
	fprintf(stderr, "\n");
}

// write all the dirty pages from the pagecache
static void sync_caches() {
	int pid, status;
	if ((pid = fork()) == 0) {
		if (execl("/bin/sync", "sync", NULL) < 0) {
			perror("execv");
			exit(1);
		}
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
		fprintf(stderr, "drop cache\n");
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

	setaffinity_node(config->placement.pagecache);

	struct Buf *read_buffer = do_buffer(sizeof(char) * READSIZE);
	if (read_buffer == NULL) {
		perror("mmap");
		exit(1);
	}
	int fd = open(config->file_name, O_RDWR);
	long acc = 0;
	while (read(fd, read_buffer, PAGE_SIZE) > 0) {
		acc += (long)read_buffer->data[0];
	}
	close(fd);
	free_buffer(read_buffer);
	return acc;
}

size_t min(size_t a, size_t b) {
	if (a < b)
		return a;
	return b;
}

double diff_timespec_us(const struct timespec *time1,
                        const struct timespec *time0) {
	return (time1->tv_sec - time0->tv_sec) * 1e6 +
	       (time1->tv_nsec - time0->tv_nsec) / 1e3;
}

double file_operation(const char *file_name, struct Buf *buffer,
                      enum Operation op) {
	size_t readChunkSize = READSIZE;
	size_t filesize = file_size(file_name);

	if (!buffer || buffer->size != filesize)
		fprintf(stderr, "incorrect buffer size");

	if (filesize < readChunkSize)
		fprintf(stderr, "file is too small for reads");

	int fd = open(file_name, O_RDWR);
	if (fd < 0) {
		perror("open");
		exit(1);
	}
	size_t total = 0;
	double total_time = 0.;
	size_t sz;
	struct timespec start, end;

	do {
		const long offset = ((random() << 32) | random()) % (filesize - 1);

		switch (op) {
		case Read:
			if (lseek(fd, offset, SEEK_SET) == -1) {
				perror("lseek");
			}

			clock_gettime(CLOCK_MONOTONIC_RAW, &start);
			sz = read(fd, buffer->data + total,
			          min(readChunkSize, filesize - total));
			clock_gettime(CLOCK_MONOTONIC_RAW, &end);
			break;
		case Write:
			clock_gettime(CLOCK_MONOTONIC_RAW, &start);
			sz = write(fd, buffer->data + offset,
			           min(readChunkSize, filesize - total));
			clock_gettime(CLOCK_MONOTONIC_RAW, &end);
			break;
		}
		total_time += diff_timespec_us(&end, &start);
		total += sz;

	} while (sz > 0);
	if (total != filesize) {
		fprintf(stderr, "read random erreur %zd %zd %zd\n", total, filesize,
		        buffer->size);
		exit(1);
	}
	close(fd);
	return total_time;
}

double read_file(const char *file_name, struct Buf *buffer) {
	return file_operation(file_name, buffer, Read);
}

double write_file(const char *file_name, struct Buf *buffer) {
	return file_operation(file_name, buffer, Write);
}

void buffer_node_pages(char *ptr, size_t len, unsigned int *pages_per_node) {

	for (char *addr = ptr; addr < ptr + len; addr += PAGE_SIZE) {
		int node = 0;

		long ret = get_mempolicy(&node, NULL, 0, (void *)addr,
		                         MPOL_F_NODE | MPOL_F_ADDR);

		if (ret < 0)
			fprintf(stderr, "get_mempolicy failed");

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
	int status;
	status = syscall(SYS_getcpu, NULL, &thread_node, NULL);
	if (status < 0) {
		perror("getcpu");
		exit(1);
	}

	results->times_us[i] = time;
	results->read_nodes[i] = thread_node;
	buffer_node_pages(buffer->data, buffer->size,
	                  &results->buffer_nodes[get_num_nodes() * i]);

	fprintf(stderr, "(%u,%u,%u): %f\n", thread_node, results->pagecache_node,
	        buffer_node, time);
}

void do_benchmark(const struct Config *config, struct Results *results) {

	setaffinity_node(config->placement.thread);
	regenerate_pagecache(config);
	setaffinity_node(config->placement.thread);
	struct Buf *buffer =
		do_buffer_node(config->placement.buffer, file_size(config->file_name));
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
		double time =
			file_operation(config->file_name, buffer, config->operation);
		compute_results(i, time, buffer, results);
	}

	free_buffer(buffer);
}

int main(int argc, char *argv[]) {
	if (numa_available() != 0) {
		perror("numa_available");
		exit(1);
	}

	srandom(time(NULL));

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
	};

	int c;
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
			{"help", no_argument, 0, 'h'},
			{0, 0, 0, 0}};

		c = getopt_long(argc, argv, "hm:s:i:o:f:t:b:p:", long_options,
		                &option_index);
		if (c == -1)
			break;

		switch (c) {
		case 'm':
			if (strcmp(optarg, "thread") == 0) {
				config.thread_migration = true;
			} else if (strcmp(optarg, "pages") == 0) {
				config.pages_migration = true;
			} else {
				fprintf(stderr, "error: --allow-migration %s not recognized\n",
				        optarg);
				print_help(1);
			}
			break;

		case 's':
			if (strcmp(optarg, "LL") == 0 || strcmp(optarg, "ll") == 0) {
				config.layout = LL;
			} else if (strcmp(optarg, "LD") == 0 || strcmp(optarg, "ld") == 0) {
				config.layout = LD;
			} else if (strcmp(optarg, "DL") == 0 || strcmp(optarg, "dl") == 0) {
				config.layout = DL;
			} else if (strcmp(optarg, "DD") == 0 || strcmp(optarg, "dd") == 0) {
				config.layout = DD;
			} else {
				printf("error: --start %s not recognized\n", optarg);
				print_help(1);
			}

			break;

		case 'i':
			config.iteration_nr = atoi(optarg);
			break;

		case 't':
			config.placement.thread = atoi(optarg);
			break;
		case 'b':
			config.placement.buffer = atoi(optarg);
			break;
		case 'p':
			config.placement.pagecache = atoi(optarg);
			break;
		case 'o':
			if (strcmp(optarg, "read") == 0)
				config.operation = Read;
			else if (strcmp(optarg, "write") == 0)
				config.operation = Write;
			else {
				fprintf(stderr, "operation %s is not supported\n", optarg);
				print_help(1);
			}
			break;
		case 'f':
			config.file_name = optarg;
			break;
		case 'h':
			print_help(0);
		case '?':
			break;
		default:
			fprintf(stderr, "?? getopt returned character code 0%o ??\n", c);
		}
	}

	struct Results results = {
		.read_node = config.placement.thread,
		.buffer_node = config.placement.buffer,
		.pagecache_node = config.placement.pagecache,
		.iteration_nr = config.iteration_nr,
	};
	allocate_struct_result(&config, &results);
	print_recap(&config);
	do_benchmark(&config, &results);
	results_to_json(&results, &config);
	free_struct_result(&results);
}
