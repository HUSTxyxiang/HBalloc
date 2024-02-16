#ifndef CONFIG_H
#define CONFIG_H 1

#define MAX_THREAD 1024ULL

#define HEADER_SIZE (1 * 1024 * 1024ULL)

#define TCACHE_SIZE (8 * 1024ULL * MAX_THREAD)

#define LOG_SIZE (1024ULL * MAX_THREAD)

#define MAX_OBJ_NUM (1024 * 1024ULL)

#define PP2VA_SIZE (MAX_OBJ_NUM << 4)

#define REGION_SIZE (256 * 1024 * 1024ULL)

#define CHUNK_SIZE (16 * 1024ULL)

#define CACHELINE_SIZE 64

#define MAGIC 0Xb510fff202273509ULL

#define NPP 0

#define BATCH_SIZE 32

#define TCACHE_SZ 8

//#define PIP 1

#endif


