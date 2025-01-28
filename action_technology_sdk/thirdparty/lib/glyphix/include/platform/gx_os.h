/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_prelude.h"

namespace gx {
namespace os {
enum { FT_REG = 0, FT_DIR = 1 };

typedef struct file_handle *file_t;           // NOLINT
typedef struct mutex_handle *mutex_t;         // NOLINT
typedef struct semaphore_handle *semaphore_t; // NOLINT
typedef struct timer_handle *timer_t;         // NOLINT

enum TimerFlag { TimerFlagOneShot, TimerFlagPeriodic };

struct dirinfo_t { // NOLINT
    void *dir;
    void *item;
    int type;
    const char *name;
};

struct thread_attr { // NOLINT
    const char *name;
    int stack_size; // NOLINT
    int priority;
};

#ifdef GX_LARGE_FILE_SUPPORT
typedef long long off_t; // NOLINT
#else
typedef long off_t; // NOLINT
#endif
} // namespace os
} // namespace gx
