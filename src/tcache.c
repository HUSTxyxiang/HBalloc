#include "include/tcache.h"
#include "include/cas.h"
#include "include/hash.h"
#include "include/persist.h"
#include <stdio.h>

extern void *TcacheBaseAddr;

extern size_t REGION_NUM;

struct TCACHE *occupyTcache(pthread_t tid)
{
    struct TCACHE *tc = (struct TCACHE *)TcacheBaseAddr;
    int h = hash1(tid, MAX_THREAD);
    while(CAS((uint64_t*)&(tc[h].occupied), 0, tid) == false)
        h = (h + 1) % MAX_THREAD;
    tc[h].h1 = hash1(tid, REGION_NUM);
    tc[h].h2 = hash2(tid, REGION_NUM);
    tc[h].h3 = hash3(tid, REGION_NUM);
    //printf("Thread %lu occupied Tcache %d\n", tid, h);
    return &tc[h];
}

struct TCACHE *findTcache(pthread_t tid)
{
    struct TCACHE *tc = (struct TCACHE *)TcacheBaseAddr;
    int h1 = hash1(tid, MAX_THREAD);
    if(tc[h1].occupied == tid)
        return &tc[h1];
    int h2 = (h1 + 1) % MAX_THREAD;
    while(h2 != h1){
        if(tc[h2].occupied == tid)
            return &tc[h2];
        h2 = (h2 + 1) % MAX_THREAD;
    }
    return NULL;
}