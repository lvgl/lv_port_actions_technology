/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
/*
 * Use the following macro definition to enable debugging logs.
 *
 * #define LOG_TAG            "tmp.tag"
 * #define LOG_LVL            LOG_INFO
 * #include <gx_clog.h>
 *
 * LOG_D("This is debug level log.");
 * LOG_I("This is infor level log.");
 */

#ifndef GX_CLOG_H_
#define GX_CLOG_H_

#include "gx_snprintf.h"
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#define GX_DEBUG
#ifndef GX_PRINT
#define GX_PRINT(...) gx_printf(__VA_ARGS__)
#endif

/* the debug log will force enable when RT_DEBUG macro is defined */
#if defined(GX_DEBUG) && !defined(LOG_ENABLE)
#define LOG_ENABLE
#endif

/* it will force output color log when RT_DEBUG_COLOR macro is defined */
#if defined(GX_DEBUG_COLOR) && !defined(LOG_COLOR)
#define LOG_COLOR
#endif

/* gx log level */
#define LOG_ERROR   (0)
#define LOG_WARNING (1)
#define LOG_INFO    (2)
#define LOG_DEBUG   (3)

#ifdef LOG_TAG
#ifndef LOG_SECTION_NAME
#define LOG_SECTION_NAME LOG_TAG
#endif
#else
/* compatible with old version */
#ifndef LOG_SECTION_NAME
#define LOG_SECTION_NAME "NO_TAG"
#endif
#endif /* LOG_TAG */

#ifdef LOG_ENABLE

#ifdef LOG_LVL
#ifndef LOG_LEVEL
#define LOG_LEVEL LOG_LVL
#endif
#else
/* compatible with old version */
#ifndef LOG_LEVEL
#define LOG_LEVEL LOG_DEBUG
#endif
#endif /* LOG_LVL */

/*
 * The color for terminal (foreground)
 * BLACK    30
 * RED      31
 * GREEN    32
 * YELLOW   33
 * BLUE     34
 * PURPLE   35
 * CYAN     36
 * WHITE    37
 */
#ifdef LOG_COLOR
#define _LOG_COLOR(n) GX_PRINT("\033[" #n "m")
#define _LOG_COLOR_HDR(lvl_name, color_n)                                                          \
    GX_PRINT("\033[" #color_n "m[" lvl_name "/" DBG_SECTION_NAME "] ")
#define _DBG_LOG_X_END GX_PRINT("\033[0m\n")
#else
#define _LOG_COLOR(n)
#define _LOG_COLOR_HDR(lvl_name, color_n) GX_PRINT("[" lvl_name "/" LOG_SECTION_NAME "] ")
#define _LOG_COLOR_END                    GX_PRINT("\n")
#endif /* LOG_COLOR */

/*
 * static debug routine
 * NOTE: This is a NOT RECOMMENDED API. Please using LOG_X API.
 *       It will be DISCARDED later. Because it will take up more resources.
 */
#define dbg_log(level, fmt, ...)                                                                   \
    if ((level) <= LOG_LEVEL) {                                                                    \
        switch (level) {                                                                           \
        case DBG_ERROR: _LOG_COLOR_HDR("E", 31); break;                                            \
        case DBG_WARNING: _LOG_COLOR_HDR("W", 33); break;                                          \
        case DBG_INFO: _LOG_COLOR_HDR("I", 32); break;                                             \
        case DBG_LOG: _LOG_COLOR_HDR("D", 0); break;                                               \
        default: break;                                                                            \
        }                                                                                          \
        GX_PRINT(fmt, ##__VA_ARGS__);                                                              \
        _LOG_COLOR(0);                                                                             \
    }

#define dbg_here                                                                                   \
    if ((LOG_LEVEL) <= DBG_LOG) {                                                                  \
        GX_PRINT(DBG_SECTION_NAME " Here %s:%d\n", __FUNCTION__, __LINE__);                        \
    }

#define dbg_log_line(lvl, color_n, fmt, ...)                                                       \
    do {                                                                                           \
        _LOG_COLOR_HDR(lvl, color_n);                                                              \
        GX_PRINT(fmt, ##__VA_ARGS__);                                                              \
        _LOG_COLOR_END;                                                                            \
    } while (0)

#define dbg_raw(...) GX_PRINT(__VA_ARGS__);

#else
#define dbg_log(level, fmt, ...)
#define dbg_here
#define dbg_enter
#define dbg_exit
#define dbg_log_line(lvl, color_n, fmt, ...)
#define dbg_raw(...)
#endif /* LOG_ENABLE */

#if (LOG_LEVEL >= LOG_DEBUG)
#define LOG_D(fmt, ...) dbg_log_line("D", 0, fmt, ##__VA_ARGS__)
#else
#define LOG_D(...)
#endif

#if (LOG_LEVEL >= LOG_INFO)
#define LOG_I(fmt, ...) dbg_log_line("I", 32, fmt, ##__VA_ARGS__)
#else
#define LOG_I(...)
#endif

#if (LOG_LEVEL >= LOG_WARNING)
#define LOG_W(fmt, ...) dbg_log_line("W", 33, fmt, ##__VA_ARGS__)
#else
#define LOG_W(...)
#endif

#if (LOG_LEVEL >= LOG_ERROR)
#define LOG_E(fmt, ...) dbg_log_line("E", 31, fmt, ##__VA_ARGS__)
#else
#define LOG_E(...)
#endif

#define LOG_RAW(...) dbg_raw(__VA_ARGS__)

#ifdef __cplusplus
}
#endif

#endif /* GX_CLOG_H_ */
