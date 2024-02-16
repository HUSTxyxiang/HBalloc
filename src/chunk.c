#include "include/chunk.h"
#include "include/hash.h"
#include "include/cas.h"
#include "include/persist.h"
#include <stdio.h>

extern void *RegionBaseAddr;

struct ChunkHeader *allocateChunks(
    struct ChunkHeader *Head,
    struct ChunkHeader *Tail,
    uint64_t *freeChunksNum, 
    int N,
    struct LOG *log
)
{
    if(N > *freeChunksNum){
        printf("Error: A region is exhausted!\n");
        exit(0);
    }

    struct ChunkHeader *p = Head->nextChunk;
    while(p != Tail && p->contiguousChunksNum < N)
        p = p->nextChunk;
    
    if(p == Tail){
        printf("Error: Failed to find %d contiguous chunks in a region!\n", N);
        exit(0);
    }
    
    if(p->contiguousChunksNum == N){
        struct ChunkHeader *pp = p->priorChunk;
        struct ChunkHeader *pn = p->nextChunk;
        LOG_CHUNK_REMOVE(log, p, pp, pn);
        TX_BEGIN(log);
        pp->nextChunk = pn;
        clwb_range(&(pp->nextChunk), 8);
        pn->priorChunk = pp;
        clwb_range(&(pn->priorChunk), 8);
        *freeChunksNum -= N;
        clwb_range(freeChunksNum, 8);
        sfence();
        TX_COMMIT(log);
        return p;
    }

    else{
        void *res = ((void *)p) +  (p->contiguousChunksNum - N) * CHUNK_SIZE;
        p->contiguousChunksNum -= N;
        clwb_range(&(p->contiguousChunksNum), 8);
        *freeChunksNum -= N;
        clwb_range(freeChunksNum, 8);
        return (struct ChunkHeader *)res;
    }
}

void freeChunks(
    struct ChunkHeader *Head,
    struct ChunkHeader *Tail,
    uint64_t *freeChunksNum,
    struct ChunkHeader *firstChunk,
    int N,
    struct LOG *log
)
{
    if(Head->nextChunk == Tail){
        LOG_CHUNK_INSERT(log, firstChunk, Head, Tail);
        TX_BEGIN(log);
        Head->nextChunk = firstChunk;
        clwb_range(&(Head->nextChunk), 8);
        Tail->priorChunk = firstChunk;
        clwb_range(&(Tail->priorChunk), 8);
        firstChunk->priorChunk = Head;
        firstChunk->nextChunk = Tail;
        firstChunk->contiguousChunksNum = N;
        clwb_range(firstChunk, 24);
        *freeChunksNum += N;
        clwb_range(freeChunksNum, 8);
        sfence();
        TX_COMMIT(log);
        return ;
    }

    if((void *)firstChunk < (void *)(Head->nextChunk)){
        if((void *)firstChunk + N * CHUNK_SIZE == (void *)(Head->nextChunk)){
            struct ChunkHeader *p = Head->nextChunk;
            struct ChunkHeader *pn = p->nextChunk;
            LOG_CHUNK_INSERT(log, firstChunk, Head, pn);
            TX_BEGIN(log);
            firstChunk->contiguousChunksNum = N + p->contiguousChunksNum;
            firstChunk->nextChunk = pn;
            firstChunk->priorChunk = Head;
            clwb_range(firstChunk, 24);
            Head->nextChunk = firstChunk;
            clwb_range(&(Head->nextChunk), 8);
            pn->priorChunk = firstChunk;
            clwb_range(&(pn->priorChunk), 8);
            sfence();
            TX_COMMIT(log);
        }
        else{
            struct ChunkHeader *p = Head->nextChunk;
            LOG_CHUNK_INSERT(log, firstChunk, Head, p);
            TX_BEGIN(log);
            firstChunk->contiguousChunksNum = N;
            firstChunk->nextChunk = p;
            firstChunk->priorChunk = Head;
            clwb_range(firstChunk, 24);
            Head->nextChunk = firstChunk;
            clwb_range(&(Head->nextChunk), 8);
            p->priorChunk = firstChunk;
            clwb_range(&(p->priorChunk), 8);
            sfence();
            TX_COMMIT(log);
        }
    }

    else if((void *)firstChunk > (void *)(Tail->priorChunk)){
        if( ((void *)(Tail->priorChunk)) + (Tail->priorChunk->contiguousChunksNum) * CHUNK_SIZE 
            == ((void *)firstChunk) )
        {
            Tail->priorChunk->contiguousChunksNum += N;
            clwb_range(&(Tail->priorChunk->contiguousChunksNum), 8);
            sfence();
        }
        else{
            struct ChunkHeader *p = Tail->priorChunk;
            LOG_CHUNK_INSERT(log, firstChunk, p, Tail);
            TX_BEGIN(log);
            firstChunk->contiguousChunksNum = N;
            firstChunk->priorChunk = p;
            firstChunk->nextChunk = Tail;
            clwb_range(firstChunk, 24);
            p->nextChunk = firstChunk;
            clwb_range(&(p->nextChunk), 8);
            Tail->priorChunk = firstChunk;
            clwb_range(&(Tail->priorChunk), 8);
            sfence();
            TX_COMMIT(log);
        }
    }

    else{
        struct ChunkHeader *p1 = Head->nextChunk;
        struct ChunkHeader *p2 = p1->nextChunk;
        while(true){
            if((void *)p1 < (void *)firstChunk && (void *)firstChunk < (void *)p2)
                break;
            p1 = p2;
            p2 = p2->nextChunk;
        }
    
        bool ContinousPrev = ((void *)p1) + p1->contiguousChunksNum * CHUNK_SIZE == (void *)firstChunk;
        bool ContinousNext = ((void *)firstChunk) + N * CHUNK_SIZE == (void *)p2;

        if( !ContinousPrev && !ContinousNext ){
            LOG_CHUNK_INSERT(log, firstChunk, p1, p2);
            TX_BEGIN(log);
            firstChunk->contiguousChunksNum = N;
            firstChunk->priorChunk = p1;
            firstChunk->nextChunk = p2;
            clwb_range(firstChunk, 24);
            p1->nextChunk = firstChunk;
            clwb_range(&(p1->nextChunk), 8);
            p2->priorChunk = firstChunk;
            clwb_range(&(p2->priorChunk), 8);
            sfence();
            TX_COMMIT(log);
        }
        
        else if( ContinousPrev && !ContinousNext ){
            p1->contiguousChunksNum += N;
            clwb_range(&(p1->contiguousChunksNum), 8);
            sfence();
        }

        else if( !ContinousPrev && ContinousNext ){
            struct ChunkHeader *p2n = p2->nextChunk;
            LOG_CHUNK_INSERT(log, firstChunk, p1, p2n);
            TX_BEGIN(log);
            firstChunk->contiguousChunksNum = N + p2->contiguousChunksNum;
            firstChunk->nextChunk = p2n;
            firstChunk->priorChunk = p1;
            clwb_range(firstChunk, 24);
            p1->nextChunk = firstChunk;
            clwb_range(&(p1->nextChunk), 8);
            p2n->priorChunk = firstChunk;
            clwb_range(&(p2n->priorChunk), 8);
            sfence();
            TX_COMMIT(log);
        }

        else{
            struct ChunkHeader *p2n = p2->nextChunk;
            LOG_CHUNK_INSERT(log, firstChunk, p1, p2n);
            TX_BEGIN(log);
            p1->contiguousChunksNum += N + p2->contiguousChunksNum;
            p1->nextChunk = p2n;
            clwb_range(&(p1->nextChunk), 16);
            p2n->priorChunk = p1;
            clwb_range(&(p2n->priorChunk), 8);
            sfence();
            TX_COMMIT(log);
        }
    }

    *freeChunksNum += N;
    clwb_range(freeChunksNum, 8);
    sfence();
}
