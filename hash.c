#include "hash.h"
const unsigned int c1 = 0xcc9e2d51;
const unsigned int c2 = 0x1b873593;

const int r1 = 15;
const int r2 = 13;

const int m = 5;
const int n = 0xe6546b64;
unsigned int MurmurHash(const char *string, size_t len, unsigned int seed) {
    char buf[4];
    unsigned int k, h;

    /* 将h初始化为一个种子seed */
    h = seed;
    for (int i = 0; i < len; i+=4) {
        if ((len - i) < 4) {
            memset(buf, 0, 4 * sizeof(char));
            memcpy(buf, string + i, (len - i) * sizeof(char));
            k = 0;
            switch (len - i)
            {
            case 3:
                k ^= buf[2] << 16;
                break;
            case 2:
                k ^= buf[1] << 8;
                break;
            case 1:
                k ^= buf[0];
            default:
                break;
            }
            k = k * c1;
            k = (k << r1) | (k >> (32 - r1));
            k = k * c2;
            h ^= k;
            h ^= (unsigned int)len;
        } else {
            memset(buf, 0, 4 * sizeof(char));
            memcpy(buf, string + i, 4 * sizeof(char));
            k = *((unsigned int *)buf);
            k *= c1;
            k = (k << r1) | (k >> (32 - r1));
            k *= c2;
            h = h ^ k;
            h = (h << r2) | (h >> (32 - r2));
            h = h * m + n;
        }
    }
    h ^= h >> 16;
    h *= 0x85ebca6b;
    h ^= h >> 13;
    h *= 0xc2b2ae35;
    h ^= h >> 16;
    return h;
}