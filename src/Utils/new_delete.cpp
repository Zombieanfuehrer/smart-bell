#ifdef __AVR__
#include <stdlib.h>
#include <stdint.h>

// Simple new/delete operators for AVR - uses malloc/free
void* operator new(size_t size) { return malloc(size); }

void* operator new[](size_t size) { return malloc(size); }

void operator delete(void* ptr) { free(ptr); }

void operator delete[](void* ptr) { free(ptr); }

void operator delete(void* ptr, size_t) { free(ptr); }

void operator delete[](void* ptr, size_t) { free(ptr); }

// C++ Guard stubs for static local initialization (single-threaded, no-op)
extern "C" {
int __cxa_guard_acquire(uint8_t *g) {
  return !(*g);
}

void __cxa_guard_release(uint8_t *g) {
  *g = 1;
}

void __cxa_guard_abort(uint8_t *) {}
}
#endif
