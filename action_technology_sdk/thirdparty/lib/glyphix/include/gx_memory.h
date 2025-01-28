/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_prelude.h"
#include <cstddef>

namespace gx {
enum AllocType { AllocChunk = 0, AllocLocal, AllocUnknown };

void *malloc(std::size_t size, int type = AllocChunk);
void *realloc(void *ptr, std::size_t size, int type = AllocChunk);
void *calloc(std::size_t count, std::size_t size, int type = AllocChunk);
void free(void *ptr, int type = AllocChunk);
std::size_t malloc_usable_size(const void *ptr, int type = AllocChunk); // NOLINT(readability-*)

template<AllocType t, class T> class type_allocator { // NOLINT(readability-*)
public:
    typedef T value_type;
    typedef T *pointer;
    typedef const T *const_pointer;
    typedef T &reference;
    typedef const T &const_reference;
    typedef std::size_t size_type;
    typedef std::ptrdiff_t difference_type;

    template<class U> struct rebind {       // NOLINT(readability-*)
        typedef type_allocator<t, U> other; // NOLINT(readability-*)
    };

    type_allocator() GX_DEFAULT;
    type_allocator(const type_allocator &) GX_DEFAULT;
    template<class U> explicit type_allocator(const type_allocator<t, U> &) {}

    GX_NODISCARD pointer allocate(size_type n, const void * = nullptr) {
        return malloc(n * sizeof(T));
    }
    void deallocate(pointer p, size_type) { free(p); }
    void construct(pointer p, const T &value) { ::new (p) T(value); }
    void destroy(pointer p) { p->~T(); }
    GX_NODISCARD pointer address(reference x) { return (pointer)&x; }
    GX_NODISCARD const_pointer address(const_reference x) { return (const_pointer)&x; }
    GX_NODISCARD size_type max_size() const { // NOLINT(readability-*)
        return size_type(-1) / sizeof(T);
    }

    template<class U> bool operator==(const type_allocator<t, U> &) { return true; }
    template<class U> bool operator!=(const type_allocator<t, U> &) { return false; }

    GX_NODISCARD static pointer malloc(size_type s) { return pointer(gx::malloc(s, t)); }
    GX_NODISCARD static pointer realloc(pointer p, size_type s) {
        return pointer(gx::realloc(p, s, t));
    }
    static void free(pointer p) { gx::free(p, t); }
};

template<AllocType t> class type_allocator<t, void> {
public:
    typedef void value_type;
    typedef void *pointer;
    typedef const void *const_pointer;
    typedef std::size_t size_type;
    typedef std::ptrdiff_t difference_type;

    template<class U> struct rebind {       // NOLINT(readability-*)
        typedef type_allocator<t, U> other; // NOLINT(readability-*)
    };

    struct object { // NOLINT(readability-*)
        void *operator new(std::size_t size) { return malloc(size); }
        void *operator new[](std::size_t size) { return malloc(size); }
        void operator delete(void *ptr) { free((void *)ptr); }
        void operator delete[](void *ptr) { free((void *)ptr); }
    };

    template<class U> bool operator==(const type_allocator<t, U> &) { return true; }
    template<class U> bool operator!=(const type_allocator<t, U> &) { return false; }

    static void *malloc(size_type s) { return gx::malloc(s, t); }
    static void *realloc(void *p, size_type s) { return gx::realloc(p, s, t); }
    static void free(void *p) { gx::free(p, t); }
};

// NOLINTNEXTLINE(readability-*)
template<class T = void> class chunk_allocator : public type_allocator<AllocChunk, T> {};

// NOLINTNEXTLINE(readability-*)
template<class T = void> class local_allocator : public type_allocator<AllocLocal, T> {};

namespace texture_allocator {
struct Block {
    uint8_t *pointer;
    std::size_t size;
    int pitch;
    Block(uint8_t *pointer, std::size_t size, int pitch)
        : pointer(pointer), size(size), pitch(pitch) {}
};

uint8_t *alloc(std::size_t size);
Block alloc(int width, int height, int pixfmt);
void free(uint8_t *ptr);
} // namespace texture_allocator
} // namespace gx
