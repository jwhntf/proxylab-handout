#ifndef CACHE_H
#define CACHE_H
#define CACHE_SLOT 1024
#include <stdlib.h>
#include <string.h>
typedef struct {
    unsigned int id;
    unsigned int timestamp;
    char *objp;
} cache_object_t;

void init_cache(cache_object_t *Cache, size_t cache_size);
cache_object_t *find_id(cache_object_t *Cache, size_t cache_size, unsigned int id);
void store_obj(cache_object_t *Cache, size_t cache_size, cache_object_t *obj);
#endif