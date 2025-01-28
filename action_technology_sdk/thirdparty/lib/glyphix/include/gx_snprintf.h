/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_assert.h"
#include <stdarg.h>
#include <stddef.h>

#ifdef __cplusplus
namespace gx {
int snprintf(char *buf, size_t size, const char *fmt, ...);
int vsnprintf(char *buf, size_t size, const char *fmt, va_list arg);
} // namespace gx
#endif

#ifdef __cplusplus
extern "C" {
#endif

// NOLINTNEXTLINE(*-identifier-naming)
int gx_snprintf(char *buf, size_t size, const char *fmt, ...);
// NOLINTNEXTLINE(*-identifier-naming)
int gx_vsnprintf(char *buf, size_t size, const char *fmt, va_list arg);

#ifdef __cplusplus
}
#endif
