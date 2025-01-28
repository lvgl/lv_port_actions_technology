/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "platform/gx_time.h"

namespace gx {
struct TimeFields {
    int seconds;  /* Seconds.	[0-60] (1 leap second) */
    int minutes;  /* Minutes.	[0-59] */
    int hour;     /* Hours.	[0-23] */
    int monthDay; /* Day.		[1-31] */
    int month;    /* Month.	[0-11] */
    int year;     /* Year	- 1900.  */
    int weekDay;  /* Day of week.	[0-6] */
    int yearDay;  /* Days in year.[0-365]	*/
    int isDst;    /* DST.		[-1/0/1]*/
};

TimeFields gmtime(int64_t timestamp);
TimeFields localtime(int64_t timestamp);
} // namespace gx
