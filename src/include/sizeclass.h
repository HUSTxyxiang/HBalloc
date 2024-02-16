#ifndef SIZECLASS_H
#define SIZECLASS_H 1

#include <sys/types.h>

#define SIZECLASS_NUM 24

int getSC(size_t size);

size_t getSize(int sc);

#endif