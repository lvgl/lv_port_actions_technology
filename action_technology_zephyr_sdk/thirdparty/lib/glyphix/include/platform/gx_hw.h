/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_os.h"
#include <cstddef>

namespace gx {
namespace os {
std::size_t critical_lock();        // NOLINT(*-identifier-naming)
void critical_unlock(size_t level); // NOLINT(*-identifier-naming)
void system_wake_lock();            // NOLINT(*-identifier-naming)
void system_wake_unlock();          // NOLINT(*-identifier-naming)
} // namespace os
} // namespace gx
