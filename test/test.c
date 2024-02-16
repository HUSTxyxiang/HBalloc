#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <time.h>
#include <pthread.h>
#include "../src/include/HBalloc.h"
#include <stdint.h>
#include <pthread.h>

#define MAXT 128
#define MAXN 1000000

pthread_t tid[MAXT];
PP pointers[MAXT][MAXN];


int SIZE;
int ITR;
int N;
int T;

double GetTimeInSeconds(void)
{
    struct timeval t;
    gettimeofday(&t, NULL);
    return (double)t.tv_sec + (double)t.tv_usec / 1000000.0;
}

void *test(void *arg)
{
    int n = N / T;
    int id = *(int*)arg;
    for(int itr = 0; itr < ITR; itr++){
        for(int i=0; i<n; i++)
            pointers[id][i] = hballoc_malloc(SIZE);
        for(int i=0; i<n; i++)
            hballoc_free(pointers[id][i], SIZE);
    }
    return NULL;
}

int main(int argc, char *argv[])
{
    if(argc != 5){
        printf("Usage: %s SIZE NOBJ ITR T\n", argv[0]);
        exit(0);
    }

    SIZE = atoi(argv[1]);
    N = atoi(argv[2]);
    ITR = atoi(argv[3]);
    T = atoi(argv[4]);
    
    if(T > MAXT || N / T > MAXN){
        printf("Too large NOBJ or T!\n");
        exit(0);
    }

    printf("Allocating %d %d-byte objects with %d working threads, repeating for %d iterations.\n",
        N, SIZE, T, ITR
    );

    int *id = malloc(T*sizeof(int));
    for(int i=0; i < T; i++)
      id[i] = i;
    
    hballoc_start("/mnt/pmem0/HBalloc", 32 * 1024 * 1024 * 1024ULL);
    
    double t1 = GetTimeInSeconds();
    
    for(int i = 0; i < T; i++)
        pthread_create( &tid[i], NULL, test, &id[i]);
    
    for(int i = 0; i < T; i++)
        pthread_join( tid[i], NULL);

    hballoc_exit();

    double t2 = GetTimeInSeconds();
    
    printf("Time = %.2f s\nThroughput = %.2f Mops/s\n", t2-t1, N * ITR / 1000000.0 / (t2-t1));

    return 0;
}