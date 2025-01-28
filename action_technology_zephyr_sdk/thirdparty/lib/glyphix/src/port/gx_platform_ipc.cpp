/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#include "gx_logger.h"
#include "kernel.h"
#include "platform/gx_thread.h"

namespace gx {
namespace os {

static k_mutex *mutex_cast(mutex_t mutex) {
    return reinterpret_cast<k_mutex *>(mutex);
}

static k_sem *semaphore_cast(semaphore_t semaphore) {
    return reinterpret_cast<k_sem *>(semaphore);
}

mutex_t mutex_create(const char *) {
    k_mutex *mutex = static_cast<k_mutex *>(k_malloc(sizeof(k_mutex)));
    if (mutex) {
        k_mutex_init(mutex);
    } else {
        LogError() << "mutex_create NO MEM";
        GX_ASSERT(0);
    }
    return mutex_t(mutex);
}

void mutex_delete(mutex_t mutex) { k_free(mutex_cast(mutex)); }

bool mutex_take(mutex_t mutex, int time) {
    int ret = k_mutex_lock(mutex_cast(mutex), SYS_TIMEOUT_MS(time));
    return ret == 0;
}

void mutex_release(mutex_t mutex) { k_mutex_unlock(mutex_cast(mutex)); }

semaphore_t semaphore_create(int value, const char *) {
    struct k_sem *sem = new k_sem;
    k_sem_init(sem, value, 100);
    return semaphore_t(sem);
}

void semaphore_delete(semaphore_t semaphore) {
    if (k_sem_count_get(semaphore_cast(semaphore)) == 0) {
        k_sem_reset(semaphore_cast(semaphore));
        delete semaphore_cast(semaphore);
    } else
        LogError() << "semaphore_delete failed:" << semaphore;
}

bool semaphore_wait(semaphore_t semaphore, int time) {
    int value = k_sem_take(semaphore_cast(semaphore), SYS_TIMEOUT_MS(time));
    return value == 0;
}

void semaphore_release(semaphore_t semaphore) {
    k_sem_give(semaphore_cast(semaphore));
}
} // namespace os
} // namespace gx
