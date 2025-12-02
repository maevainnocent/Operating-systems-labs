#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "list.h"
#include "util.h"

void TOUPPER(char * arr){
    for(int i = 0; arr[i] != '\0'; i++){
        arr[i] = toupper(arr[i]);
    }
}

void get_input(char *args[], int input[][2], int *n, int *size, int *policy)
{
    FILE *input_file = fopen(args[1], "r");
    if (!input_file) {
        fprintf(stderr, "Error: Invalid filepath\n");
        fflush(stdout);
        exit(0);
    }

    parse_file(input_file, input, n, size);
    fclose(input_file);

    TOUPPER(args[2]);

    if ((strcmp(args[2], "-F") == 0) || (strcmp(args[2], "-FIFO") == 0))
        *policy = 1;
    else if ((strcmp(args[2], "-B") == 0) || (strcmp(args[2], "-BESTFIT") == 0))
        *policy = 2;
    else if ((strcmp(args[2], "-W") == 0) || (strcmp(args[2], "-WORSTFIT") == 0))
        *policy = 3;
    else {
        printf("usage: ./mmu <input file> -{F | B | W }  \n(F=FIFO | B=BESTFIT | W-WORSTFIT)\n");
        exit(1);
    }
}

/* Allocate memory according to policy:
 * policy: 1 FIFO (first-fit -> freelist in FIFO)
 *         2 Best-fit (freelist sorted ascending by blocksize)
 *         3 Worst-fit (freelist sorted descending by blocksize)
 */
void allocate_memory(list_t * freelist, list_t * alloclist, int pid, int blocksize, int policy) {
    if (freelist == NULL || alloclist == NULL) return;

    /* Check if any free block can satisfy the request */
    if (!list_is_in_by_size(freelist, blocksize)) {
        printf("Error: Not Enough Memory\n");
        return;
    }

    /* pick the first block in the freelist (the freelist is expected to be kept in
       correct order for Best/Worst/FIFO semantics) */
    int idx = list_get_index_of_by_Size(freelist, blocksize);
    if (idx < 0) {
        printf("Error: Not Enough Memory\n");
        return;
    }

    block_t *blk = list_remove_at_index(freelist, idx);
    if (blk == NULL) {
        printf("Error: Not Enough Memory\n");
        return;
    }

    /* record original end to create fragment if needed */
    int original_end = blk->end;

    /* allocate portion to process */
    blk->pid = pid;
    blk->end = blk->start + blocksize - 1;

    /* add to allocated list sorted by address */
    list_add_ascending_by_address(alloclist, blk);

    /* create fragment if leftover */
    if (blk->end < original_end) {
        block_t *fragment = (block_t*) malloc(sizeof(block_t));
        if (fragment == NULL) {
            fprintf(stderr, "Fatal: malloc failed for fragment\n");
            exit(1);
        }
        fragment->pid = 0;
        fragment->start = blk->end + 1;
        fragment->end = original_end;

        /* insert fragment back to free list according to policy */
        if (policy == 1) {
            list_add_to_back(freelist, fragment);
        } else if (policy == 2) {
            list_add_ascending_by_blocksize(freelist, fragment);
        } else {
            list_add_descending_by_blocksize(freelist, fragment);
        }
    }
}

void deallocate_memory(list_t * alloclist, list_t * freelist, int pid, int policy) {
    if (alloclist == NULL || freelist == NULL) return;

    if (!list_is_in_by_pid(alloclist, pid)) {
        printf("Error: Can't locate Memory Used by PID: %d\n", pid);
        return;
    }

    int idx = list_get_index_of_by_Pid(alloclist, pid);
    if (idx < 0) {
        printf("Error: Can't locate Memory Used by PID: %d\n", pid);
        return;
    }

    block_t *blk = list_remove_at_index(alloclist, idx);
    if (blk == NULL) {
        printf("Error: Can't locate Memory Used by PID: %d\n", pid);
        return;
    }

    blk->pid = 0;

    /* insert back into freelist according to policy */
    if (policy == 1) {
        list_add_to_back(freelist, blk);
    } else if (policy == 2) {
        list_add_ascending_by_blocksize(freelist, blk);
    } else {
        list_add_descending_by_blocksize(freelist, blk);
    }
}

/* Coalesce the free list:
 * - Move all nodes into a temporary list sorted by address
 * - Free the original list struct (nodes/blocks moved)
 * - Coalesce adjacent blocks in temp list
 * - Return the new list
 */
list_t* coalese_memory(list_t * list) {
    if (list == NULL) return NULL;

    list_t *temp_list = list_alloc();
    block_t *blk;

    /* move all blocks into temp_list ordered by address */
    while ((blk = list_remove_from_front(list)) != NULL) {
        list_add_ascending_by_address(temp_list, blk);
    }

    /* free the original list struct (it no longer owns nodes or blocks) */
    free(list);

    /* merge adjacent free blocks in temp_list */
    list_coalese_nodes(temp_list);

    return temp_list;
}

void print_list(list_t * list, char * message){
    node_t *current = (list == NULL) ? NULL : list->head;
    block_t *blk;
    int i = 0;

    printf("%s:\n", message);

    while(current != NULL){
        blk = current->blk;
        printf("Block %d:\t START: %d\t END: %d\t PID: %d\n", i, blk->start, blk->end, blk->pid);
        current = current->next;
        i += 1;
    }
}

/* DO NOT MODIFY - main orchestrates simulation */
int main(int argc, char *argv[])
{
    int PARTITION_SIZE, inputdata[200][2], N = 0, Memory_Mgt_Policy;

    list_t *FREE_LIST = list_alloc();   /* free blocks (pid == 0) */
    list_t *ALLOC_LIST = list_alloc();  /* allocated blocks (pid != 0) */
    int i;

    if(argc != 3) {
        printf("usage: ./mmu <input file> -{F | B | W }  \n(F=FIFO | B=BESTFIT | W-WORSTFIT)\n");
        exit(1);
    }

    get_input(argv, inputdata, &N, &PARTITION_SIZE, &Memory_Mgt_Policy);

    /* create initial partition and add to free list */
    block_t * partition = (block_t*) malloc(sizeof(block_t));
    if (partition == NULL) {
        fprintf(stderr, "Fatal: malloc failed for initial partition\n");
        exit(1);
    }
    partition->start = 0;
    partition->end = PARTITION_SIZE + partition->start - 1;
    partition->pid = 0;

    list_add_to_front(FREE_LIST, partition);

    for(i = 0; i < N; i++) {
        printf("************************\n");
        if(inputdata[i][0] != -99999 && inputdata[i][0] > 0) {
            printf("ALLOCATE: %d FROM PID: %d\n", inputdata[i][1], inputdata[i][0]);
            allocate_memory(FREE_LIST, ALLOC_LIST, inputdata[i][0], inputdata[i][1], Memory_Mgt_Policy);
        }
        else if (inputdata[i][0] != -99999 && inputdata[i][0] < 0) {
            printf("DEALLOCATE MEM: PID %d\n", abs(inputdata[i][0]));
            deallocate_memory(ALLOC_LIST, FREE_LIST, abs(inputdata[i][0]), Memory_Mgt_Policy);
        }
        else {
            printf("COALESCE/COMPACT\n");
            FREE_LIST = coalese_memory(FREE_LIST);
        }

        printf("************************\n");
        print_list(FREE_LIST, "Free Memory");
        print_list(ALLOC_LIST,"\nAllocated Memory");
        printf("\n\n");
    }

    /* free both lists and their blocks */
    list_free(FREE_LIST);
    list_free(ALLOC_LIST);

    return 0;
}
