#ifndef JSONNUMABENCH
#define JSONNUMABENCH

#include <stdbool.h>
#include "numabench.h"

void uint_array_to_json(unsigned int *array, unsigned int len,
                        bool ending);

void double_array_to_json(double *array, unsigned int len,
                          bool ending);

void buffer_array_to_json(unsigned int *array, unsigned int length,
                          unsigned int max_numa_nodes, bool ending);

void results_to_json(struct Results *results, struct Config *config);
#endif
