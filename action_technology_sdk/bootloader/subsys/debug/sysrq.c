/*
 * Copyright (c) 2017 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Magic System Request Key Hacks
 *
 */

#include <kernel.h>
#include <drivers/uart.h>
#include <soc.h>

struct sysrq_key_op {
	char key;
	char reserved[3];

	void (*handler)(int);
	char *help_msg;
	char *action_msg;
};

#if CONFIG_KERNEL_SHOW_STACK
extern void show_stack(void);
static void sysrq_handle_show_stack(int key)
{
	show_stack();
}
#endif


static void sysrq_handle_change_jtag(int key)
{
	jtag_set();
}



#if defined(CONFIG_INIT_STACKS) && defined(CONFIG_THREAD_STACK_INFO) &&	defined(CONFIG_THREAD_MONITOR)
static void sysrq_stack_dump(const struct k_thread *thread, void *user_data)
{
	unsigned int pcnt;
	size_t unused;
	size_t size = thread->stack_info.size;
	const char *tname;
	int ret;
	ret = k_thread_stack_space_get(thread, &unused);
	if (ret) {
		printk("Unable to determine unused stack size (%d)\n", ret);
		return;
	}
	tname = k_thread_name_get((struct k_thread *)thread);
	/* Calculate the real size reserved for the stack */
	pcnt = ((size - unused) * 100U) / size;

	printk("%p %-10s (real size %u):\tunused %u\tusage %u / %u (%u %%)\n",
		      thread,
		      tname ? tname : "NA",
		      size, unused, size - unused, size, pcnt);
}

static void sysrq_handle_stack_dump(int key)
{
	k_thread_foreach(sysrq_stack_dump, NULL);
}
#endif


static const struct sysrq_key_op sysrq_key_table[] = {
#ifdef CONFIG_KERNEL_SHOW_STACK
	{
		.key		= 't',
		.handler	= sysrq_handle_show_stack,
		.help_msg	= "show-thread-states(t)",
		.action_msg	= "Show State:",
	},
#endif
	{
		.key		= 'j',
		.handler	= sysrq_handle_change_jtag,
		.help_msg	= "change jtag to grp1 (j)",
		.action_msg = "jtag:",
	},

#if defined(CONFIG_INIT_STACKS) && defined(CONFIG_THREAD_STACK_INFO) &&	defined(CONFIG_THREAD_MONITOR)
	{
		.key		= 's',
		.handler	= sysrq_handle_stack_dump,
		.help_msg	= "show thread stack usage (s)",
		.action_msg	= "stack usage:",
	},
#endif

};

/* magic sysrq key: CTLR + 'b' 'r' 'e' 'a' 'k' */
static const char sysrq_breakbuf[] = {0x02, 0x12, 0x05, 0x01, 0x0b};
static int sysrq_break_idx;

static struct sysrq_key_op* sysrq_get_key_op(char key)
{
	int i, cnt;

	cnt = ARRAY_SIZE(sysrq_key_table);

	for (i = 0; i < cnt; i++) {
		if (key == sysrq_key_table[i].key) {
			return (struct sysrq_key_op*)&sysrq_key_table[i];
		}
	}

	return NULL;
}

static void sysrq_print_help(void)
{
	int i;

	printk("HELP : quit(q) \n");
	for (i = 0; i < ARRAY_SIZE(sysrq_key_table); i++) {
		printk("%s \n", sysrq_key_table[i].help_msg);
	}
	printk("\n");
}

extern void printk_dma_switch(int sw_dma);
static void handle_sysrq_main(const struct device * port)
{
	struct sysrq_key_op* op_p;
	char key;
#ifdef CONFIG_ACTIONS_PRINTK_DMA
	printk_dma_switch(0);
#endif
	for(;;) {
		printk("Wait SYSRQ Key:\n");
		sysrq_print_help();

		uart_poll_in(port, &key);
		if (key == 'q') {
			printk("sysrq: exit\n");
			break;
		}

		op_p = sysrq_get_key_op(key);
		if (op_p) {
			printk("%s\n", op_p->action_msg);
			op_p->handler(key);
		} else {
			sysrq_print_help();
		}
	}
#ifdef CONFIG_ACTIONS_PRINTK_DMA
	printk_dma_switch(1);
#endif
}

void uart_handle_sysrq_char(const struct device * port, char c)
{
	if (c == sysrq_breakbuf[sysrq_break_idx]) {
		sysrq_break_idx++;
		if (sysrq_break_idx == sizeof(sysrq_breakbuf)) {
			sysrq_break_idx = 0;
			handle_sysrq_main(port);
		}
	} else {
		sysrq_break_idx = 0;
	}
}
