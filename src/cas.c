#include "include/cas.h"

bool CAS(
    volatile uint64_t *addr,
    uint64_t old,
    uint64_t newValue
    )
{
    return __sync_bool_compare_and_swap(addr, old, newValue);
}