#include <stdio.h>
#include <omp.h>

int main() {
    printf("Hello OpenMP\n");
    #pragma omp parallel
    {
        printf("Thread %d\n", omp_get_thread_num());
    }
    return 0;
}
