#include "../numabench.h"

#include <numa.h>
#include <time.h>
#include <fcntl.h>
#include <sched.h>
#include <stdio.h>
#include <getopt.h>
#include <numaif.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/syscall.h>

/* -------------------------------------------------------------------------- */

/**
 * generate a random seed based on nanosecond
 */
static void seed_random()
{
	struct timespec a;
	if (clock_gettime(CLOCK_MONOTONIC_RAW, &a) < 0)
		perror("clock a");
	srandom(a.tv_nsec);
}

inline static void print_help()
{
	printf("numabench [-o operation] [-m migration] [-i iterations] "
			"[-t nid] [-p nid] [-b nid] [ -r | -n ] [-h]\n\n");
	printf("\t--mode -o [read|write]\n");
	printf("\t--allow-migration -m [pages|thread]\n");
	printf("\t--thread nid\n");
	printf("\t--buffer nid\n");
	printf("\t--random -r\n");
	printf("\t--sequential -s\n");
	printf("\t--pagecache nid\n");
	printf("\t--iterations -i iterations\n");
	printf("\t--verbose -v\n\n");
	printf("\t--num-patch -n\n\n");
	printf("example:\n");
	printf("numabench -o read -m pages -m thread -i 100 -t 0 -p 1 "
								"-b 0 -r\n");
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

static void parse_args(int argc, char *argv[], struct Config *config)
{
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
			{"num-patch", required_argument, 0, 'n'},
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
		case 'n':
			config->patch = atoi(optarg);
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

/**
 * Return size of the file test_file_name
 */
static size_t file_size(const char *test_file_name)
{
	struct stat st;
	if (stat(test_file_name, &st) != 0 || st.st_size < 0) {
		perror("stat");
		exit(1);
	}
	return st.st_size;
}

static size_t min(size_t a, size_t b) {
	if (a < b)
		return a;
	return b;
}

static size_t min3(size_t a, size_t b, size_t c) {
	return min(min(a, b), c);
}

/**
 * Process can run on any node
 */
static void setaffinity_any() {
	if (numa_run_on_node(-1) < 0) {
		perror("numa_run_on_node");
		exit(1);
	}
}

/* -------------------------------------------------------------------------- */

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

double file_operation(const char *file_name, struct Buf *buffer,
			enum Operation op, const bool random_op)
{
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
			sz = read(fd, buffer->data + total, min(readChunkSize, filesize - total));
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
		if (!fprintf(stderr, "error on size read/write %zd %zd\n", total, buffer->size))
			perror("fprintf");
		exit(1);
	}
	close(fd);
	return 0;
}

void do_benchmark(const struct Config *config) {
	struct Buf *buffer = do_buffer(file_size(config->file_name));

	setaffinity_any();

	struct timespec a, b;
	clock_gettime(CLOCK_MONOTONIC_RAW, &a);

	for (int i = 0; i < config->iteration_nr; i++) {
		file_operation(config->file_name, buffer, config->operation,
						config->random_operation);
		if (config->verbose) {
			struct timespec a;
			if (clock_gettime(CLOCK_MONOTONIC_RAW, &a) < 0)
				perror("clock a");

			printf("%lu %d\n", (unsigned long)((double)a.tv_sec 
						* 1e9 + (double)a.tv_nsec), i);
		}
	}

	clock_gettime(CLOCK_MONOTONIC_RAW, &b);
	printf("%f\n", diff_timespec_us(&b, &a));

	free_buffer(buffer);
}

/* -------------------------------------------------------------------------- */

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
		.patch = 0,
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

	/* random seed for random operations */
	if (config.random_operation)
		seed_random();

	/* run the benchmark */
	do_benchmark(&config);
	return 0;
}