#ifndef BITMAP_H
#define BITMAP_H 1

#include <sys/types.h>
#include <stddef.h>
#include <stdint.h>
#include <unistd.h>

void clearBit(uint8_t *a, int i);
void setBit(uint8_t *a, int i);
int getBit(uint8_t *a, int i);

#endif