#include <stddef.h>

void config_module(void *buf, int pagecache_node, int pid, size_t size);
void start_module();
void pause_module();
void force_log_module();
