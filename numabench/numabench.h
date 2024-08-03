#ifndef NUMABENCH
#define NUMABENCH

#include <stdbool.h>
#include <stddef.h>

#define MAX_NODES_POSSIBLE 16
#define PAGE_SIZE (size_t) 0x1000
#define READSIZE (10 * 1024 * 1024)
#define ARRAY_SIZE MAX_NODES_POSSIBLE
#define ALIGN_TO_PAGE(x)                                             \
	((void *)((((unsigned long)(x)) / (unsigned long)PAGE_SIZE) * \
	(unsigned long)PAGE_SIZE))

enum Layout { LL, DL, LD, DD };
enum Operation { Read, Write };

struct Placement {
	unsigned int pagecache;
	unsigned int thread;
	unsigned int buffer;
};

struct Config {
	struct Placement placement;
	int iteration_nr;
	enum Operation operation;
	bool thread_migration;
	bool pages_migration;
	bool random_operation;
	enum Layout layout;
	char *file_name;
	bool verbose;
	int patch;
};

struct Results {
	int iteration_nr;
	unsigned int buffer_node;
	unsigned int read_node;
	unsigned int pagecache_node;
	unsigned int *read_nodes;
	unsigned int *buffer_nodes;
	double *times_us;
};

struct Buf {
	size_t size;
	char data[0];
};

#endif // NUMABENCH