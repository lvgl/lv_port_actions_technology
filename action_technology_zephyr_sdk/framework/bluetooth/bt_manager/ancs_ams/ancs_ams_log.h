/*Copyright (c) 2018 Actions (Zhuhai) Technology
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __ANCS_AMS_LOG_H__
#define __ANCS_AMS_LOG_H__

#include <sys/printk.h>

#define ANCS_AMS_DEBUG_LOG		1

#if ANCS_AMS_DEBUG_LOG
#define ancs_log_inf(fmt, ...) \
		do {	\
			printk("AN I: "); \
			printk(fmt, ##__VA_ARGS__);	\
		} while (0)

#define ancs_log_warn(fmt, ...) \
		do {	\
			printk("AN W: "); \
			printk(fmt, ##__VA_ARGS__);	\
		} while (0)

#define ancs_log_err(fmt, ...) \
		do {	\
			printk("AN E: "); \
			printk(fmt, ##__VA_ARGS__);	\
		} while (0)

#define ams_log_inf(fmt, ...) \
		do {	\
			printk("AM I: "); \
			printk(fmt, ##__VA_ARGS__);	\
		} while (0)

#define ams_log_warn(fmt, ...) \
		do {	\
			printk("AM W: "); \
			printk(fmt, ##__VA_ARGS__);	\
		} while (0)

#define ams_log_err(fmt, ...) \
		do {	\
			printk("AM E: "); \
			printk(fmt, ##__VA_ARGS__);	\
		} while (0)

#define anam_log_inf(fmt, ...) \
		do {	\
			printk("ANAM I: "); \
			printk(fmt, ##__VA_ARGS__);	\
		} while (0)

#define anam_log_warn(fmt, ...) \
		do {	\
			printk("ANAM W: "); \
			printk(fmt, ##__VA_ARGS__);	\
		} while (0)

#define anam_log_err(fmt, ...) \
		do {	\
			printk("ANAM E: "); \
			printk(fmt, ##__VA_ARGS__);	\
		} while (0)

#define dis_log_inf(fmt, ...) \
		do {	\
			printk("DIS I: "); \
			printk(fmt, ##__VA_ARGS__);	\
		} while (0)

#define dis_log_warn(fmt, ...) \
		do {	\
			printk("DIS W: "); \
			printk(fmt, ##__VA_ARGS__);	\
		} while (0)

#define dis_log_err(fmt, ...) \
		do {	\
			printk("DIS E: "); \
			printk(fmt, ##__VA_ARGS__);	\
		} while (0)

#define gap_log_inf(fmt, ...) \
		do {	\
			printk("GA I: "); \
			printk(fmt, ##__VA_ARGS__);	\
		} while (0)

#define gap_log_warn(fmt, ...) \
		do {	\
			printk("GA W: "); \
			printk(fmt, ##__VA_ARGS__);	\
		} while (0)

#define gap_log_err(fmt, ...) \
		do {	\
			printk("GA E: "); \
			printk(fmt, ##__VA_ARGS__);	\
		} while (0)
#else
#define ancs_log_inf(fmt, ...)
#define ancs_log_warn(fmt, ...)
#define ancs_log_err(fmt, ...)

#define ams_log_inf(fmt, ...)
#define ams_log_warn(fmt, ...)
#define ams_log_err(fmt, ...)

#define anam_log_inf(fmt, ...)
#define anam_log_warn(fmt, ...)
#define anam_log_err(fmt, ...)

#define dis_log_inf(fmt, ...)
#define dis_log_warn(fmt, ...)
#define dis_log_err(fmt, ...)

#define gap_log_inf(fmt, ...)
#define gap_log_warn(fmt, ...)
#define gap_log_err(fmt, ...)
#endif
#endif