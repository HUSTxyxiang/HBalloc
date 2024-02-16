#include "include/persist.h"
#include "include/config.h"

void clflush(const void *ptr) {
    asm volatile("clflush %0" : "+m" (ptr));
}

void clflush_range(const void *ptr, size_t len) {
    unsigned long long start = (((unsigned long long)ptr) >> 6) << 6;
    for (; start < (unsigned long long)ptr + len; start += CACHELINE_SIZE) {
        clflush((void*)start);
    }
}

void clflushopt(const void *ptr) {
    asm volatile("clflushopt %0" : "+m" (ptr));
}

void clflushopt_range(const void *ptr, size_t len) {
    unsigned long long start = (((unsigned long long)ptr) >> 6) << 6;
    for (; start < (unsigned long long)ptr + len; start += CACHELINE_SIZE) {
        clflushopt((void*)start);
    }
}

void clwb(const void *ptr) {
    asm volatile("clwb %0" : "+m" (ptr));
}

void clwb_range(const void *ptr, size_t len) {
    unsigned long long start = (((unsigned long long)ptr) >> 6) << 6;
    for (; start < (unsigned long long)ptr + len; start += CACHELINE_SIZE) {
        clwb((void*)start);
    }
}

void sfence()
{
    asm volatile("sfence":::"memory");
}