#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DIM 3
#define MAXINT 10

void print_array(char *name, int array[DIM][DIM]) {
    int i, j;

    printf("%s\n", name);
    for (i = 0; i < DIM; i++) {
        for (j = 0; j < DIM; j++) {
            printf("%4d ", array[i][j]); 
        }
        printf("\n");
    }
    printf("\n");
}


int main(int argc, char *argv[]) {
    int i, j, k, sum;

    int first[DIM][DIM];
    int second[DIM][DIM];
    int result[DIM][DIM];

    for (i = 0; i < DIM; i++) {
        for (j = 0; j < DIM; j++) {
            first[i][j] = rand() % MAXINT;
            second[i][j] = rand() % MAXINT;
        }
    }

    for (i = 0; i < DIM; i++) {
        for (j = 0; j < DIM; j++) {
            sum = 0;
            for (k = 0; k < DIM; k++) {
                sum = sum + first[i][k] * second[k][j];
            }
            result[i][j] = sum;
        }
    }

    if (argc > 1 && strcmp(argv[1], "--verbose") == 0) {
        print_array("first", first);
        print_array("second", second);
        print_array("result", result);
    }
}
