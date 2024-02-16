#ifndef TCACHE_H
#define TCACHE_H 1

#include "sizeclass.h"
#include "chunk.h"
#include <pthread.h>

struct TCACHE{
    
    pthread_t occupied;
    
    uint64_t h1;
    uint64_t h2;
    uint64_t h3;

    struct{
        struct ChunkHeader Head;
        struct ChunkHeader Tail;
    }partialChunkLists[SIZECLASS_NUM];
    uint64_t partialChunksNum[SIZECLASS_NUM];

    struct{
        struct ChunkHeader Head;
        struct ChunkHeader Tail;
    }freeChunkList;
    uint64_t freeChunksNum;

    char alignment[1560];
};  // 8 KB

struct TCACHE *occupyTcache(pthread_t tid);
struct TCACHE *findTcache(pthread_t tid);

#endif