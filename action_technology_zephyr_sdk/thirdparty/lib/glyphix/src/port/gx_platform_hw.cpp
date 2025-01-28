/*
 * This file is part of PersimUI
 * Copyright (c) 2006-2023, RT-Thread Development Team
 */
#include "gx_mutex.h"
#include "platform/gx_hw.h"
#include <sys_wakelock.h>

namespace gx {
namespace os {
Mutex interruptMutex;

size_t critical_lock() {
    interruptMutex.lock();
    return 0;
}

void critical_unlock(size_t) { interruptMutex.unlock(); }

void system_wake_lock() {
    sys_wake_lock(PARTIAL_WAKE_LOCK);
}

void system_wake_unlock() {
    sys_wake_unlock(PARTIAL_WAKE_LOCK);
}
} // namespace os
} // namespace gx
