#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>

#define MAX_SLEEP 5

void ParentProcess(volatile int []);
void ChildProcess(volatile int []);

int main() {
    int ShmID;
    volatile int *ShmPTR;     // volatile added for strict alternation correctness
    pid_t pid;
    int status;

    srand(time(NULL));

    // Request shared memory for BankAccount and Turn
    ShmID = shmget(IPC_PRIVATE, 2 * sizeof(int), IPC_CREAT | 0666);
    if (ShmID < 0) {
        printf("*** shmget error ***\n");
        exit(1);
    }

    ShmPTR = (int *) shmat(ShmID, NULL, 0);
    if ((long)ShmPTR == -1) {
        printf("*** shmat error ***\n");
        exit(1);
    }

    // Initialize shared variables: BankAccount = 0, Turn = 0
    ShmPTR[0] = 0;
    ShmPTR[1] = 0;

    // Fork the child
    pid = fork();

    if (pid < 0) {
        printf("*** fork error ***\n");
        exit(1);
    }
    else if (pid == 0) {
        ChildProcess(ShmPTR);
        exit(0);
    }
    else {
        ParentProcess(ShmPTR);
    }

    wait(&status);

    shmdt((void *)ShmPTR);
    shmctl(ShmID, IPC_RMID, NULL);

    return 0;
}

// -------------------------------------------------------------
// Parent Process — "Dear Old Dad"
// -------------------------------------------------------------
void ParentProcess(volatile int SharedMem[]) {
    int account, balance;

    for (int i = 0; i < 25; i++) {

        sleep(rand() % (MAX_SLEEP + 1));

        // Copy shared BankAccount into local variable
        account = SharedMem[0];

        // -------------------------------------------------
        // Strict Alternation:
        // Wait while it is NOT the parent's turn (Turn != 0)
        // Small usleep added to avoid CPU spin
        // -------------------------------------------------
        while (SharedMem[1] != 0) {
            usleep(100);
        }

        // Deposit logic
        if (account <= 100) {
            balance = rand() % 100;

            if (balance % 2 == 0) {
                account += balance;
                printf("Dear old Dad: Deposits $%d / Balance = $%d\n",
                       balance, account);
            }
            else {
                printf("Dear old Dad: Doesn't have any money to give\n");
            }
        }
        else {
            printf("Dear old Dad: Thinks Student has enough Cash ($%d)\n",
                   account);
        }

        // Copy updated value back to shared memory
        SharedMem[0] = account;

        // Give turn to Child
        SharedMem[1] = 1;
    }
}

// -------------------------------------------------------------
// Child Process — "Poor Student"
// -------------------------------------------------------------
void ChildProcess(volatile int SharedMem[]) {
    int account, need;

    for (int i = 0; i < 25; i++) {

        sleep(rand() % (MAX_SLEEP + 1));

        // Copy shared BankAccount into a local variable
        account = SharedMem[0];

        // -------------------------------------------------
        // Strict Alternation:
        // Wait while it is NOT the child's turn (Turn != 1)
        // Small usleep avoids high CPU usage
        // -------------------------------------------------
        while (SharedMem[1] != 1) {
            usleep(100);
        }

        need = rand() % 50;
        printf("Poor Student needs $%d\n", need);

        if (need <= account) {
            account -= need;
            printf("Poor Student: Withdraws $%d / Balance = $%d\n",
                   need, account);
        }
        else {
            printf("Poor Student: Not Enough Cash ($%d)\n", account);
        }

        // Copy updated account back to shared memory
        SharedMem[0] = account;

        // Give turn back to Parent
        SharedMem[1] = 0;
    }
}
