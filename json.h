#ifndef JSONNUMABENCH
#define JSONNUMABENCH

#include <stdbool.h>
#include "numabench.h"

void uint_array_to_json(char *string, unsigned int *array, unsigned int len,
                        bool ending);

void double_array_to_json(char *string, double *array, unsigned int len,
                          bool ending);

void results_to_json(struct Results *results, struct Config *config);
#endif
