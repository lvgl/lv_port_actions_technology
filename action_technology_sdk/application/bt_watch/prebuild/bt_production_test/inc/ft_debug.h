#ifndef FT_DEBUG_H_
#define FT_DEBUG_H_

#include <stdio.h>

#define FT_DEBUG_LOG_EN   1 //是否打开调试打印

#if (FT_DEBUG_LOG_EN == 1)

/* DEBUG level */
#define FT_DBG_ERROR           0
#define FT_DBG_WARNING         1
#define FT_DBG_INFO            2
#define FT_DBG_LOG             3

#define FT_DBG_LEVEL         FT_DBG_LOG


#define ft_dbg_log_line(fmt, ...)                         \
	do {                                                       \
		self->ft_printf(fmt, ##__VA_ARGS__);                         \
	}                                                       \
	while (0)

#if (FT_DBG_LEVEL >= FT_DBG_LOG)
#define FT_LOG_D(fmt, ...)      ft_dbg_log_line(fmt, ##__VA_ARGS__)
#else
#define FT_LOG_D(...)
#endif

#if (FT_DBG_LEVEL >= FT_DBG_INFO)
#define FT_LOG_I(fmt, ...)      ft_dbg_log_line(fmt, ##__VA_ARGS__)
#else
#define FT_LOG_I(...)
#endif

#if (FT_DBG_LEVEL >= FT_DBG_WARNING)
#define FT_LOG_W(fmt, ...)      ft_dbg_log_line(fmt, ##__VA_ARGS__)
#else
#define FT_LOG_W(...)
#endif

#if (FT_DBG_LEVEL >= FT_DBG_ERROR)
#define FT_LOG_E(fmt, ...)      ft_dbg_log_line(fmt, ##__VA_ARGS__)
#else
#define FT_LOG_E(...)
#endif

#else

#define FT_LOG_D(...)
#define FT_LOG_I(...)
#define FT_LOG_W(...)
#define FT_LOG_E(...)

#endif
#endif /* FT_DEBUG_H_ */
