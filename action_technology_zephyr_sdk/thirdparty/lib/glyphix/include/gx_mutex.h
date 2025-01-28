/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_lockguard.h"
#include "platform/gx_thread.h"

namespace gx {
class Mutex : public NonCopyable<Mutex> {
public:
    typedef os::mutex_t native_handle_type; // NOLINT(readability-*)

    explicit Mutex(const char *name = nullptr) : m_mutex(os::mutex_create(name)) {}
    ~Mutex() { os::mutex_delete(m_mutex); }

    void lock() { os::mutex_take(m_mutex, -1); }
    bool try_lock() { return os::mutex_take(m_mutex, 0); } // NOLINT(readability-*)
    bool try_lock_for(int duration) {                      // NOLINT(readability-*)
        return os::mutex_take(m_mutex, duration);
    }
    void unlock() { os::mutex_release(m_mutex); }
    native_handle_type native_handle() { return m_mutex; } // NOLINT(readability-*)

private:
    os::mutex_t m_mutex;
};
} // namespace gx
