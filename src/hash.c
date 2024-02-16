#include "include/hash.h"
#include <stdlib.h>
#include <time.h>
#include <stdio.h>

uint64_t hash1(uint64_t key, uint64_t MOD)
{
    while((key & 0x1) == 0)
        key = (key >> 1);
    key = (~key) + (key << 21);
    key = key ^ (key >> 24);
    key = (key + (key << 3)) + (key << 8);
    key = key ^ (key >> 14);
    key = (key + (key << 2)) + (key << 4);
    key = key ^ (key >> 28);
    key = key + (key << 31);
    return key % MOD;
}

uint64_t hash2(uint64_t a, uint64_t MOD)
{
    while((a & 0x1) == 0)
        a = (a >> 1);
    a = (a+0x7ed55d16) + (a<<12);
    a = (a^0xc761c23c) ^ (a>>19);
    a = (a+0x165667b1) + (a<<5);
    a = (a+0xd3a2646c) ^ (a<<9);
    a = (a+0xfd7046c5) + (a<<3);
    a = (a^0xb55a4f09) ^ (a>>16); 
    return a % MOD;
}

uint64_t hash3(uint64_t key, uint64_t MOD)
{
    while((key & 0x1) == 0)
        key = (key >> 1);
    key = (~key) + (key << 18);
    key = key ^ (key >> 31);
    key = key * 21;
    key = key ^ (key >> 11);
    key = key + (key << 6);
    key = key ^ (key >> 22);
    return key % MOD;
}