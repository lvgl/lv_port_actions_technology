/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief string
 *
 * Memory and string code for libc.
 */

#include <toolchain.h>

_ASM_FILE_PROLOGUE

GTEXT(memcpy32)
GTEXT(memset32)

/**
 *
 * @brief Copy N * 32bytes in memory
 *
 * @return pointer to start of destination buffer
 */
SECTION_SUBSEC_FUNC(text,_libc_string_section,memcpy32)
	push {r4-r10}
memcpy32_loop:
	ldm r1!,{r3-r10}
	subs r2,r2,#1
	stm r0!,{r3-r10}
	cmp r2,#0
	bne memcpy32_loop
	pop {r4-r10}
	bx lr

/**
 *
 * @brief Set N * 32bytes in memory
 *
 * @return pointer to start of buffer
 */
SECTION_SUBSEC_FUNC(text,_libc_string_section,memset32)
	push {r4-r9}
	mov r3,r1
	mov r4,r1
	mov r5,r1
	mov r6,r1
	mov r7,r1
	mov r8,r1
	mov r9,r1
memset32_loop:
	subs r2,r2,#1
	stm r0!,{r1,r3-r9}
	cmp r2,#0
	bne memset32_loop
	pop {r4-r9}
	bx lr
