/*
 * Copyright (c) 2022 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file soc_sleep.S  sleep  for Actions SoC
 */

#include <toolchain.h>
#include <linker/sections.h>

 _ASM_FILE_PROLOGUE

GTEXT(__cpu_suspend)
GTEXT(__cpu_resume)

GDATA(soc_sleep_backup)

SECTION_FUNC(sleepfunc, __cpu_suspend)
	ldr ip, =soc_sleep_backup
	stmia ip!, {r0-r11}
	mov r0, sp
	mov r1, lr
	mrs r2, MSP
	mrs r3, PSP
	stmia ip!, {r0-r3}
	dsb
	wfi
	isb
	bx lr

SECTION_FUNC(sleepfunc, __cpu_resume)
	mrs r0, CONTROL
	movs r1, #2
	orrs r0, r1
	msr CONTROL, r0
	isb

	ldr ip, =soc_sleep_backup
	add ip, #48
	ldmia ip!, {r0-r3}
	mov sp, r0
	mov lr, r1
	msr MSP, r2
	msr PSP, r3
	ldr ip, =soc_sleep_backup
	ldmia ip!, {r0-r11}
	isb
	bx lr
