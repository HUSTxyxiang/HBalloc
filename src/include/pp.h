#ifndef PP_H
#define PP_H

#include <time.h>
#include <stdint.h>

typedef uint64_t oid_t;

typedef oid_t PP;

typedef struct{
    PP pp;
    void *va;
}PP2VA_PAIR;

oid_t get_oid(void);
PP getPP(void *va);
void freePP(PP pp);
void *pp2va(PP pp);

#endif