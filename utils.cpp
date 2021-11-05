#include <cassert>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include "utils.hpp"

typedef uint32_t hashType;

// Jenkins hash function. Source: https://en.wikipedia.org/wiki/Jenkins_hash_function#one_at_a_time
hashType calcHash(const char *dataPointer, size_t nBytes) {
    assert(dataPointer);

    hashType hash, i;
    for(hash = i = 0; i < (hashType) nBytes; ++i)
    {
        hash += (hashType) dataPointer[i];
        hash += (hash << 10);
        hash ^= (hash >> 6);
    }
    hash += (hash << 3);
    hash ^= (hash >> 11);
    hash += (hash << 15);

    return hash;
}

void* recalloc(void* ptr, size_t nElements, size_t size) {
    size_t sizeToAllocate = nElements * size;
    void* newPtr = realloc(ptr, sizeToAllocate);
    if (newPtr != ptr && newPtr != nullptr) {
        // memset(newPtr, 0, sizeToAllocate);
    }

    return newPtr;
}