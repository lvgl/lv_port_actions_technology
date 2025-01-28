/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_os.h"

namespace gx {
namespace os {
uint32_t clock_ms();     // NOLINT(readability-*)
uint64_t clock_us();     // NOLINT(readability-*)
void delay_ms(int time); // NOLINT(readability-*)

//! Get UTC timestamps with millisecond precision.
int64_t timestamp_ms(); // NOLINT(readability-*)

//! Get UTC timestamps with second precision.
inline int64_t timestamp() { return timestamp_ms() / 1000; }

//! Gets the time-zone difference between UTC time (second precision) and local time in
//! seconds.
int timezone_offset(int64_t timestamp); // NOLINT(readability-*)
} // namespace os
} // namespace gx
