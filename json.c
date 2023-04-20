#include "json.h"
#include "numabench.h"
#include <stdio.h>

void uint_array_to_json(unsigned int *array, unsigned int length, bool ending) {
	printf("[");
	for (unsigned int i = 0; i < length - 1; i++) {
		printf("%u", array[i]);
		printf(",");
	}
	printf("%u", array[length - 1]);
	if (ending)
		printf("]");
	else
		printf("],");
}

void double_array_to_json(double *array, unsigned int length, bool ending) {
	printf("[");
	for (unsigned int i = 0; i < length - 1; i++) {
		printf("%f", array[i]);
		printf(",");
	}
	printf("%f", array[length - 1]);
	if (ending)
		printf("]");
	else
		printf("],");
}

void buffer_array_to_json(unsigned int *array, unsigned int length,
                          unsigned int max_numa_nodes, bool ending) {
	printf("[");
	for (unsigned int i = 0; i < length - 1; i++) {
		uint_array_to_json(&array[i * max_numa_nodes], max_numa_nodes, false);
	}
	uint_array_to_json(&array[(length - 1) * max_numa_nodes], max_numa_nodes,
	                   true);
	if (ending)
		printf("]");
	else
		printf("],");
}

void results_to_json(struct Results *results, struct Config *config) {
	unsigned int num_nodes = get_num_nodes();

	int iterations_nr = config->iteration_nr;
	printf("{");
	printf("\"read_node\":%d,", results->read_node);
	printf("\"buffer_node\":%d,", results->buffer_node);
	printf("\"pagecache_node\":%d ,", results->pagecache_node);

	printf("\"%s\":", "buffer_nodes");
	buffer_array_to_json(results->buffer_nodes, iterations_nr, num_nodes,
	                     false);

	printf("\"%s\":", "read_nodes");
	uint_array_to_json(results->read_nodes, iterations_nr, false);

	printf("\"%s\":", "times_us");
	double_array_to_json(results->times_us, iterations_nr, true);

	printf("}");
}
