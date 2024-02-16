#ifndef CHUNK_H
#define CHUNK_H 1

#include <stdint.h>
#include <pthread.h>
#include "config.h"
#include "log.h"

struct VHeader{
    uint8_t bitmap[40];
    uint64_t freeBlocksNum;
    uint64_t firstFreeBlock;
    uint64_t counter;
};  // 64 bytes

struct ChunkHeader{
    struct ChunkHeader *priorChunk;
    struct ChunkHeader *nextChunk;
    uint64_t contiguousChunksNum;

    uint64_t sizeclass;
    uint64_t blockSize;
    
    uint8_t bitmap[40];
    uint64_t freeBlocksNum;
    uint64_t firstFreeBlock;

    struct VHeader *vhp;

    char alignment[24];
};  // 128 bytes



struct ChunkHeader *allocateChunks(
    struct ChunkHeader *Head,
    struct ChunkHeader *Tail,
    uint64_t *freeChunksNum, 
    int N,
    struct LOG *log
);

void freeChunks(
    struct ChunkHeader *Head,
    struct ChunkHeader *Tail,
    uint64_t *freeChunksNum,
    struct ChunkHeader *firstChunk,
    int N,
    struct LOG *log
);

#endif