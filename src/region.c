#include "include/region.h"
#include "include/cas.h"
#include "include/hash.h"
#include "include/tcache.h"
#include "include/config.h"

extern void *RegionBaseAddr;
extern size_t REGION_NUM;

struct RegionHeader *occupyAregion(struct TCACHE *tc)
{
    pthread_t tid = tc->occupied;
    uint64_t h1 = tc->h1;
    struct RegionHeader *prh = (struct RegionHeader *)(RegionBaseAddr + h1 * REGION_SIZE);
    if(CAS((volatile uint64_t*)&(prh->occupied), 0, tid) == true)
        return prh;
    
    int h2 = tc->h2;
    prh = (struct RegionHeader *)(RegionBaseAddr + h2 * REGION_SIZE);
    if(CAS((volatile uint64_t*)&(prh->occupied), 0, tid) == true)
        return prh;
    
    int h3 = tc->h3;
    prh = (struct RegionHeader *)(RegionBaseAddr + h3 * REGION_SIZE);
    if(CAS((volatile uint64_t*)&(prh->occupied), 0, tid) == true)
        return prh;
    
    while(true){
        h3 = (h3 + 1) % REGION_NUM;
        prh = (struct RegionHeader *)(RegionBaseAddr + h3 * REGION_SIZE);
        if(CAS((volatile uint64_t*)&(prh->occupied), 0, tid) == true)
            return prh;
    }
}


void releaseAregion(struct RegionHeader *r)
{
    r->occupied = 0;
}
