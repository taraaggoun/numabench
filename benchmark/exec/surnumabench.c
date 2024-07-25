#include <stdio.h>
#include <stdlib.h>
#include <numa.h>
#include <sched.h>
#include <unistd.h>

// Fonction pour exécuter une commande shell
void execute_command(const char *command) {
    int ret = system(command);
    if (ret != 0) {
        perror("system");
    }
}

// Fonction pour définir l'affinité de la CPU sur un nœud spécifique
void set_cpu_affinity(int node) {
    cpu_set_t cpu_set;
    CPU_ZERO(&cpu_set);

    // Ajout des CPU du nœud spécifié au set
    struct bitmask *bm = numa_allocate_cpumask();
    numa_node_to_cpus(node, bm);
    for (int i = 0; i < bm->size; i++) {
        if (numa_bitmask_isbitset(bm, i)) {
            CPU_SET(i, &cpu_set);
        }
    }
    numa_free_cpumask(bm);

    // Appliquer l'affinité de la CPU
    if (sched_setaffinity(0, sizeof(cpu_set_t), &cpu_set) != 0) {
        perror("sched_setaffinity");
    }
}

void setaffinity_any() {
	if (numa_run_on_node(-1) < 0) {
		perror("numa_run_on_node");
		exit(1);
	}
}

int main() {
    // Vérifier si le système supporte NUMA
    if (numa_available() < 0) {
        fprintf(stderr, "NUMA not supported on this system.\n");
        return 1;
    }

    for (int i = 1; i <= 10; i++) {
        // Local
        set_cpu_affinity(0);
        setaffinity_any();
        execute_command("./regenerate_pagecache");
        execute_command("./numabench -o read -m thread -i 1000 -t 0 -p 0 -f testfile >> media/LL_3");
        
        // Distant
        set_cpu_affinity(1);
        setaffinity_any();
        execute_command("./regenerate_pagecache");
        execute_command("./numabench -o read -m thread -i 1000 -t 0 -p 1 -f testfile >> media/DL_3");
    }
    return 0;
}
