/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#if !defined(GX_ENABLE_SOURCE) && !defined(GX_META_VERSION)
#include "gx_config_export.h"
#endif

#ifndef GX_META_VERSION
#include "gx_platform_config.h"
#endif

/* -------- start of assert macros ---------- */
#ifndef GX_ASSERT
#ifdef GX_ASSERT_HANDLER

#ifdef __cplusplus
extern "C" {
#endif
void GX_ASSERT_HANDLER(const char *message, const char *file, unsigned line);
#ifdef __cplusplus
}
#endif

#define GX_ASSERT(expression)                                                                      \
    (void)((!!(expression)) ||                                                                     \
           (GX_ASSERT_HANDLER((#expression), (__FILE__), (unsigned)(__LINE__)), 0))
#else
#define GX_ASSERT(expr) ((void)0)
#endif
#endif

#ifdef GX_NO_DEBUG
#undef GX_ASSERT
#define GX_ASSERT(expr) ((void)0)
#endif

#ifdef GX_ENABLE_CRT_CHECK
#define GX_CRT_ASSERT(expr) GX_ASSERT(expr)
#else
#define GX_CRT_ASSERT(expr) ((void)0)
#endif
/* --------- end of assert macros ----------- */
