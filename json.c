#include "json.h"
#include "numabench.h"
#include <stdio.h>

void uint_array_to_json(char *string, unsigned int *array, unsigned int length,
                        bool ending) {
	printf("\"%s\":", string);
	printf("[");
	for (int i = 0; i < length - 1; i++) {
		printf("%u", array[i]);
		printf(",");
	}
	printf("%u", array[length - 1]);
	if (ending)
		printf("]");
	else
		printf("],");
}

void double_array_to_json(char *string, double *array, unsigned int length,
                          bool ending) {
	printf("\"%s\":", string);
	printf("[");
	for (int i = 0; i < length - 1; i++) {
		printf("%f", array[i]);
		printf(",");
	}
	printf("%f", array[length - 1]);
	if (ending)
		printf("]");
	else
		printf("],");
}

void results_to_json(struct Results *results, struct Config *config) {
	int iterations_nr = config->iteration_nr;
	printf("{");
	printf("\"read_node\":%d,", results->read_node);
	printf("\"buffer_node\":%d,", results->buffer_node);
	printf("\"pagecache_node\":%d ,", results->pagecache_node);

	uint_array_to_json("buffer_nodes", results->buffer_nodes, iterations_nr,
	                   false);

	uint_array_to_json("read_nodes", results->read_nodes, iterations_nr, false);

	double_array_to_json("times_ms", results->times_ms, iterations_nr, true);

	printf("}");
}
