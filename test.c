#include<stdio.h>
#include "hash.h"
#include "http.h"
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
    char bar[] = "asdfasdfadfContent-length: 1234\r\n";
    size_t len = get_content_length(bar);
    printf("content-length: %ld\n", len);
}