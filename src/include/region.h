#ifndef REGION_H
#define REGION_H 1

#include <stddef.h>
#include <sys/types.h>
#include <pthread.h>
#include <stdint.h>
#include "chunk.h"
#include "tcache.h"

struct RegionHeader{
    pthread_t occupied;
    struct ChunkHeader chunkListHead;
    struct ChunkHeader chunkListTail;
    uint64_t freeChunksNum;
    char alignment[CHUNK_SIZE - 272];
};  // 16 KB

struct RegionHeader *occupyAregion(struct TCACHE *tc);
void releaseAregion(struct RegionHeader *r);

#endif