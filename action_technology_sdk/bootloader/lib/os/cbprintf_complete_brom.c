/*
 * Copyright (c) 1997-2010, 2012-2015 Wind River Systems, Inc.
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <toolchain.h>
#include <sys/types.h>
#include <sys/util.h>
#include <sys/cbprintf.h>
#include "soc.h"

int cbvprintf(cbprintf_cb out, void *ctx, const char *fp, va_list ap)
{
	return pbrom_libc_api->p_cbvprintf(out, ctx, fp, ap);
}