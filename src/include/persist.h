#ifndef PERSIST_H
#define PERSIST_H 1

#include <sys/types.h>

void clflush(const void *ptr);

void clflush_range(const void *ptr, size_t len);

void clflushopt(const void *ptr);

void clflushopt_range(const void *ptr, size_t len);

void clwb(const void *ptr);

void clwb_range(const void *ptr, size_t len);

void sfence();

#endif