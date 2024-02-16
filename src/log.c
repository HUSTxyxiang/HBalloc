#include "include/log.h"
#include "include/config.h"
#include "include/persist.h"
#include "include/hash.h"
#include "include/cas.h"


extern void *mappedAddr;
extern void *LastMappedAddr;
extern void *LogBaseAddr;

void LogInit()
{
    struct LOG *log = (struct LOG *)LogBaseAddr;
    for(int i=0; i < MAX_THREAD; i++){
        log[i].occupied = 0;
        log[i].state = LOG_COMMITTED;
        clwb_range((void *)&log[i], 16);
    }
    sfence();
}


struct LOG *occupyLog(pthread_t tid)
{
    struct LOG *logs = (struct LOG *)LogBaseAddr;
    int h = hash1(tid, MAX_THREAD);
    while(CAS((uint64_t*)&(logs[h].occupied), 0, tid) == false)
        h = (h + 1) % MAX_THREAD;
    return &logs[h];
}


struct LOG *getLog(pthread_t tid)
{
    struct LOG *logs = (struct LOG *)LogBaseAddr;
    int h1 = hash1(tid, MAX_THREAD);
    if(logs[h1].occupied == tid)
        return &logs[h1];
    int h2 = (h1 + 1) % MAX_THREAD;
    while(h2 != h1){
        if(logs[h2].occupied == tid)
            return &logs[h2];
        h2 = (h2 + 1) % MAX_THREAD;
    }
    return NULL;  
}


void TX_BEGIN(struct LOG *log)
{
    log->state = LOG_RUNNING;
    clwb_range(&(log->state), 4);
    sfence();
}

void TX_COMMIT(struct LOG *log)
{
    // log->state = LOG_COMMITTED;
    // clwb_range(&(log->state), 4);
    // sfence();
}


void LOG_CHUNK_INSERT(
    struct LOG *log,
    void *chunk,
    void *pChunk,
    void *nChunk
)
{
    log->type = LOG_TYPE_CHUNK_INSERT;
    
    log->logEntrys[0].addr = chunk;
    log->logEntrys[0].len = 24;
    memcpy(log->logEntrys[0].buf, chunk, 24);
    clwb_range((void *)(log->logEntrys[0].buf), 24);
    
    log->logEntrys[0].addr = pChunk;
    log->logEntrys[0].len = 24;
    memcpy(log->logEntrys[0].buf, pChunk, 24);
    clwb_range((void *)(log->logEntrys[0].buf), 24);

    log->logEntrys[0].addr = nChunk;
    log->logEntrys[0].len = 24;
    memcpy(log->logEntrys[0].buf, nChunk, 24);
    clwb_range((void *)(log->logEntrys[0].buf), 24);

    sfence();
}

void LOG_CHUNK_REMOVE(
    struct LOG *log,
    void *chunk,
    void *pChunk,
    void *nChunk
)
{
    log->type = LOG_TYPE_CHUNK_INSERT;
    
    log->logEntrys[0].addr = chunk;
    log->logEntrys[0].len = 24;
    memcpy(log->logEntrys[0].buf, chunk, 24);
    clwb_range((void *)(log->logEntrys[0].buf), 24);
    
    log->logEntrys[0].addr = pChunk;
    log->logEntrys[0].len = 24;
    memcpy(log->logEntrys[0].buf, pChunk, 24);
    clwb_range((void *)(log->logEntrys[0].buf), 24);

    log->logEntrys[0].addr = nChunk;
    log->logEntrys[0].len = 24;
    memcpy(log->logEntrys[0].buf, nChunk, 24);
    clwb_range((void *)(log->logEntrys[0].buf), 24);

    sfence();
}

void LOG_VHEADER_SYNC(struct LOG *log, void *chunk)
{
    log->type = LOG_TYPE_VHEADER_SYNC;

    log->logEntrys[0].addr = chunk + 40;
    log->logEntrys[0].len = 56;
    memcpy((void *)log->logEntrys[0].buf, log->logEntrys[0].addr, 56);
    clwb_range((void *)(log->logEntrys[0].buf), 56);
    
    sfence();
}

void undo(struct LOG *log)
{
    switch(log->type){
        case LOG_TYPE_VHEADER_SYNC:
            undo_vheader_sync(log);
            break;
        case LOG_TYPE_CHUNK_INSERT:
            undo_chunk_insert(log);
            break;
        case LOG_TYPE_CHUNK_REMOVE:
            undo_chunk_remove(log);
            break;
        default:
            printf("Error: Illegal log type found!\n");
            exit(0);
    }
}

void undo_vheader_sync(struct LOG *log)
{
    void *p = log->logEntrys[0].addr - LastMappedAddr + mappedAddr;
    memcpy(p, log->logEntrys[0].buf, log->logEntrys[0].len);
    clwb_range(p, log->logEntrys[0].len);
    sfence();
}

void undo_chunk_insert(struct LOG *log)
{
    void *p = log->logEntrys[0].addr - LastMappedAddr + mappedAddr;
    memcpy(p, log->logEntrys[0].buf, log->logEntrys[0].len);
    clwb_range(p, log->logEntrys[0].len);
    
    p = log->logEntrys[1].addr - LastMappedAddr + mappedAddr;
    memcpy(p, log->logEntrys[1].buf, log->logEntrys[1].len);
    clwb_range(p, log->logEntrys[1].len);

    p = log->logEntrys[2].addr - LastMappedAddr + mappedAddr;
    memcpy(p, log->logEntrys[2].buf, log->logEntrys[2].len);
    clwb_range(p, log->logEntrys[2].len);

    sfence();
}

void undo_chunk_remove(struct LOG *log)
{
    void *p = log->logEntrys[0].addr - LastMappedAddr + mappedAddr;
    memcpy(p, log->logEntrys[0].buf, log->logEntrys[0].len);
    clwb_range(p, log->logEntrys[0].len);
    
    p = log->logEntrys[1].addr - LastMappedAddr + mappedAddr;
    memcpy(p, log->logEntrys[1].buf, log->logEntrys[1].len);
    clwb_range(p, log->logEntrys[1].len);

    p = log->logEntrys[2].addr - LastMappedAddr + mappedAddr;
    memcpy(p, log->logEntrys[2].buf, log->logEntrys[2].len);
    clwb_range(p, log->logEntrys[2].len);

    sfence();
}

void recover()
{
    printf("Started to scan the undo logs...\n");
    struct LOG *logs = (struct LOG *)LogBaseAddr;
    int n = 0;
    for(int i=0; i < MAX_THREAD; i++){
        if(logs[i].occupied != 0 && logs[i].state == LOG_RUNNING){
            n++;
            undo(&logs[i]);
        }
    }
    if(n == 0)
        printf("No uncommitted transaction found.\n");
    else
        printf("%d uncommitted transactions(s) have been rolled back.\n", n);
}
