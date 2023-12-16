#include "cache.h"

cache_object_t Cache[CACHE_SLOT];
unsigned int global_time;

void init_cache(cache_object_t *Cache, size_t cache_size) {
    memset(Cache, 0, sizeof(cache_object_t) * cache_size);
}

/* 寻找该id是否已经在Cache中, 如果有就把指针放到result */
cache_object_t *find_id(cache_object_t *Cache, size_t cache_size, unsigned int id) {
    int i;
    for (i = 0; i < cache_size; i++) {
        if (Cache[i].id == id) {
            return &Cache[i];
        }
    }
    return NULL;
}

void store_obj(cache_object_t *Cache, size_t cache_size, cache_object_t *obj) {
    unsigned int lru_value;
    size_t lru_slot;

    lru_value = 0xFFFFFFFF;
    for (size_t i = 0; i < cache_size; i++) {
        if (Cache[i].timestamp < lru_value) {
            lru_value = Cache[i].timestamp;
            lru_slot = i;
        }
        if (Cache[i].id == 0) {
            Cache[i] = *obj; /* 进行值复制 */
            return;
        }
    }
    /* 没有找到空槽 */
    Cache[lru_slot] = *obj;
    return;
}