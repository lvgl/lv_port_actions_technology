/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include <stddef.h>
#include <stdint.h>

namespace gx {
namespace os {
enum EventFlags { EVENT_AND = 1, EVENT_OR = 2, EVENT_CLEAR = 4 };

enum EventSetError {
    EVENT_EOK = 0,
    EVENT_ERROR,
    EVENT_ETIMEOUT,
    EVENT_EFULL,
    EVENT_EEMPTY,
    EVENT_ENOMEM,
    EVENT_ENOSYS,
    EVENT_EBUSY,
    EVENT_EIO,
    EVENT_EINTR,
    EVENT_EINVAL
};

typedef struct eventset *eventset_t; // NOLINT(*-identifier-naming)

eventset_t eventset_create(const char *name = nullptr); // NOLINT(*-identifier-naming)
int eventset_delete(eventset_t event);                  // NOLINT(*-identifier-naming)
// NOLINTNEXTLINE(*-identifier-naming)
int eventset_recv(eventset_t event, uint32_t set, int option, int timeout, uint32_t *recved);
int eventset_send(eventset_t event, uint32_t set); // NOLINT(*-identifier-naming)
}                                                  // namespace os
}                                                  // namespace gx
