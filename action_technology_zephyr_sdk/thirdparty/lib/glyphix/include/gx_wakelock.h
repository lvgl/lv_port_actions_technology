/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "platform/gx_hw.h"

namespace gx {
class WakeLock {
public:
    WakeLock() { os::system_wake_lock(); }
    ~WakeLock() { os::system_wake_unlock(); }
};
} // namespace gx
