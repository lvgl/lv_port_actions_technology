/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file cbuf interface
 */

#include <kernel.h>
#include <string.h>



#ifdef CONFIG_ARM_UNWIND

void unwind_backtrace(struct k_thread *th);
static void stack_dump(const struct k_thread *th)
{
	unwind_backtrace((struct k_thread *)th);
}
#else
static void stack_dump(const struct k_thread *th)
{
	const z_arch_esf_t *esf;
	const struct _callee_saved *callee = &th->callee_saved;
	esf = (z_arch_esf_t *)callee->psp;
	printk("############ thread: %s info############\n", thread_get_name(th));
	printk("r0/a1:  0x%08x  r1/a2:  0x%08x  r2/a3:  0x%08x\n",
		esf->basic.a1, esf->basic.a2, esf->basic.a3);
	printk("r3/a4:  0x%08x r12/ip:  0x%08x r14/lr:  0x%08x\n",
		esf->basic.a4, esf->basic.ip, esf->basic.lr);
	printk("r4/v1:  0x%08x  r5/v2:  0x%08x  r6/v3:  0x%08x\n",
		callee->v1, callee->v2, callee->v3);
	printk("r7/v4:  0x%08x  r8/v5:  0x%08x  r9/v6:  0x%08x\n",
		callee->v4, callee->v5, callee->v6);
	printk("r10/v7: 0x%08x  r11/v8: 0x%08x    psp:  0x%08x\n",
			callee->v7, callee->v8, callee->psp);
	printk(" xpsr:  0x%08x\n", esf->basic.xpsr);
	printk("(r15/pc): 0x%08x\n",	esf->basic.pc);

}
#endif

static void thread_show_info(const struct k_thread *cthread, void *user_data)
{
	const struct k_thread *cur = (const struct k_thread *)user_data;
	if(cur == cthread)
		return;
	stack_dump(cthread);
}

void show_stack(void)
{
	struct k_thread *cur = (struct k_thread *) k_current_get();

	printk("****show thread stack cur=%s ****\n", k_thread_name_get(cur));	
	k_thread_foreach(thread_show_info, NULL);
}


