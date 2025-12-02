#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>

#define NUM_THREADS 10

int MAX; // matrix size
int matA[20][20]; 
int matB[20][20]; 

int matSumResult[20][20];
int matDiffResult[20][20]; 
int matProductResult[20][20]; 

typedef struct {
    int start_row;
    int end_row;
} ThreadData;

void fillMatrix(int matrix[20][20]) {
    for(int i = 0; i<MAX; i++) {
        for(int j = 0; j<MAX; j++) {
            matrix[i][j] = rand()%10+1;
        }
    }
}

void printMatrix(int matrix[20][20]) {
    for(int i = 0; i<MAX; i++) {
        for(int j = 0; j<MAX; j++) {
            printf("%5d", matrix[i][j]);
        }
        printf("\n");
    }
    printf("\n");
}

// Thread function for addition
void* computeSum(void* args) {
    ThreadData *data = (ThreadData*)args;
    for(int i = data->start_row; i < data->end_row; i++) {
        for(int j = 0; j < MAX; j++) {
            matSumResult[i][j] = matA[i][j] + matB[i][j];
        }
    }
    pthread_exit(NULL);
}

// Thread function for subtraction
void* computeDiff(void* args) {
    ThreadData *data = (ThreadData*)args;
    for(int i = data->start_row; i < data->end_row; i++) {
        for(int j = 0; j < MAX; j++) {
            matDiffResult[i][j] = matA[i][j] - matB[i][j];
        }
    }
    pthread_exit(NULL);
}

// Thread function for matrix multiplication (dot product)
void* computeProduct(void* args) {
    ThreadData *data = (ThreadData*)args;
    for(int i = data->start_row; i < data->end_row; i++) {
        for(int j = 0; j < MAX; j++) {
            matProductResult[i][j] = 0;
            for(int k = 0; k < MAX; k++) {
                matProductResult[i][j] += matA[i][k] * matB[k][j];
            }
        }
    }
    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    if(argc < 2) {
        printf("Usage: %s <matrix_size>\n", argv[0]);
        return 1;
    }

    MAX = atoi(argv[1]);
    if(MAX > 20 || MAX < 1) {
        printf("Matrix size must be between 1 and 20\n");
        return 1;
    }

    srand(time(0));
    fillMatrix(matA);
    fillMatrix(matB);

    printf("Matrix A:\n");
    printMatrix(matA);
    printf("Matrix B:\n");
    printMatrix(matB);

    pthread_t threads[NUM_THREADS];
    ThreadData threadData[NUM_THREADS];
    int rows_per_thread = (MAX + NUM_THREADS - 1)/NUM_THREADS; // ceiling division

    // --- Addition ---
    for(int t = 0; t < NUM_THREADS; t++) {
        threadData[t].start_row = t * rows_per_thread;
        threadData[t].end_row = (t+1)*rows_per_thread;
        if(threadData[t].end_row > MAX) threadData[t].end_row = MAX;
        pthread_create(&threads[t], NULL, computeSum, (void*)&threadData[t]);
    }
    for(int t = 0; t < NUM_THREADS; t++) pthread_join(threads[t], NULL);

    // --- Subtraction ---
    for(int t = 0; t < NUM_THREADS; t++) {
        pthread_create(&threads[t], NULL, computeDiff, (void*)&threadData[t]);
    }
    for(int t = 0; t < NUM_THREADS; t++) pthread_join(threads[t], NULL);

    // --- Multiplication ---
    for(int t = 0; t < NUM_THREADS; t++) {
        pthread_create(&threads[t], NULL, computeProduct, (void*)&threadData[t]);
    }
    for(int t = 0; t < NUM_THREADS; t++) pthread_join(threads[t], NULL);

    printf("Results:\n");
    printf("Sum:\n");
    printMatrix(matSumResult);
    printf("Difference:\n");
    printMatrix(matDiffResult);
    printf("Product:\n");
    printMatrix(matProductResult);

    return 0;
}
