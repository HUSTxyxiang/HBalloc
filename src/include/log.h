#ifndef LOG_H
#define LOG_H 1

#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <sys/types.h>

struct LogEntry{
    void *addr;
    size_t len;
    char buf[112];
};  // 128 bytes

struct LOG{
    pthread_t occupied;
    int state;
    int type;
    char alignment[624];
    struct LogEntry logEntrys[3];
};  // 1 KB

#define LOG_RUNNING 1
#define LOG_COMMITTED 0

#define LOG_TYPE_VHEADER_SYNC 0 // 1 * 56 bytes
#define LOG_TYPE_CHUNK_INSERT 1 // 3 * 24 bytes
#define LOG_TYPE_CHUNK_REMOVE 2 // 3 * 24 bytes

void LogInit();
void recover();

struct LOG *occupyLog(pthread_t tid);
struct LOG *getLog(pthread_t tid);

void TX_BEGIN(struct LOG *log);
void TX_COMMIT(struct LOG *log);

void LOG_VHEADER_SYNC(struct LOG *log, void *chunk);
void LOG_CHUNK_INSERT(struct LOG *log, void *chunk, void *pChunk, void *nChunk);
void LOG_CHUNK_REMOVE(struct LOG *log, void *chunk, void *pChunk, void *nChunk );


void undo(struct LOG *log);
void undo_vheader_sync(struct LOG *log);
void undo_chunk_insert(struct LOG *log);
void undo_chunk_remove(struct LOG *log);

#endif