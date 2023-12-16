#ifndef HASH_H
#define HASH_H
#include <stdlib.h>
#include <string.h>

unsigned int MurmurHash(const char *string, size_t len, unsigned int seed);
#endif