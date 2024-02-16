#ifndef HBALLOC_H
#define HBALLOC_H 1

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stddef.h>
#include <string.h>
#include <sys/types.h>
#include <sys/mman.h>
#include "bitmap.h"
#include "cas.h"
#include "chunk.h"
#include "config.h"
#include "hash.h"
#include "log.h"
#include "persist.h"
#include "pp.h"
#include "region.h"
#include "sizeclass.h"
#include "tcache.h"


PP malloc_small(struct TCACHE *tc, int sc, struct LOG *log);
PP malloc_large(struct TCACHE *tc, size_t size, struct LOG *log);
PP malloc_huge(size_t size);
void free_small(struct TCACHE *tc, void *va, size_t size, struct LOG *log);
void free_large(struct TCACHE *tc, void *va, size_t size, struct LOG *log);
void free_huge(PP pp, size_t size);

void vheader_sync(struct ChunkHeader *chunk, struct LOG *log);

void heapInit();
void updateVAs();

void hballoc_start(const char *work_dir, size_t heap_size);
void hballoc_exit();

PP hballoc_malloc(size_t size);
void hballoc_free(PP pp, size_t size);

void *hballoc_pp2va(PP pp);
void hballoc_persist(const void *addr, unsigned long len);

#endif