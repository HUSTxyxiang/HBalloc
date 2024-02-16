#include "include/HBalloc.h"
#include <sys/time.h>
#include <time.h>

void *mappedAddr = NULL;
// The current memory mapping address of the persistent heap

void *LastMappedAddr = NULL;
// The last memory mapping address of the persistent heap

void *TcacheBaseAddr = NULL;
// The base address of the thread-local cache

void *RegionBaseAddr = NULL;
// The base address of the first region in the persistent heap

void *LogBaseAddr = NULL;
// The memory mapping address of the log file

void *PP2VA_BaseAddr = NULL;
// The memory mapping address of the file that 
// stores the <persistent pointer, virtual address> mappings

size_t HEAP_SIZE = 0;
// The available size of the persistent heap
// It is specified by the user

size_t MAP_SIZE = 0;
// The mapping size of the persistent heap
// which is equal to the heap size plus the size of the chunk header and Tcache

size_t REGION_NUM = 0;
// Number of regions
// Which is equal to the heap size divides by the region size

void *pp2vaMap = NULL;
// The copy of <persistent pointer, virtual address> mappings in DRAM

char WORK_DIR[128] = "";
// The working directory specified by the user

static double GetTimeInSeconds(void)
{
    struct timeval t;
    gettimeofday(&t, NULL);
    return (double)t.tv_sec + (double)t.tv_usec / 1000000.0;
}

void hballoc_start(const char *work_dir, size_t heap_size)
{
    printf("Starting the HBalloc...\n");
    
    strcpy(WORK_DIR, work_dir);
    HEAP_SIZE = heap_size;

    REGION_NUM = HEAP_SIZE / REGION_SIZE;
    
    char heap_file[128];
    char log_file[128];
    char pp2va_file[128];
   
    sprintf(heap_file, "%s/hballoc_heap", work_dir);
    sprintf(log_file, "%s/hballoc_log", work_dir);
    sprintf(pp2va_file, "%s/hballoc_pp2va", work_dir);
    
    MAP_SIZE = HEAP_SIZE + HEADER_SIZE + TCACHE_SIZE;

    int fd;
    if(access(heap_file, F_OK) != 0){
        printf("The heap file \"%s\" does not exist.\n", heap_file);
        fd = open(heap_file, O_RDWR | O_CREAT);
        if(fd == -1){
            printf("Failed to create the heap file \"%s\"!\n", heap_file);
            exit(0);
        }
        ftruncate(fd, MAP_SIZE);
        printf("The heap file \"%s\" is created.\n", heap_file);
    }
    else{
        fd = open(heap_file, O_RDWR);
        if(fd == -1){
            printf("Failed to open the heap file \"%s\"!\n", heap_file);
            exit(0);
        }
        printf("The heap file \"%s\" is opened.\n", heap_file);
    }

    void *aimedAddr = (void *)0x7f0000000000;
    for(int i=0; i < 16; i++){
        mappedAddr = mmap(aimedAddr, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        if(mappedAddr == aimedAddr)
            break;
        aimedAddr += 0x1000000000;
    }
    
    if(mappedAddr != aimedAddr){
        printf("Error: Failed to align the mapping address to 1MB. Please retry!\n");
        exit(0);
    }

    printf("The heap file \"%s\" is mapped at %p\n", heap_file, mappedAddr);
    close(fd);

    if(access(log_file, F_OK) != 0){
        fd = open(log_file, O_RDWR | O_CREAT);
        if(fd == -1){
            printf("Failed to create the file \"%s\"!\n", log_file);
            exit(0);
        }
        ftruncate(fd, LOG_SIZE);
        LogBaseAddr = mmap(NULL, LOG_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
        if(LogBaseAddr == MAP_FAILED){
            printf("Failed to mmap() the file \"%s\"!\n", log_file);
            exit(0);
        }
        close(fd);
        LogInit();
    }
    else{
        fd = open(log_file, O_RDWR);
        if(fd == -1){
            printf("Failed to open the file \"%s\"!\n", log_file);
            exit(0);
        }
        LogBaseAddr = mmap(NULL, LOG_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
        if(LogBaseAddr == MAP_FAILED){
            printf("Failed to mmap() the file \"%s\"!\n", log_file);
            exit(0);
        }
        close(fd);
    }

    if(access(pp2va_file, F_OK) != 0){
        fd = open(pp2va_file, O_RDWR | O_CREAT);
        if(fd == -1){
            printf("Failed to create the file \"%s\"!\n", pp2va_file);
            exit(0);
        }
        ftruncate(fd, PP2VA_SIZE);
        PP2VA_BaseAddr = mmap(NULL, PP2VA_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
        if(PP2VA_BaseAddr == MAP_FAILED){
            printf("Failed to mmap() the file \"%s\"!\n", pp2va_file);
            exit(0);
        }
        close(fd);
        memset(PP2VA_BaseAddr, 0, PP2VA_SIZE);
        clwb_range(PP2VA_BaseAddr, PP2VA_SIZE);
    }
    else{
        fd = open(pp2va_file, O_RDWR);
        if(fd == -1){
            printf("Failed to open the file \"%s\"!\n", pp2va_file);
            exit(0);
        }
        PP2VA_BaseAddr = mmap(NULL, PP2VA_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
        if(PP2VA_BaseAddr == MAP_FAILED){
            printf("Failed to mmap() the file \"%s\"!\n", pp2va_file);
            exit(0);
        }
        close(fd);
    }
    pp2vaMap = malloc(PP2VA_SIZE);
    memcpy(pp2vaMap, PP2VA_BaseAddr, PP2VA_SIZE);


    TcacheBaseAddr = mappedAddr + HEADER_SIZE;
    RegionBaseAddr = TcacheBaseAddr + TCACHE_SIZE;
    printf("Region Base Address = %lx\n", (uint64_t)RegionBaseAddr);

    if(*(uint64_t *)mappedAddr != MAGIC){
        printf("Initializing the persistent heap...\n");
        heapInit();
        printf("The persistent heap is initialized.\n");
    }
    else{
        LastMappedAddr = *(void **)(mappedAddr+8);
        printf("Last time the persistent heap is mapped at %p\n", LastMappedAddr);
        *(uint64_t *)mappedAddr = MAGIC;
        *(void **)(mappedAddr+8) = mappedAddr;
        *(void **)(mappedAddr+16) = TcacheBaseAddr;
        *(void **)(mappedAddr+24) = RegionBaseAddr;
        *(uint64_t *)(mappedAddr+32) = heap_size;
        *(uint64_t *)(mappedAddr+40) = MAP_SIZE;
        clwb_range(mappedAddr, 48);
        sfence();
        double t1 = GetTimeInSeconds();
        recover();
        double t2 = GetTimeInSeconds();
        printf("Recovery time is %lfs\n", t2-t1);
        updateVAs();
    }

    printf("HBalloc is started!\n");
}

void hballoc_exit()
{
    printf("HBalloc is exiting...\n");
    struct TCACHE *tc = (struct TCACHE *)TcacheBaseAddr;
    for(int i=0; i < MAX_THREAD; i++){
        pthread_t tid = tc[i].occupied;
        if(tid == 0)
            continue;
        struct LOG *log = getLog(tid);
        while(tc[i].freeChunksNum != 0){
            printf("There are %ld free chunks left in Tcache %d\n", tc[i].freeChunksNum, i);
            struct ChunkHeader *chunk = tc[i].freeChunkList.Head.nextChunk;
            struct ChunkHeader *chunkn = chunk->nextChunk;
            int N = chunk->contiguousChunksNum;
            LOG_CHUNK_REMOVE(log, chunk, &(tc[i].freeChunkList.Head), chunkn);
            TX_BEGIN(log);
            tc[i].freeChunkList.Head.nextChunk = chunkn;
            clwb_range(&(tc[i].freeChunkList.Head.nextChunk), 8);
            chunkn->priorChunk = &(tc[i].freeChunkList.Head);
            clwb_range(&(chunkn->priorChunk), 8);
            tc[i].freeChunksNum -= N;
            clwb_range(&tc[i].freeChunksNum, 8);
            sfence();
            TX_COMMIT(log);
            
            int regionID = ((char *)chunk - (char *)RegionBaseAddr) / REGION_SIZE;
            struct RegionHeader *r = (struct RegionHeader *)((char *)RegionBaseAddr + regionID * REGION_SIZE);
            freeChunks(
                &(r->chunkListHead),
                &(r->chunkListTail),
                &(r->freeChunksNum),
                chunk,
                N,
                log
            );
        }
        tc[i].occupied = 0;
        tc[i].h1 = 0;
        tc[i].h2 = 0;
        tc[i].h3 = 0;
        clwb_range(&tc[i], 32);
    }

    struct LOG *log = (struct LOG *)LogBaseAddr;
    for(int i=0; i < MAX_THREAD; i++){
        log[i].occupied = 0;
        log[i].state = LOG_COMMITTED;
        clwb_range((void *)&log[i], 16);
    }

    munmap(mappedAddr, MAP_SIZE);
    printf("The persistent heap is unmapped.\n");

    munmap(LogBaseAddr, LOG_SIZE);
    printf("The HBalloc LOG file is unmapped.\n");
    
    memcpy(PP2VA_BaseAddr, pp2vaMap, PP2VA_SIZE);
    clwb_range(PP2VA_BaseAddr, PP2VA_SIZE);
    sfence();
    free(pp2vaMap);
    munmap(PP2VA_BaseAddr, PP2VA_SIZE);
    printf("The HBalloc PP2VA file is unmapped.\n");
    
    printf("HBalloc is exited safely.\nBye.\n");
}

void heapInit()
{
    struct TCACHE *tc = (struct TCACHE *)TcacheBaseAddr;
    for(int i=0; i < MAX_THREAD; i++){
        tc[i].occupied = 0;
        tc[i].h1 = 0;
        tc[i].h2 = 0;
        tc[i].h3 = 0;
        for(int j=0; j < SIZECLASS_NUM; j++){
            tc[i].partialChunkLists[j].Head.nextChunk = &(tc[i].partialChunkLists[j].Tail);
            tc[i].partialChunkLists[j].Head.priorChunk = NULL;
            tc[i].partialChunkLists[j].Tail.priorChunk = &(tc[i].partialChunkLists[j].Head);
            tc[i].partialChunkLists[j].Tail.nextChunk = NULL;
            tc[i].partialChunksNum[j] = 0;
        }
        tc[i].freeChunkList.Head.nextChunk = &(tc[i].freeChunkList.Tail);
        tc[i].freeChunkList.Head.priorChunk = NULL;
        tc[i].freeChunkList.Tail.priorChunk = &(tc[i].freeChunkList.Head);
        tc[i].freeChunkList.Tail.nextChunk = NULL;
        tc[i].freeChunksNum = 0;
    }
    clwb_range(TcacheBaseAddr, TCACHE_SIZE);

    printf("There are %lu regions in the persistent heap\n", REGION_NUM);
    struct RegionHeader *r = (struct RegionHeader *)RegionBaseAddr;
    for(int i=0; i < REGION_NUM; i++){
        struct ChunkHeader *firstChunk = (struct ChunkHeader *)((void *)r + sizeof(struct RegionHeader));
        firstChunk->priorChunk = &(r->chunkListHead);
        firstChunk->nextChunk = &(r->chunkListTail);
        firstChunk->contiguousChunksNum = (REGION_SIZE - sizeof(struct RegionHeader)) / CHUNK_SIZE;
        r->occupied = 0;
        r->freeChunksNum = firstChunk->contiguousChunksNum;
        r->chunkListHead.priorChunk = NULL;
        r->chunkListHead.nextChunk = firstChunk;
        r->chunkListTail.priorChunk = firstChunk;
        r->chunkListTail.nextChunk = NULL;
        clwb_range((void *)r, sizeof(struct RegionHeader));
        clwb_range((void *)firstChunk, sizeof(struct ChunkHeader));
        r = (struct RegionHeader *) ( (void *)r + REGION_SIZE );
    }

    *(uint64_t *)mappedAddr = MAGIC;
    *(void **)(mappedAddr+8) = mappedAddr;
    *(void **)(mappedAddr+16) = TcacheBaseAddr;
    *(void **)(mappedAddr+24) = RegionBaseAddr;
    *(uint64_t *)(mappedAddr+32) = HEAP_SIZE;
    *(uint64_t *)(mappedAddr+40) = MAP_SIZE;
    clwb_range(mappedAddr, 48);
    sfence();
}

void updateVAs()
{
    //printf("Start to update virtual addresses of link pointers...\n");
    struct RegionHeader *r = (struct RegionHeader *)RegionBaseAddr;
    for(int i=0; i < REGION_NUM; i++){
        //printf("Region %d\n", i);
        //printf("region base address = %p\n", r);
        r->chunkListHead.nextChunk
            = (void *)(r->chunkListHead.nextChunk) - LastMappedAddr + mappedAddr;
        //printf("r->chunkListHead.nextChunk = %p\n", r->chunkListHead.nextChunk);
        
        r->chunkListTail.priorChunk
            = (void *)(r->chunkListTail.priorChunk) - LastMappedAddr + mappedAddr;
        struct ChunkHeader *p = r->chunkListHead.nextChunk;
        while(p != &(r->chunkListTail)){
            p->priorChunk = (void *)(p->priorChunk) - LastMappedAddr + mappedAddr;
            p->nextChunk = (void *)(p->nextChunk) - LastMappedAddr + mappedAddr;
            p = p->nextChunk;
        }
        r = (struct RegionHeader *) ( (void *)r + REGION_SIZE );
    }

    struct TCACHE *tc = (struct TCACHE *)TcacheBaseAddr;
    for(int i=0; i < MAX_THREAD; i++){
        for(int j=0; j < SIZECLASS_NUM; j++){
            tc[i].partialChunkLists[j].Head.nextChunk
                = (void *)(tc[i].partialChunkLists[j].Head.nextChunk) - LastMappedAddr + mappedAddr;
            tc[i].partialChunkLists[j].Tail.priorChunk
                = (void *)(tc[i].partialChunkLists[j].Tail.priorChunk) - LastMappedAddr + mappedAddr;
            struct ChunkHeader *p = tc[i].partialChunkLists[j].Head.nextChunk;
            while(p != &(tc[i].partialChunkLists[j].Tail)){
                p->priorChunk = (void *)(p->priorChunk) - LastMappedAddr + mappedAddr;
                p->nextChunk = (void *)(p->nextChunk) - LastMappedAddr + mappedAddr;
                p = p->nextChunk;
            }
        }
        
        tc[i].freeChunkList.Head.nextChunk
            = (void *)(tc[i].freeChunkList.Head.nextChunk) - LastMappedAddr + mappedAddr;
        tc[i].freeChunkList.Tail.priorChunk
            = (void *)(tc[i].freeChunkList.Tail.priorChunk) - LastMappedAddr + mappedAddr;
        struct ChunkHeader *p = tc[i].freeChunkList.Head.nextChunk;
        while(p != &(tc[i].freeChunkList.Tail)){
            p->priorChunk = (void *)(p->priorChunk) - LastMappedAddr + mappedAddr;
            p->nextChunk = (void *)(p->nextChunk) - LastMappedAddr + mappedAddr;
            p = p->nextChunk;
        }

        tc[i].occupied = 0;
        tc[i].h1 = 0;
        tc[i].h2 = 0;
        tc[i].h3 = 0;
    }

    //printf("Finished to update virtual addresses.\n");
}


PP hballoc_malloc(size_t size)
{
    pthread_t tid = pthread_self();

    struct TCACHE *tc = findTcache(tid);
    if(tc == NULL)
        tc = occupyTcache(tid);
    
    struct LOG *log = getLog(tid);
    if(log == NULL)
        log = occupyLog(tid);

    if(size <= 8192){
        int sc = getSC(size);
        return malloc_small(tc, sc, log);
    }

    else if(size <= (128 * 1024ULL * 1024ULL)){
        return malloc_large(tc, size, log);
    }
    
    else{
        return malloc_huge(size);
    }
}

PP malloc_small(struct TCACHE *tc, int sc, struct LOG *log)
{
    //printf("Get into malloc_small()\n");
    //printf("tc->partialChunksNum = %ld\n", tc->partialChunksNum[sc]);
    if(tc->partialChunksNum[sc] == 0){
        if(tc->freeChunksNum == 0){
            struct RegionHeader *r = occupyAregion(tc);
            struct ChunkHeader *chunks = allocateChunks(
                &(r->chunkListHead),
                &(r->chunkListTail),
                &(r->freeChunksNum),
                TCACHE_SZ,
                log
            );
            printf(
                "Thread %lu allocated %d chunks from Region %d\n",
                tc->occupied,
                TCACHE_SZ,
                (int)(((char *)r - (char *)RegionBaseAddr) / REGION_SIZE)
            );
            releaseAregion(r);
            freeChunks(
                &(tc->freeChunkList.Head),
                &(tc->freeChunkList.Tail),
                &(tc->freeChunksNum),
                chunks,
                TCACHE_SZ,
                log
            );
        }

        struct ChunkHeader *chunk = allocateChunks(
            &(tc->freeChunkList.Head),
            &(tc->freeChunkList.Tail),
            &(tc->freeChunksNum),
            1,
            log
        );

        struct ChunkHeader *p = tc->partialChunkLists[sc].Head.nextChunk;

        LOG_CHUNK_INSERT(log, chunk, &(tc->partialChunkLists[sc].Head), p);
        TX_BEGIN(log);
        chunk->priorChunk = &(tc->partialChunkLists[sc].Head);
        chunk->nextChunk = p;
        clwb_range(chunk, 16);
        p->priorChunk = chunk;
        clwb_range(&(p->priorChunk), 8);
        tc->partialChunkLists[sc].Head.nextChunk = chunk;
        tc->partialChunksNum[sc]++;
        clwb_range(&(tc->partialChunkLists[sc].Head.nextChunk), 8);
        sfence();
        TX_COMMIT(log);

        chunk->sizeclass = sc;
        chunk->blockSize = getSize(sc);
        memset(chunk->bitmap, 0xff, sizeof(chunk->bitmap));
        chunk->freeBlocksNum = (CHUNK_SIZE - sizeof(struct ChunkHeader)) / (chunk->blockSize);
        chunk->firstFreeBlock = 0;
        chunk->vhp = malloc(sizeof(struct VHeader));
        memset(chunk->vhp->bitmap, 0xff, sizeof(chunk->bitmap));
        chunk->vhp->freeBlocksNum = chunk->freeBlocksNum;
        chunk->vhp->firstFreeBlock = 0;
        chunk->vhp->counter = 0;
        clwb_range(chunk, sizeof(struct ChunkHeader));
    }
    
    struct ChunkHeader *chunk = tc->partialChunkLists[sc].Head.nextChunk;
    while(chunk != &(tc->partialChunkLists[sc].Tail) && chunk->vhp->freeBlocksNum == 0)
        chunk = chunk->nextChunk;
    
    int idx = chunk->vhp->firstFreeBlock;
    clearBit(chunk->vhp->bitmap, idx);
    (chunk->vhp->freeBlocksNum)--;

    if(chunk->vhp->freeBlocksNum == 0){ // Now the chunk is exhausted
        
        chunk->vhp->firstFreeBlock = -1;
        vheader_sync(chunk, log);

        // Move the exhausted chunk to the end of the list
        struct ChunkHeader *chunkp = chunk->priorChunk;
        struct ChunkHeader *chunkn = chunk->nextChunk;
        LOG_CHUNK_REMOVE(log, chunk, chunkp, chunkn);
        TX_BEGIN(log);
        chunkp->nextChunk = chunkn;
        clwb_range(&(chunkp->nextChunk), 8);
        chunkn->priorChunk = chunkp;
        clwb_range(&(chunkn->priorChunk), 8);
        sfence();
        TX_COMMIT(log);
        
        struct ChunkHeader *tailp = tc->partialChunkLists[sc].Tail.priorChunk;
        LOG_CHUNK_INSERT(log, chunk, tailp, &(tc->partialChunkLists[sc].Tail));
        TX_BEGIN(log);
        chunk->priorChunk = tailp;
        chunk->nextChunk = &(tc->partialChunkLists[sc].Tail);
        clwb_range(chunk, 16);
        tailp->nextChunk = chunk;
        clwb_range(&(tailp->nextChunk), 8);
        tc->partialChunkLists[sc].Tail.priorChunk = chunk;
        clwb_range(&(tc->partialChunkLists[sc].Tail.priorChunk), 8);
        sfence();
        TX_COMMIT(log);
        (tc->partialChunksNum[sc])--;
    }

    else{
        (chunk->vhp->firstFreeBlock)++;
        while(getBit(chunk->vhp->bitmap, chunk->vhp->firstFreeBlock) == 0)
            (chunk->vhp->firstFreeBlock)++;
        if(++(chunk->vhp->counter) == BATCH_SIZE)
            vheader_sync(chunk, log);
    }
    
    void *va = (void *)chunk + sizeof(struct ChunkHeader) + idx * (chunk->blockSize);
    
    return getPP(va);
}

PP malloc_large(struct TCACHE *tc, size_t size, struct LOG *log)
{
    int N = ((size % CHUNK_SIZE) == 0) ? (size / CHUNK_SIZE) : (size / CHUNK_SIZE + 1);
    if(tc->freeChunksNum < N){
        struct RegionHeader *r = occupyAregion(tc);
        struct ChunkHeader *chunks = allocateChunks(
            &(r->chunkListHead),
            &(r->chunkListTail),
            &(r->freeChunksNum),
            N,
            log
        );
        printf(
                "Thread %lu allocated %d chunks from Region %d\n",
                tc->occupied,
                TCACHE_SZ,
                (int)(((char *)r - (char *)RegionBaseAddr) / REGION_SIZE)
            );
        releaseAregion(r);
        freeChunks(
            &(tc->freeChunkList.Head),
            &(tc->freeChunkList.Tail),
            &(tc->freeChunksNum),
            chunks,
            N,
            log
        );
    }
    
    struct ChunkHeader *chunks = allocateChunks(
        &(tc->freeChunkList.Head),
        &(tc->freeChunkList.Tail),
        &(tc->freeChunksNum),
        N,
        log
    );
    
    return getPP((void *)chunks);
}

PP malloc_huge(size_t size)
{
    if(size % 4096 != 0)
        size = ((size >> 12) + 1) << 12;
    
    oid_t oid = get_oid();
    
    char filename[128];
    sprintf(filename, "%s/hugeblock_%lu\n", WORK_DIR, oid);
    
    int fd = open(filename, O_RDWR | O_CREAT);
    
    if(fd == -1){
        printf("Failed to create the heap file \"%s\"!\n", filename);
        exit(0);
    }
    
    ftruncate(fd, size);
    
    void *addr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    
    if(mappedAddr == MAP_FAILED){
        printf("Failed to mmap() the heap file \"%s\"!\n", filename);
        exit(0);
    }

    PP2VA_PAIR *a = pp2vaMap;
    int i = hash1(oid, MAX_OBJ_NUM);
    while(CAS((volatile uint64_t*)&(a[i].pp), 0, oid) == false)
        i = (i+1) % MAX_OBJ_NUM;
    a[i].va = addr;

    return oid;
}


void hballoc_free(PP pp, size_t size)
{
    void *va = pp2va(pp);
    
    pthread_t tid = pthread_self();
    
    struct TCACHE *tc = findTcache(tid);
    
    struct LOG *log = getLog(tid);
    
    if(va < mappedAddr || va >= mappedAddr + MAP_SIZE){
        free_huge(pp, size);
    }

    else if((((uint64_t)va >> 14) << 14) == (uint64_t)va){
        free_large(tc, va, size, log);
    }

    else{
        free_small(tc, va, size, log);
    }
    
    freePP(pp);
}

void free_small(struct TCACHE *tc, void *va, size_t size, struct LOG *log)
{
   
    struct ChunkHeader *chunk = (struct ChunkHeader *)(((uint64_t)va >> 14) << 14);
    // Get the base address of the chunk that contains the block 
    // By aligning the address of the block down to 16KB
    // Since the base address of a chunk must be the multiple of the chunk size (16KB)

    int sc = chunk->sizeclass;
    
    int idx = (va - (void *)chunk - sizeof(struct ChunkHeader)) / (chunk->blockSize);
    
    setBit(chunk->vhp->bitmap, idx);
    (chunk->vhp->freeBlocksNum)++;
    
    if(idx < chunk->vhp->firstFreeBlock)
        chunk->vhp->firstFreeBlock = idx;
    
    if(chunk->vhp->freeBlocksNum == 1)
        (tc->partialChunksNum[sc])++;
    
    chunk->vhp->counter++;

    if(chunk->vhp->counter == BATCH_SIZE)
        vheader_sync(chunk, log);
    
    // All blocks in the chunk is reclaimed, we can reclaim the chunk now
    if(chunk->vhp->freeBlocksNum == (CHUNK_SIZE - sizeof(struct ChunkHeader)) / (chunk->blockSize))
    {
        //printf("We are going to free a chunk\n");
        
        // Destroy the vheader
        vheader_sync(chunk, log);
        free(chunk->vhp);

        // Remove the chunk from the partial chunk list
        struct ChunkHeader *chunkp = chunk->priorChunk;
        struct ChunkHeader *chunkn = chunk->nextChunk;
        LOG_CHUNK_REMOVE(log, chunk, chunkp, chunkn);
        TX_BEGIN(log);
        chunkp->nextChunk = chunkn;
        clwb_range(&(chunkp->nextChunk), 8);
        chunkn->priorChunk = chunkp;
        clwb_range(&(chunkn->priorChunk), 8);
        (tc->partialChunksNum[sc])--;
        clwb_range(&(tc->partialChunksNum[sc]), 8);
        sfence();
        TX_COMMIT(log);

        // Insert the chunk into the free chunk list
        freeChunks(
            &(tc->freeChunkList.Head),
            &(tc->freeChunkList.Tail),
            &(tc->freeChunksNum),
            chunk,
            1,
            log
        );

        if(tc->freeChunksNum >= TCACHE_SZ){
            while(tc->freeChunksNum != 0){
                struct ChunkHeader *chunk = tc->freeChunkList.Head.nextChunk;
                struct ChunkHeader *chunkn = chunk->nextChunk;
                int N = chunk->contiguousChunksNum;
                LOG_CHUNK_REMOVE(log, chunk, &(tc->freeChunkList.Head), chunkn);
                TX_BEGIN(log);
                tc->freeChunkList.Head.nextChunk = chunkn;
                clwb_range(&(tc->freeChunkList.Head.nextChunk), 8);
                chunkn->priorChunk = &(tc->freeChunkList.Head);
                clwb_range(&(chunkn->priorChunk), 8);
                tc->freeChunksNum -= N;
                clwb_range(&tc->freeChunksNum, 8);
                sfence();
                TX_COMMIT(log);

                int regionID = ((char *)chunk - (char *)RegionBaseAddr) / REGION_SIZE;
                struct RegionHeader *r = (struct RegionHeader *)((char *)RegionBaseAddr + regionID * REGION_SIZE);
                while( CAS((volatile uint64_t *)&(r->occupied), 0, tc->occupied) == false )
                    continue;
                freeChunks(
                    &(r->chunkListHead),
                    &(r->chunkListTail),
                    &(r->freeChunksNum),
                    chunk,
                    N,
                    log
                );
                printf(
                    "Thread %lu freed %d chunks to Region %d\n",
                    tc->occupied,
                    N,
                    regionID
                );
                r->occupied = 0;
            }
        }
    }
}

void free_large(struct TCACHE *tc, void *va, size_t size, struct LOG *log)
{
    struct ChunkHeader *chunk = (struct ChunkHeader *)va;
    int N = ((size % CHUNK_SIZE) == 0) ? (size / CHUNK_SIZE) : (size / CHUNK_SIZE + 1);
    freeChunks(
        &(tc->freeChunkList.Head),
        &(tc->freeChunkList.Tail),
        &(tc->freeChunksNum),
        chunk,
        N,
        log
    );
}

void free_huge(PP pp, size_t size)
{
    PP2VA_PAIR *a = pp2vaMap;
    int i = hash1(pp, MAX_OBJ_NUM);
    while(a[i].pp != pp)
        i = (i+1) % MAX_OBJ_NUM;

    munmap(a[i].va, size);
    
    char filename[128];
    sprintf(filename, "%s/hugeblock_%lu\n", WORK_DIR, pp);
    
    if(remove(filename) != 0){
        printf("Failed to remove the file \"%s\"!\n", filename);
        exit(0);
    }

    i = hash1(pp, MAX_OBJ_NUM);
    while(a[i].pp != pp)
        i = (i+1) % MAX_OBJ_NUM;
    a[i].pp = 0;
    a[i].va = 0;
}

void *hballoc_pp2va(PP pp)
{
    return pp2va(pp);
}

void hballoc_persist(const void *addr, unsigned long len)
{
    clwb_range(addr, len);
    sfence();
}

void vheader_sync(struct ChunkHeader *chunk, struct LOG *log)
{
    LOG_VHEADER_SYNC(log, chunk);
    TX_BEGIN(log);
    memcpy(chunk->bitmap, chunk->vhp->bitmap, sizeof(chunk->bitmap));
    chunk->freeBlocksNum = chunk->vhp->freeBlocksNum;
    chunk->firstFreeBlock = chunk->vhp->firstFreeBlock;
    clwb_range(chunk->bitmap, 56);
    sfence();
    TX_COMMIT(log);
    chunk->vhp->counter = 0;
}