#include <stdio.h>
#include "project.h"

void print_simulation_results(const struct Result *res) {
    if (!res) {
        fprintf(stderr, "Error: Null pointer provided to result printer.\n");
        return;
    }

    printf("\n");
    printf("Total Cycles : %u\n", res->cycles); 
    printf("Zero Flags : %u\n", res->zeros); 
    printf("Sign Flags : %u\n", res->signs); 
    printf("Overflow Flags : %u\n", res->overflows); 
    printf("Underflow Flags : %u\n", res->underflows);
    printf("Inexact Flags : %u\n", res->inexacts);
    printf("NaN Flags : %u\n", res->nans);
    printf("\n");
}