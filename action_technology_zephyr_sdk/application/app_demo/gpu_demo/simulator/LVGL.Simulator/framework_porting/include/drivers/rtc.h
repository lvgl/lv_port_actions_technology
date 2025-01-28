/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef DRIVERS_RTC_H_
#define DRIVERS_RTC_H_

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*!
 * struct rtc_time
 * @brief The structure to discribe TIME
 */
struct rtc_time {
	uint8_t tm_sec; /* rtc second 0-59 */
	uint8_t tm_min; /* rtc minute 0-59 */
	uint8_t tm_hour; /* rtc hour 0-23 */
	uint8_t tm_mday; /* rtc day 1-31 */
	uint8_t tm_mon; /* rtc month 1-12 */
	uint8_t tm_wday;  /* day of the week  */
	uint16_t tm_year; /* rtc year */
	uint16_t tm_ms; /* rtc milisecond 0 ~ 999 */
};

#ifdef __cplusplus
}
#endif
/**
 * @}
 */

#endif /* DRIVERS_RTC_H_ */
