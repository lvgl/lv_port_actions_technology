/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_prelude.h"
#ifndef GX_COPMATIBLE_CXX_98
#include <atomic>
#endif

namespace gx {
#ifdef GX_COPMATIBLE_CXX_98
template<class T> struct Atomic;

template<> struct Atomic<int> {
    Atomic() GX_NOEXCEPT : m_value() {}
    Atomic(int value) GX_NOEXCEPT : m_value(value) {}
    int fetch_add(int arg) {
#ifdef __GNUC__
        return __sync_fetch_and_add(&m_value, arg);
#endif
    }
    int fetch_sub(int arg) {
#ifdef __GNUC__
        return __sync_fetch_and_sub(&m_value, arg);
#endif
    }
    int operator+=(int arg) {
#ifdef __GNUC__
        return __sync_add_and_fetch(&m_value, arg);
#endif
    }
    int operator-=(int arg) {
#ifdef __GNUC__
        return __sync_sub_and_fetch(&m_value, arg);
#endif
    }
    int operator&=(int arg) {
#ifdef __GNUC__
        return __sync_and_and_fetch(&m_value, arg);
#endif
    }
    int operator|=(int arg) {
#ifdef __GNUC__
        return __sync_or_and_fetch(&m_value, arg);
#endif
    }
    operator int() const GX_NOEXCEPT { return m_value; }
    int operator++() GX_NOEXCEPT { return *this += 1; }
    int operator++(int) GX_NOEXCEPT { return fetch_add(1); }
    int operator--() GX_NOEXCEPT { return *this -= 1; }
    int operator--(int) GX_NOEXCEPT { return fetch_sub(1); }

private:
    int m_value;
};

template<> struct Atomic<unsigned int> {
    Atomic() GX_NOEXCEPT : m_value() {}
    Atomic(unsigned int value) GX_NOEXCEPT : m_value(value) {}
    unsigned int fetch_add(unsigned int arg) {
#ifdef __GNUC__
        return __sync_fetch_and_add(&m_value, arg);
#endif
    }
    unsigned int fetch_sub(unsigned int arg) {
#ifdef __GNUC__
        return __sync_fetch_and_sub(&m_value, arg);
#endif
    }
    unsigned int operator+=(unsigned int arg) {
#ifdef __GNUC__
        return __sync_add_and_fetch(&m_value, arg);
#endif
    }
    unsigned int operator-=(unsigned int arg) {
#ifdef __GNUC__
        return __sync_sub_and_fetch(&m_value, arg);
#endif
    }
    unsigned int operator&=(unsigned int arg) {
#ifdef __GNUC__
        return __sync_and_and_fetch(&m_value, arg);
#endif
    }
    unsigned int operator|=(unsigned int arg) {
#ifdef __GNUC__
        return __sync_or_and_fetch(&m_value, arg);
#endif
    }
    operator unsigned int() const GX_NOEXCEPT { return m_value; }
    unsigned int operator++() GX_NOEXCEPT { return *this += 1; }
    unsigned int operator++(int) GX_NOEXCEPT { return fetch_add(1); }
    unsigned int operator--() GX_NOEXCEPT { return *this -= 1; }
    unsigned int operator--(int) GX_NOEXCEPT { return fetch_sub(1); }

private:
    unsigned int m_value;
};
#else
template<class T> using Atomic = std::atomic<T>;
#endif

typedef Atomic<int> AtomicInt;
typedef Atomic<unsigned int> AtomicUInt;
} // namespace gx
