#include<stdio.h>
#include "hash.h"
typedef struct {
    unsigned int id;
    unsigned int timestamp;
    void *objp;
} cache_object;
int main() {
    char buf[] = "asdfasdfasdfa\0";
    unsigned int hs = MurmurHash(buf, strlen(buf), 0x11451400);
    printf("%s: 0x%x\n", buf, hs);
    printf("size: %ld\n", sizeof(cache_object));
}