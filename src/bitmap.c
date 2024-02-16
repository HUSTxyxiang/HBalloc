#include "include/bitmap.h"

void clearBit(uint8_t *a, int i)
{
    int j = i >> 3;
    int k = i & 0x7;
    a[j] = (a[j] & (~(1 << k)));
}

void setBit(uint8_t *a, int i)
{
    int j = i >> 3;
    int k = i & 0x7;
    a[j] = (a[j] | (1 << k));
}

int getBit(uint8_t *a, int i)
{
    int j = i >> 3;
    int k = i & 0x7;
    return (a[j] >> k) & 0x1;
}