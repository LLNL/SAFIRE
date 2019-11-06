#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <stdint.h>
#include <omp.h>

//void doInject(uint64_t *ret) __attribute__((preserve_all));
//void doInject(uint64_t *ret) __attribute__((0));

int init = 0;
void doInject(uint64_t *ret)
{
    *ret = 0;
    int nthreads = omp_get_num_threads();
    /*if(nthreads > 1) {
        int thread_no = omp_get_thread_num();
        if(thread_no == 4) {
            printf("fi-thread %d/%d\n", thread_no, nthreads);
            *ret = 1;
        }
    }*/
#if 0
    //return;
    if(!init) {
        srandom(time(0));
        init = 1;
    }
    if(random()%1000000 == 0)
        *ret = 1;
#endif
    //printf("doInject: %llu\n", *ret);
}

