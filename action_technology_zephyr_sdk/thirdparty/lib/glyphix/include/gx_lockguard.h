/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_prelude.h"

namespace gx {
struct AdoptLock {};

template<class Lock> class LockGuard : NonCopyable<LockGuard<Lock> > {
public:
    typedef Lock lock_type;  // NOLINT(readability-*)
    typedef Lock mutex_type; // NOLINT(readability-*)

    explicit LockGuard(lock_type &lock) : m_lock(lock) { m_lock.lock(); }
    LockGuard(lock_type &lock, AdoptLock) : m_lock(lock) {}
    ~LockGuard() { m_lock.unlock(); }

private:
    lock_type &m_lock;
};
} // namespace gx
