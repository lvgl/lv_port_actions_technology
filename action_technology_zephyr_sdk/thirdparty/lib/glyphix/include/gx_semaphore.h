/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "platform/gx_thread.h"

namespace gx {
class Semaphore {
public:
    explicit Semaphore(int value = 0, const char *name = nullptr)
        : m_semaphore(os::semaphore_create(value, name)) {}
    ~Semaphore() { os::semaphore_delete(m_semaphore); }
    void notify() { os::semaphore_release(m_semaphore); }
    void wait() { os::semaphore_wait(m_semaphore, -1); }
    bool wait_for(int msec) { // NOLINT(readability-*)
        return os::semaphore_wait(m_semaphore, msec);
    }

private:
    os::semaphore_t m_semaphore;
};
} // namespace gx
