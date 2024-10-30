// aligned_malloc.c from https://embeddedartistry.com/blog/2017/02/22/generating-aligned-memory/
// and https://github.com/embeddedartistry/embedded-resources/blob/master/examples/libc/malloc_aligned.c
//
// Modified to work with Bristlemouth bm_core bm_os.h

#include "aligned_malloc.h"
#include "bm_os.h"
#include <stdint.h>

// Align a pointer address upwards to the nearest given power of 2
#ifndef align_up
#define align_up(num, align) (((num) + ((align) - 1)) & ~((align) - 1))
#endif

// Number of bytes we're using for storing the aligned pointer offset
typedef uint16_t offset_t;

void *aligned_malloc(size_t align, size_t size) {
  void *ptr = NULL;

  // Align must be a power of two since align_up operates on powers of two
  if (align && size && (align & (align - 1)) == 0) {
    const size_t offset_size = sizeof(offset_t);
    uint32_t hdr_size = offset_size + (align - 1);
    void *p = bm_malloc(size + hdr_size);
    if (p) {
      ptr = (void *)align_up(((uintptr_t)p + offset_size), align);

      // Calculate the offset and store it behind our aligned pointer
      *((offset_t *)ptr - 1) = (offset_t)((uintptr_t)ptr - (uintptr_t)p);

    } // else NULL, could not malloc
  } // else NULL, invalid arguments

  return ptr;
}

/**
 * aligned_free works like free(), but we work backwards from the returned
 * pointer to find the correct offset and pointer location to return to free()
 * Note that it is VERY BAD to call free() on an aligned_malloc() pointer.
 */
void aligned_free(void *ptr) {
  if (ptr) {
    /*
     * Walk backwards from the passed-in pointer to get the pointer offset
     * We convert to an offset_t pointer and rely on pointer math to get the data
     */
    offset_t offset = *((offset_t *)ptr - 1);

    // Once we have the offset, we can get our original pointer and call free
    void *p = (void *)((uint8_t *)ptr - offset);
    bm_free(p);
  }
}
