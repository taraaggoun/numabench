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
static void seed_random()
{
	struct timespec a;
	if (clock_gettime(CLOCK_MONOTONIC_RAW, &a) < 0) {
		perror("clock a");
	}
	srandom(a.tv_nsec);
}

size_t file_size(const char *test_file_name)
{
	struct stat st;
	if (stat(test_file_name, &st) != 0 || st.st_size < 0) {
		perror("stat");
		exit(1);
	}
	return st.st_size;
}

void setaffinity_any()
{
	if (numa_run_on_node(-1) < 0) {
		perror("numa_run_on_node");
		exit(1);
	}
}

size_t min(size_t a, size_t b)
{
	if (a < b)
		return a;
	return b;
}

size_t min3(size_t a, size_t b, size_t c)
{
	return min(min(a, b), c);
}

double file_operation(const char *file_name, struct Buf *buffer, enum Operation op, const bool random_op, int fd)
{
	size_t readChunkSize = (unsigned long)READSIZE;
	size_t filesize = file_size(file_name);

	if (!buffer || buffer->size != filesize)
		if (!fprintf(stderr, "incorrect buffer size"))
			perror("fprintf");

	if (filesize < readChunkSize)
		if (!fprintf(stderr, "file is too small for reads"))
			perror("fprintf");

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
	return 0;
}

void do_benchmark(const struct Config *config)
{
	struct Buf *buffer = (struct Buf *)mmap(NULL, file_size(config->file_name) + sizeof(struct Buf), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, 0, 0);
	buffer->size = file_size(config->file_name);

	if (config->thread_migration)
		setaffinity_any();

	int fd = open(config->file_name, O_RDWR);
	if (fd < 0) {
		perror("open");
		exit(1);
	}

	struct timespec a, b;
	clock_gettime(CLOCK_MONOTONIC_RAW, &a);

	for (int i = 0; i < config->iteration_nr; i++)
		file_operation(config->file_name, buffer, config->operation, config->random_operation, fd);
	
	clock_gettime(CLOCK_MONOTONIC_RAW, &b);
	printf("%f\n", (double)(b.tv_sec - a.tv_sec) * 1e3 + (double)(b.tv_nsec - a.tv_nsec) * 1e-6);

	close(fd);
	munmap(buffer, sizeof(struct Buf) + file_size(config->file_name));
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
				exit(1);
			}
			break;
		case 'f':
			config->file_name = optarg;
			break;
		case 'v':
			config->verbose = true;
			break;
		case '?':
			break;
		default:
			if (!fprintf(stderr, "?? getopt returned character code 0%o ??\n",
			             c))
				perror("fprintf");
		}
	}

	if (random_set && sequential_set) {
		if (!fprintf(stderr,
		             "--sequential and --random are mutually exclusive\n"))
			perror("fprintf");
		exit(1);
	}
}

int main(int argc, char *argv[])
{
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
		if (!fprintf(stderr, "No support for NUMA is available in this system\n"))
			perror("fprintf");
		exit(1);
	}
	/* random seed for random operations */
	if (config.random_operation)
		seed_random();

	/* run the benchmark */
	do_benchmark(&config);
}