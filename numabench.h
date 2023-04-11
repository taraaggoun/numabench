#ifndef NUMABENCH
#define NUMABENCH

#include <stdbool.h>
#include <stddef.h>

#define PAGE_SIZE 0x1000
#define MAX_NODES_POSSIBLE 9
#define ARRAY_SIZE MAX_NODES_POSSIBLE
#define ALIGN_TO_PAGE(x)                                                       \
	((void *)((((unsigned long)(x)) / (unsigned long)PAGE_SIZE) *              \
	          (unsigned long)PAGE_SIZE))

enum Layout { LL, DL, LD, DD };

struct Placement {
	unsigned int pagecache;
	unsigned int thread;
	unsigned int buffer;
};

struct Config {
	struct Placement placement;
	int iteration_nr;
	bool thread_migration;
	bool pages_migration;
	enum Layout layout;
	char *file_name;
};

struct Results {
	int iteration_nr;
	unsigned int buffer_node;
	unsigned int read_node;
	unsigned int pagecache_node;
	unsigned int *read_nodes;
	unsigned int *buffer_nodes;
	double *times_ms;
};

struct Buf {
	size_t size;
	char data[0];
};

size_t file_size(const char *test_file_name);

struct Buf *do_buffer(size_t size);
struct Buf *do_buffer_node(int node, size_t size);
inline static void print_help(int err_code);
static char *layout_to_string(enum Layout layout);

enum Layout placement_to_layout(const struct Placement *placement);
size_t min(size_t a, size_t b);

void print_placement(const struct Placement *placement);

void print_recap(const struct Config *config);

// write all the dirty pages from the pagecache
static void sync_caches();

void drop_caches();

void setaffinity_node(unsigned int node);

void setaffinity_any();

unsigned int get_num_nodes();

int regenerate_pagecache(const struct Config *config);

void do_benchmark(const struct Config *config, struct Results *results);

#endif
