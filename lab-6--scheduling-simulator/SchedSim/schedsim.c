// C program for implementation of Simulation 
#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include "process.h"
#include "util.h"

// Function to find the waiting time for Round Robin
void findWaitingTimeRR(ProcessType plist[], int n, int quantum) 
{ 
    int rem_bt[n];
    for (int i = 0; i < n; i++)
        rem_bt[i] = plist[i].bt;
    
    int t = 0; // Current time
    int done;
    do {
        done = 1;
        for (int i = 0; i < n; i++) {
            if (rem_bt[i] > 0 && plist[i].art <= t) {
                done = 0;
                if (rem_bt[i] > quantum) {
                    t += quantum;
                    rem_bt[i] -= quantum;
                } else {
                    t += rem_bt[i];
                    plist[i].wt = t - plist[i].bt - plist[i].art;
                    rem_bt[i] = 0;
                }
            }
        }
        // If no process was ready, advance time
        if (done) {
            for (int i = 0; i < n; i++) {
                if (rem_bt[i] > 0) {
                    t++;
                    done = 0;
                    break;
                }
            }
        }
    } while (!done);
} 

// Function to find waiting time for SRTF (preemptive SJF)
void findWaitingTimeSJF(ProcessType plist[], int n)
{ 
    int rem_bt[n];
    int complete = 0, t = 0, min_index = -1;
    int min_bt;
    
    for (int i = 0; i < n; i++)
        rem_bt[i] = plist[i].bt;
    
    while (complete < n) {
        min_bt = INT_MAX;
        min_index = -1;
        for (int i = 0; i < n; i++) {
            if (plist[i].art <= t && rem_bt[i] > 0 && rem_bt[i] < min_bt) {
                min_bt = rem_bt[i];
                min_index = i;
            }
        }
        if (min_index == -1) { // No process has arrived yet
            t++;
            continue;
        }
        rem_bt[min_index]--;
        t++;
        if (rem_bt[min_index] == 0) {
            plist[min_index].wt = t - plist[min_index].bt - plist[min_index].art;
            complete++;
        }
    }
} 

// Function to find waiting time for FCFS considering arrival times
void findWaitingTime(ProcessType plist[], int n)
{ 
    int t = 0; // current time
    for (int i = 0; i < n; i++) {
        if (t < plist[i].art)
            t = plist[i].art; // wait for process to arrive
        plist[i].wt = t - plist[i].art;
        t += plist[i].bt;
    }
} 
  
// Function to calculate turn around time 
void findTurnAroundTime(ProcessType plist[], int n)
{ 
    for (int i = 0; i < n; i++)
        plist[i].tat = plist[i].bt + plist[i].wt;
} 
  
// Comparator for Priority Scheduling (Descending order: largest pri first)
int my_comparer(const void *this, const void *that)
{ 
    ProcessType *a = (ProcessType *)this;
    ProcessType *b = (ProcessType *)that;

    if (a->pri != b->pri)
        return b->pri - a->pri; // DESCENDING order
    return a->art - b->art; // tie-breaker: earlier arrival
} 

// Calculate average time FCFS
void findavgTimeFCFS(ProcessType plist[], int n) 
{ 
    findWaitingTime(plist, n); 
    findTurnAroundTime(plist, n); 
    printf("\n*********\nFCFS\n");
}

// Calculate average time SJF
void findavgTimeSJF(ProcessType plist[], int n) 
{ 
    findWaitingTimeSJF(plist, n); 
    findTurnAroundTime(plist, n); 
    printf("\n*********\nSJF (Preemptive/SRTF)\n");
}

// Calculate average time RR
void findavgTimeRR(ProcessType plist[], int n, int quantum) 
{ 
    findWaitingTimeRR(plist, n, quantum); 
    findTurnAroundTime(plist, n); 
    printf("\n*********\nRR Quantum = %d\n", quantum);
}

// Calculate average time Priority
void findavgTimePriority(ProcessType plist[], int n) 
{ 
    qsort(plist, n, sizeof(ProcessType), my_comparer);
    findWaitingTime(plist, n); // now considers arrival times
    findTurnAroundTime(plist, n); 
    printf("\n*********\nPriority\n");
}

// Print metrics
void printMetrics(ProcessType plist[], int n)
{
    int total_wt = 0, total_tat = 0; 
    float awt, att;
    
    printf("\tProcesses\tBurst time\tWaiting time\tTurn around time\n"); 
    for (int i = 0; i < n; i++) {
        total_wt += plist[i].wt; 
        total_tat += plist[i].tat; 
        printf("\t%d\t\t%d\t\t%d\t\t%d\n", plist[i].pid, plist[i].bt, plist[i].wt, plist[i].tat); 
    } 
  
    awt = ((float)total_wt / (float)n);
    att = ((float)total_tat / (float)n);
    
    printf("\nAverage waiting time = %.2f", awt); 
    printf("\nAverage turn around time = %.2f\n", att); 
} 

ProcessType * initProc(char *filename, int *n) 
{
    FILE *input_file = fopen(filename, "r");
    if (!input_file) {
        fprintf(stderr, "Error: Invalid filepath\n");
        fflush(stdout);
        exit(0);
    }
    ProcessType *plist = parse_file(input_file, n);
    fclose(input_file);
    return plist;
}
  
// Driver code 
int main(int argc, char *argv[]) 
{ 
    int n; 
    int quantum = 2;
    ProcessType *proc_list;
  
    if (argc < 2) {
        fprintf(stderr, "Usage: ./schedsim <input-file-path>\n");
        fflush(stdout);
        return 1;
    }
    
    // FCFS
    n = 0;
    proc_list = initProc(argv[1], &n);
    findavgTimeFCFS(proc_list, n);
    printMetrics(proc_list, n);
    free(proc_list);
  
    // SJF
    n = 0;
    proc_list = initProc(argv[1], &n);
    findavgTimeSJF(proc_list, n); 
    printMetrics(proc_list, n);
    free(proc_list);
  
    // Priority
    n = 0; 
    proc_list = initProc(argv[1], &n);
    findavgTimePriority(proc_list, n); 
    printMetrics(proc_list, n);
    free(proc_list);
    
    // RR
    n = 0;
    proc_list = initProc(argv[1], &n);
    findavgTimeRR(proc_list, n, quantum); 
    printMetrics(proc_list, n);
    free(proc_list);
    
    return 0; 
} 
