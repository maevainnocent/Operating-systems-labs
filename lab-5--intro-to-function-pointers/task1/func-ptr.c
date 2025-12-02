#include <stdio.h>
#include <stdlib.h>
#include "process.h"

/**
 * my_comparer function for qsort
 * Sorts processes by:
 *   1. Priority descending (higher number = higher priority)
 *   2. Arrival time ascending (earlier arrival first) in case of tie
 */
int my_comparer(const void *a, const void *b) {
    const Process *p1 = (const Process *)a;
    const Process *p2 = (const Process *)b;

    // First, compare priority descending
    if (p2->priority != p1->priority) {
        return p2->priority - p1->priority;
    }

    // If priority equal, compare arrival time ascending
    return p1->arrival_time - p2->arrival_time;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <input_file>\n", argv[0]);
        return 1;
    }

    // Open CSV file
    FILE *f = fopen(argv[1], "r");
    if (!f) {
        perror("Error opening file");
        return 1;
    }

    // Parse processes from file
    Process *plist = parse_file(f);
    fclose(f);

    // Sort the processes using the my_comparer function
    qsort(plist, P_SIZE, sizeof(Process), my_comparer);

    // Print sorted processes
    for (int i = 0; i < P_SIZE; i++) {
        printf("%d (%d, %d)\n", plist[i].pid, plist[i].arrival_time, plist[i].priority);
    }

    free(plist);
    return 0;
}

