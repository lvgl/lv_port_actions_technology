#ifndef __ATT_DEBUG_H_
#define __ATT_DEBUG_H_

#include <stdio.h>
#include "att_prj_config.h"

#if (SUPPORT_DEBUG_LOG == 1)
/* DEBUG level */
#define DBG_ERROR           0
#define DBG_WARNING         1
#define DBG_INFO            2
#define DBG_LOG             3

#define DBG_LEVEL         DBG_LOG /*NOTE : DBG_INFO and DBG_WARNING are forbidden*/


#define _DBG_LOG_HDR(lvl_name)                              \
	LOG_FUNC("["lvl_name"] ")

#define dbg_log_line(lvl, fmt, ...)                         \
	do {                                                    \
		_DBG_LOG_HDR(lvl);                                  \
		LOG_FUNC(fmt, ##__VA_ARGS__);                         \
	}                                                       \
	while (0)

#define dbg_log_line_err(lvl, fmt, ...)                     \
	do {                                                    \
		_DBG_LOG_HDR(lvl);                                  \
		LOG_FUNC(fmt, ##__VA_ARGS__);                         \
	}                                                       \
	while (0)

#if (DBG_LEVEL >= DBG_LOG)
#define LOG_D(fmt, ...)      dbg_log_line("D", fmt, ##__VA_ARGS__)
#else
#define LOG_D(...)
#endif

#if (DBG_LEVEL >= DBG_INFO)
#define LOG_I(fmt, ...)      dbg_log_line("I", fmt, ##__VA_ARGS__)
#else
#define LOG_I(...)
#endif

#if (DBG_LEVEL >= DBG_WARNING)
#define LOG_W(fmt, ...)      dbg_log_line("W", fmt, ##__VA_ARGS__)
#else
#define LOG_W(...)
#endif

#if (DBG_LEVEL >= DBG_ERROR)
#define LOG_E(fmt, ...)      dbg_log_line_err("E", fmt, ##__VA_ARGS__)
#else
#define LOG_E(...)
#endif

#else

#define LOG_D(...)
#define LOG_I(...)
#define LOG_W(...)
#define LOG_E(...)

#endif

#endif /* __ATT_DEBUG_H_ */
