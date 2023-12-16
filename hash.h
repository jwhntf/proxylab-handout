#ifndef HASH_H
#define HASH_H

#include <string.h>

/* 实现了MurmurHash3算法, 用于将字符串转化为一个32bit值 */
/* 缓存是靠资源路径判定是否访问过的, 但是资源路径作为一个字符串不定长 */
/* 并且字符串比较函数肯定比整数比较慢得多, 所以考虑将资源路径哈希为一个整数, 作为其在缓存中的tag */
unsigned int MurmurHash(const char *string, size_t len, unsigned int seed);
#endif