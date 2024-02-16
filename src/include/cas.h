#ifndef CAS_H
#define CAS_H 1

#include <stdbool.h>
#include <stdint.h>

bool CAS(
    volatile uint64_t *addr,
    uint64_t old,
    uint64_t newValue
    );

#endif