#ifdef __AVR__
#include <stdlib.h>

// Simple new/delete operators for AVR - uses malloc/free
void* operator new(size_t size) { return malloc(size); }

void* operator new[](size_t size) { return malloc(size); }

void operator delete(void* ptr) { free(ptr); }

void operator delete[](void* ptr) { free(ptr); }

void operator delete(void* ptr, size_t) { free(ptr); }

void operator delete[](void* ptr, size_t) { free(ptr); }
#endif
