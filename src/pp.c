#include "include/config.h"
#include "include/pp.h"
#include "include/hash.h"
#include "include/cas.h"
#include "include/persist.h"
#include <stdio.h>
#include <stdbool.h>

extern void *pp2vaMap;
extern void *mappedAddr;

unsigned long long object_counter = 0;

oid_t get_oid(void)
{
    time_t tm = time(NULL);
    while(true){
        unsigned long long counter = object_counter;
        if(CAS((uint64_t*)&object_counter, counter, counter+1) == true)
            return (tm << 32) | (counter & 0xffffffff);
    }
}


PP getPP(void *va)
{
    if(va == NULL)
        return NPP;

#ifdef PIP    
    oid_t oid = get_oid();
    PP2VA_PAIR *a = pp2vaMap;
    int i = hash1(oid, MAX_OBJ_NUM);
    while(CAS((volatile uint64_t*)&(a[i].pp), 0, oid) == false)
        i = (i+1) % MAX_OBJ_NUM;
    a[i].va = va;
    return oid;
#else
    return va - mappedAddr;
#endif

}

void freePP(PP pp)
{
#ifdef PIP
    PP2VA_PAIR *a = pp2vaMap;
    int i = hash1(pp, MAX_OBJ_NUM);
    while(a[i].pp != pp)
        i = (i+1) % MAX_OBJ_NUM;
    a[i].pp = 0;
    a[i].va = 0;
#endif
}

void *pp2va(PP pp)
{
    if(pp == NPP)
        return NULL;

#ifdef PIP
    PP2VA_PAIR *a = pp2vaMap;
    int i = hash1(pp, MAX_OBJ_NUM);
    while(a[i].pp != pp)
        i = (i+1) % MAX_OBJ_NUM;
    return a[i].va;
#else
    return pp + mappedAddr;
#endif

}