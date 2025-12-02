

#define _XOPEN_SOURCE 700
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <signal.h>
#include <sys/wait.h>
#include <string.h>
#include <errno.h>

#define SHM_NAME "/bank_shm"
#define SEM_NAME "/banksem"

static pid_t *child_pids = NULL;
static int total_children = 0;
static sem_t *mutex = NULL;
static int shm_fd = -1;
static int *BankAccount = NULL;

/* Clean up resources (called by master on SIGINT) */
void cleanup_and_exit(int signo) {
    (void)signo;
    printf("\nMaster: cleaning up, killing child processes...\n");

    // kill all children
    for (int i = 0; i < total_children; ++i) {
        if (child_pids[i] > 0) {
            kill(child_pids[i], SIGTERM);
        }
    }

    // wait for children to exit
    for (int i = 0; i < total_children; ++i) {
        if (child_pids[i] > 0) waitpid(child_pids[i], NULL, 0);
    }

    // semaphore cleanup
    if (mutex != NULL) {
        sem_close(mutex);
        sem_unlink(SEM_NAME);
    }

    // shared memory cleanup
    if (BankAccount != NULL) {
        munmap(BankAccount, sizeof(int));
    }
    if (shm_fd != -1) {
        close(shm_fd);
        shm_unlink(SHM_NAME);
    }

    free(child_pids);
    printf("Master: cleanup complete. Exiting.\n");
    exit(0);
}

/* Sleep for random seconds in [0, max_seconds] */
static void rand_sleep(int max_seconds) {
    int t = rand() % (max_seconds + 1);
    sleep(t);
}

/* Dear Old Dad process */
void dear_old_dad_process() {
    srand(time(NULL) ^ (getpid()<<16));
    while (1) {
        rand_sleep(5); // 0-5 seconds
        printf("Dear Old Dad: Attempting to Check Balance\n");

        int r = rand() % 10; // decide whether to attempt deposit or just check

        sem_wait(mutex);
        int localBalance = *BankAccount;

        if (r % 2 == 0) {
            if (localBalance < 100) {
                int amount = rand() % 100; // 0-99
                if (amount % 2 == 0) {
                    localBalance += amount;
                    *BankAccount = localBalance;
                    printf("Dear old Dad: Deposits $%d / Balance = $%d\n", amount, localBalance);
                } else {
                    printf("Dear old Dad: Doesn't have any money to give\n");
                }
            } else {
                printf("Dear old Dad: Thinks Student has enough Cash ($%d)\n", localBalance);
            }
        } else {
            printf("Dear Old Dad: Last Checking Balance = $%d\n", localBalance);
        }

        sem_post(mutex);
    }
    // never reached
}

/* Lovable Mom process */
void lovable_mom_process() {
    srand(time(NULL) ^ (getpid()<<8));
    while (1) {
        rand_sleep(10); // 0-10 seconds
        printf("Lovable Mom: Attempting to Check Balance\n");

        sem_wait(mutex);
        int localBalance = *BankAccount;

        if (localBalance <= 100) {
            int amount = rand() % 125; // 0-124
            localBalance += amount;
            *BankAccount = localBalance;
            printf("Lovable Mom: Deposits $%d / Balance = $%d\n", amount, localBalance);
        } else {
            // Mom only deposits when <=100; otherwise can just check
            printf("Lovable Mom: Last Checking Balance = $%d\n", localBalance);
        }

        sem_post(mutex);
    }
}

/* Poor Student process */
void poor_student_process(int student_id) {
    srand(time(NULL) ^ (getpid()));
    while (1) {
        rand_sleep(5); // 0-5 seconds
        printf("Poor Student: Attempting to Check Balance\n");

        int r = rand() % 10;

        if (r % 2 == 0) {
            sem_wait(mutex);
            int localBalance = *BankAccount;
            int need = rand() % 50; // 0-49
            printf("Poor Student needs $%d\n", need);

            if (need <= localBalance) {
                localBalance -= need;
                *BankAccount = localBalance;
                printf("Poor Student: Withdraws $%d / Balance = $%d\n", need, localBalance);
            } else {
                printf("Poor Student: Not Enough Cash ($%d)\n", localBalance);
            }
            sem_post(mutex);
        } else {
            // just check last known balance (we'll grab it under mutex to be consistent)
            sem_wait(mutex);
            int localBalance = *BankAccount;
            printf("Poor Student: Last Checking Balance = $%d\n", localBalance);
            sem_post(mutex);
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <num_parents (1 or 2)> <num_students>\n", argv[0]);
        exit(1);
    }

    int num_parents = atoi(argv[1]);
    int num_students = atoi(argv[2]);
    if ((num_parents != 1 && num_parents != 2) || num_students < 1) {
        fprintf(stderr, "Invalid args. num_parents must be 1 or 2. num_students >= 1.\n");
        exit(1);
    }

    srand(time(NULL) ^ getpid());

    /* Install SIGINT handler for cleanup */
    struct sigaction act;
    memset(&act, 0, sizeof(act));
    act.sa_handler = cleanup_and_exit;
    sigaction(SIGINT, &act, NULL);

    /* Create POSIX shared memory object */
    shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open");
        exit(1);
    }
    if (ftruncate(shm_fd, sizeof(int)) == -1) {
        perror("ftruncate");
        shm_unlink(SHM_NAME);
        exit(1);
    }
    BankAccount = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (BankAccount == MAP_FAILED) {
        perror("mmap");
        shm_unlink(SHM_NAME);
        exit(1);
    }
    *BankAccount = 0; // initial balance

    /* Create/open named semaphore */
    mutex = sem_open(SEM_NAME, O_CREAT, 0644, 1);
    if (mutex == SEM_FAILED) {
        perror("sem_open");
        munmap(BankAccount, sizeof(int));
        shm_unlink(SHM_NAME);
        exit(1);
    }

    /* Prepare to store child PIDs: we'll create num_parents + num_students children */
    total_children = num_parents + num_students;
    child_pids = calloc((size_t)total_children, sizeof(pid_t));
    if (!child_pids) {
        perror("calloc");
        cleanup_and_exit(0);
    }

    int idx = 0;

    /* Fork parents: if num_parents == 1, create only Dad; if 2 create Mom and Dad.
       We'll create Dad first, then Mom (order is not critical). */
    for (int p = 0; p < num_parents; ++p) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            cleanup_and_exit(0);
        } else if (pid == 0) {
            // child: run the appropriate parent role
            if (num_parents == 1) {
                // only Dad
                dear_old_dad_process();
            } else {
                // if two parents: p==0 -> Dad, p==1 -> Mom
                if (p == 0) dear_old_dad_process();
                else lovable_mom_process();
            }
            exit(0);
        } else {
            // master stores pid
            child_pids[idx++] = pid;
            printf("Master: spawned parent PID %d (index %d)\n", pid, idx-1);
        }
    }

    /* Fork students */
    for (int s = 0; s < num_students; ++s) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            cleanup_and_exit(0);
        } else if (pid == 0) {
            poor_student_process(s+1);
            exit(0);
        } else {
            child_pids[idx++] = pid;
            printf("Master: spawned student PID %d (index %d)\n", pid, idx-1);
        }
    }

    /* Master waits (sleep) until SIGINT cause cleanup (or just pause) */
    printf("Master: spawned %d child processes. Press Ctrl-C to terminate and cleanup.\n", total_children);
    // keep the master alive
    while (1) {
        pause(); // waits until signal (SIGINT triggers cleanup handler and exit)
    }

    // unreachable
    return 0;
}
