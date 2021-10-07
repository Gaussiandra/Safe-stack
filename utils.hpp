typedef uint32_t hashType;

hashType calcHash(const char *dataPointer, size_t nBytes);
void* recalloc(void* ptr, size_t nElements, size_t size);