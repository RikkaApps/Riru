/* tinynew.cpp

   Overrides operators new and delete
   globally to reduce code size.

   Public domain, use however you wish.
   If you really need a license, consider it MIT:
   http://www.opensource.org/licenses/mit-license.php

   - Eric Agan
     Elegant Invention
 */

#include <new>
#include <malloc.h>

void* operator new(std::size_t size) {
    return malloc(size);
}

void* operator new[](std::size_t size) {
    return malloc(size);
}

void operator delete(void* ptr) {
    free(ptr);
}

void operator delete[](void* ptr) {
    free(ptr);
}

/* Optionally you can override the 'nothrow' versions as well.
   This is useful if you want to catch failed allocs with your
   own debug code, or keep track of heap usage for example,
   rather than just eliminate exceptions.
 */

void* operator new(std::size_t size, const std::nothrow_t&) noexcept {
    return malloc(size);
}

void* operator new[](std::size_t size, const std::nothrow_t&) noexcept {
    return malloc(size);
}

void operator delete(void* ptr, const std::nothrow_t&) noexcept {
    free(ptr);
}

void operator delete[](void* ptr, const std::nothrow_t&) noexcept {
    free(ptr);
}

//eof
