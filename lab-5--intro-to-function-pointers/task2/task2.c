#include <stdio.h>
#include <stdlib.h>

/* Operation functions */
int add(int a, int b) { printf("Adding 'a' and 'b'\n"); return a + b; }
int subtract(int a, int b) { printf("Subtracting 'b' from 'a'\n"); return a - b; }
int multiply(int a, int b) { printf("Multiplying 'a' and 'b'\n"); return a * b; }
int divide(int a, int b) { 
    int safe_b = b + (b == 0);  /* Prevent division by zero */
    printf("Dividing 'a' by 'b'\n"); 
    return a / safe_b; 
}
int exit_func(int a, int b) { 
    printf("Exiting program...\n"); 
    exit(0); 
    return 0; /* unreachable */ 
}
int invalid(int a, int b) { 
    printf("Invalid choice! Must be between 0 and 4.\n"); 
    return 0; 
}

int main(void) {
    int a = 6, b = 3;
    char choice;

    printf("Operand 'a' : %d | Operand 'b' : %d\n", a, b);
    printf("Specify the operation to perform "
           "(0 : add | 1 : subtract | 2 : Multiply | 3 : divide | 4 : exit): ");

    scanf(" %c", &choice);

    /* Map input char '0'-'4' to index 0-4, else index 5 for invalid */
    int index_table[256];
    for (int i = 0; i < 256; i++) index_table[i] = 5; /* default = invalid */
    index_table['0'] = 0;
    index_table['1'] = 1;
    index_table['2'] = 2;
    index_table['3'] = 3;
    index_table['4'] = 4;

    /* Function pointer array */
    int (*operations[6])(int, int) = { add, subtract, multiply, divide, exit_func, invalid };

    /* Call the operation without any conditionals */
    int result = operations[(unsigned char)index_table[choice]](a, b);

    printf("x = %d\n", result);

    return 0;
}



