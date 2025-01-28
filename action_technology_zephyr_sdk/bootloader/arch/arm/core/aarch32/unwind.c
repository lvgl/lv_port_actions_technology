/*
 * arch/arm/kernel/unwind.c
 *
 * Copyright (C) 2008 ARM Limited
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 *
 * Stack unwinding support for ARM
 *
 * An ARM EABI version of gcc is required to generate the unwind
 * tables. For information about the structure of the unwind tables,
 * see "Exception Handling ABI for the ARM Architecture" at:
 *
 * http://infocenter.arm.com/help/topic/com.arm.doc.subset.swdev.abi/index.html
 */
#include <kernel.h>
#include <kernel_internal.h>
#include <linker/sections.h>
#include <linker/linker-defs.h>
#include <logging/log.h>
#include <arch/arm/aarch32/cortex_m/cmsis.h>
#include <kallsyms.h>
#include <soc.h>


LOG_MODULE_REGISTER(unwind, LOG_LEVEL_INF);


struct unwind_ctrl_block {
	unsigned long vrs[16];		/* virtual register set */
	const unsigned long *insn;	/* pointer to the current instructions word */
	int entries;			/* number of entries left to interpret */
	int byte;			/* current byte number in the instructions word */
};

enum regs {
	FP = 11,
	SP = 13,
	LR = 14,
	PC = 15
};

struct stackframe {
	unsigned long fp;
	unsigned long sp;
	unsigned long lr;
	unsigned long pc;
};



enum unwind_reason_code {
	URC_OK = 0,			/* operation completed successfully */
	URC_CONTINUE_UNWIND = 8,
	URC_FAILURE = 9			/* unspecified failure of some kind */
};

struct unwind_idx {
	unsigned long addr_offset;
	unsigned long insn;
};

struct unwind_table {
	//struct list_head list;
	const struct unwind_idx *start;
	const struct unwind_idx *origin;
	const struct unwind_idx *stop;
	unsigned long begin_addr;
	unsigned long end_addr;
};



extern const struct unwind_idx __start_unwind_idx[];
static const struct unwind_idx *__origin_unwind_idx;
extern const struct unwind_idx __stop_unwind_idx[];

int core_kernel_text(unsigned long addr)
{
	if (addr >= (unsigned long)__text_region_start &&
	    addr <= (unsigned long)__text_region_end)
		return 1;
	return 0;
}


/* Convert a prel31 symbol to an absolute address */
#define prel31_to_addr(ptr)				\
({							\
	/* sign-extend to 32 bits */			\
	long offset = (((long)*(ptr)) << 1) >> 1;	\
	(unsigned long)(ptr) + offset;			\
})


#if defined(CONFIG_THREAD_STACK_INFO)
static unsigned int g_cur_thread_top;
static void unwind_stack_end_set(const struct k_thread *th)
{
	g_cur_thread_top = th->stack_info.start + th->stack_info.size;
}
static unsigned int unwind_stack_end_get(unsigned int sp)
{
	if(g_cur_thread_top) {
		if(sp > g_cur_thread_top)
			return sp ;
		else
			return g_cur_thread_top;
	} else {
		return sp+0x100;
	}
}
static void unwind_stack_end_clear(void)
{
	g_cur_thread_top = 0;
}

#else
static unsigned int g_cur_thread_top;
static void unwind_stack_end_set(const struct k_thread *th)
{

}
static unsigned int unwind_stack_end_get(unsigned int sp)
{
	return sp+0x100;
}

static void unwind_stack_end_clear(void)
{

}
#endif


/*
 * Binary search in the unwind index. The entries are
 * guaranteed to be sorted in ascending order by the linker.
 *
 * start = first entry
 * origin = first entry with positive offset (or stop if there is no such entry)
 * stop - 1 = last entry
 */
static const struct unwind_idx *search_index(unsigned long addr,
				       const struct unwind_idx *start,
				       const struct unwind_idx *origin,
				       const struct unwind_idx *stop)
{
	unsigned long addr_prel31;

	LOG_DBG("%s(%08lx, %p, %p, %p)\n",
			__func__, addr, start, origin, stop);

	/*
	 * only search in the section with the matching sign. This way the
	 * prel31 numbers can be compared as unsigned longs.
	 */
	if (addr < (unsigned long)start)
		/* negative offsets: [start; origin) */
		stop = origin;
	else
		/* positive offsets: [origin; stop) */
		start = origin;

	/* prel31 for address relavive to start */
	addr_prel31 = (addr - (unsigned long)start) & 0x7fffffff;

	while (start < stop - 1) {
		const struct unwind_idx *mid = start + ((stop - start) >> 1);

		/*
		 * As addr_prel31 is relative to start an offset is needed to
		 * make it relative to mid.
		 */
		if (addr_prel31 - ((unsigned long)mid - (unsigned long)start) <
				mid->addr_offset)
			stop = mid;
		else {
			/* keep addr_prel31 relative to start */
			addr_prel31 -= ((unsigned long)mid -
					(unsigned long)start);
			start = mid;
		}
	}

	if (start->addr_offset <= addr_prel31)
		return start;
	else {
		LOG_WRN("unwind: Unknown symbol address %08lx\n", addr);
		return NULL;
	}
}

static const struct unwind_idx *unwind_find_origin(
		const struct unwind_idx *start, const struct unwind_idx *stop)
{
	LOG_DBG("%s(%p, %p)\n", __func__, start, stop);
	while (start < stop) {
		const struct unwind_idx *mid = start + ((stop - start) >> 1);

		if (mid->addr_offset >= 0x40000000)
			/* negative offset */
			start = mid + 1;
		else
			/* positive offset */
			stop = mid;
	}
	LOG_DBG("%s -> %p\n", __func__, stop);
	return stop;
}

static const struct unwind_idx *unwind_find_idx(unsigned long addr)
{
	const struct unwind_idx *idx = NULL;
	//unsigned long flags;

	LOG_DBG("%s(%08lx)\n", __func__, addr);

	if (core_kernel_text(addr)) {
		if (!__origin_unwind_idx)
			__origin_unwind_idx =
				unwind_find_origin(__start_unwind_idx,
						__stop_unwind_idx);

		/* main unwind table */
		idx = search_index(addr, __start_unwind_idx,
				   __origin_unwind_idx,
				   __stop_unwind_idx);
	} else {
		LOG_ERR("err: pc=0x%08lx\n", addr);
		return NULL;
	}

	LOG_DBG("%s: idx = %p\n", __func__, idx);
	return idx;
}

static unsigned long unwind_get_byte(struct unwind_ctrl_block *ctrl)
{
	unsigned long ret;

	if (ctrl->entries <= 0) {
		LOG_WRN("unwind: Corrupt unwind table\n");
		return 0;
	}

	ret = (*ctrl->insn >> (ctrl->byte * 8)) & 0xff;

	if (ctrl->byte == 0) {
		ctrl->insn++;
		ctrl->entries--;
		ctrl->byte = 3;
	} else
		ctrl->byte--;

	return ret;
}

/*
 * Execute the current unwind instruction.
 */
static int unwind_exec_insn(struct unwind_ctrl_block *ctrl)
{
	unsigned long insn = unwind_get_byte(ctrl);

	LOG_DBG("%s: insn = %08lx\n", __func__, insn);

	if ((insn & 0xc0) == 0x00)
		ctrl->vrs[SP] += ((insn & 0x3f) << 2) + 4;
	else if ((insn & 0xc0) == 0x40)
		ctrl->vrs[SP] -= ((insn & 0x3f) << 2) + 4;
	else if ((insn & 0xf0) == 0x80) {
		unsigned long mask;
		unsigned long *vsp = (unsigned long *)ctrl->vrs[SP];
		int load_sp, reg = 4;

		insn = (insn << 8) | unwind_get_byte(ctrl);
		mask = insn & 0x0fff;
		if (mask == 0) {
			LOG_WRN("unwind: 'Refuse to unwind' instruction %04lx\n",
				   insn);
			return -1;
		}

		/* pop R4-R15 according to mask */
		load_sp = mask & (1 << (13 - 4));
		while (mask) {
			if (mask & 1)
				ctrl->vrs[reg] = *vsp++;
			mask >>= 1;
			reg++;
		}
		if (!load_sp)
			ctrl->vrs[SP] = (unsigned long)vsp;
	} else if ((insn & 0xf0) == 0x90 &&
		   (insn & 0x0d) != 0x0d)
		ctrl->vrs[SP] = ctrl->vrs[insn & 0x0f];
	else if ((insn & 0xf0) == 0xa0) {
		unsigned long *vsp = (unsigned long *)ctrl->vrs[SP];
		int reg;

		/* pop R4-R[4+bbb] */
		for (reg = 4; reg <= 4 + (insn & 7); reg++)
			ctrl->vrs[reg] = *vsp++;
		if (insn & 0x80)
			ctrl->vrs[14] = *vsp++;
		ctrl->vrs[SP] = (unsigned long)vsp;
	} else if (insn == 0xb0) {
		if (ctrl->vrs[PC] == 0)
			ctrl->vrs[PC] = ctrl->vrs[LR];
		/* no further processing */
		ctrl->entries = 0;
	} else if (insn == 0xb1) {
		unsigned long mask = unwind_get_byte(ctrl);
		unsigned long *vsp = (unsigned long *)ctrl->vrs[SP];
		int reg = 0;

		if (mask == 0 || mask & 0xf0) {
			LOG_WRN("unwind: Spare encoding %04lx\n",
			       (insn << 8) | mask);
			return -1;
		}

		/* pop R0-R3 according to mask */
		while (mask) {
			if (mask & 1)
				ctrl->vrs[reg] = *vsp++;
			mask >>= 1;
			reg++;
		}
		ctrl->vrs[SP] = (unsigned long)vsp;
	} else if (insn == 0xb2) {
		unsigned long uleb128 = unwind_get_byte(ctrl);

		ctrl->vrs[SP] += 0x204 + (uleb128 << 2);
	} else {
		LOG_WRN("unwind: Unhandled instruction %02lx\n", insn);
		return -1;
	}

	LOG_DBG("%s: fp = %08lx sp = %08lx lr = %08lx pc = %08lx\n", __func__,
		 ctrl->vrs[FP], ctrl->vrs[SP], ctrl->vrs[LR], ctrl->vrs[PC]);

	return 0;
}

/*
 * Unwind a single frame starting with *sp for the symbol at *pc. It
 * updates the *pc and *sp with the new values.
 */
static int unwind_frame(struct stackframe *frame)
{
	unsigned long high, low;
	const struct unwind_idx *idx;
	struct unwind_ctrl_block ctrl;

	/* only go to a higher address on the stack */
	low = frame->sp;
	high = unwind_stack_end_get(low);

	LOG_DBG("%s(pc = %08lx lr = %08lx sp = %08lx)\n", __func__,
		 frame->pc, frame->lr, frame->sp);

	if (!core_kernel_text(frame->pc))
		return -1;

	idx = unwind_find_idx(frame->pc);
	if (!idx) {
		LOG_WRN("unwind: Index not found %08lx\n", frame->pc);
		return -1;
	}

	ctrl.vrs[FP] = frame->fp;
	ctrl.vrs[SP] = frame->sp;
	ctrl.vrs[LR] = frame->lr;
	ctrl.vrs[PC] = 0;

	if (idx->insn == 1)
		/* can't unwind */
		return -1;
	else if ((idx->insn & 0x80000000) == 0)
		/* prel31 to the unwind table */
		ctrl.insn = (unsigned long *)prel31_to_addr(&idx->insn);
	else if ((idx->insn & 0xff000000) == 0x80000000)
		/* only personality routine 0 supported in the index */
		ctrl.insn = &idx->insn;
	else {
		LOG_WRN("unwind: Unsupported personality routine %08lx in the index at %p\n",
			   idx->insn, idx);
		return -1;
	}

	/* check the personality routine */
	if ((*ctrl.insn & 0xff000000) == 0x80000000) {
		ctrl.byte = 2;
		ctrl.entries = 1;
	} else if ((*ctrl.insn & 0xff000000) == 0x81000000) {
		ctrl.byte = 1;
		ctrl.entries = 1 + ((*ctrl.insn & 0x00ff0000) >> 16);
	} else {
		LOG_WRN("unwind: Unsupported personality routine %08lx at %p\n",
			   *ctrl.insn, ctrl.insn);
		return -1;
	}

	while (ctrl.entries > 0) {
		int urc = unwind_exec_insn(&ctrl);
		if (urc < 0)
			return urc;
		if (ctrl.vrs[SP] < low || ctrl.vrs[SP] >= high)
			return -1;
	}

	if (ctrl.vrs[PC] == 0)
		ctrl.vrs[PC] = ctrl.vrs[LR];

	/* check for infinite loop */
	if (frame->pc == ctrl.vrs[PC])
		return -1;

	frame->fp = ctrl.vrs[FP];
	frame->sp = ctrl.vrs[SP];
	frame->lr = ctrl.vrs[LR];
	frame->pc = ctrl.vrs[PC];

	return 0;
}




static void unwind_print_backtrace(unsigned long where, unsigned long pc)
{
#ifdef CONFIG_KALLSYMS
	char buffer[KSYM_SYMBOL_LEN+1];
	sprint_backtrace(buffer, where);
	printk("[<%08lx>] (%s) from ", where, buffer);
	sprint_backtrace(buffer, pc);
	printk("[<%08lx>] (%s)\n", pc, buffer);
#else
	printk("Function entered at [PC <%08lx>] from [PC <%08lx>]\n", where, pc);
#endif
}

static void unwind_backtrace_print(struct stackframe *frame)
{
	unsigned long where;
	int urc;
	bool b_one = false;

	while (1) {
		where = frame->pc;
		urc = unwind_frame(frame);
		if (urc < 0)
			break;
		unwind_print_backtrace(where, frame->pc);
		b_one = true;
	}

	if(!b_one)
		unwind_print_backtrace(where, frame->lr);

	unwind_stack_end_clear();
}



static void unwind_print_thread_info(struct k_thread *th)
{
	const char *tname;
	tname = k_thread_name_get(th);
	printk("\n %s%p %s state: %s backtrace \n", ((th == k_current_get()) ? "*" : " "),
		th, (tname?tname : "NA"), k_thread_state_str(th));
}



uint32_t __get_PC(void)
{
	uint32_t result;
	__asm__ volatile ("mov %0, pc"  : "=r" (result) );
	return(result);
}
uint32_t __get_LR(void)
{
	uint32_t result;
	__asm__ volatile ("mov %0, lr"  : "=r" (result) );
	return(result);
}

uint32_t __get_FP(void)
{
	uint32_t result;
	__asm__ volatile ("mov %0, r11"  : "=r" (result) );
	return(result);
}

static void unwind_dump_mem(const char *msg, uint32_t mem_addr, int len)
{
	int i;
	uint32_t *ptr = (uint32_t *)mem_addr;
	printk("%s=0x%08x\n", msg, mem_addr);
	for(i = 0; i < len/4; i++)
	{
		printk("%08x ", ptr[i]);
		if(i % 8 == 7)
			printk("\n");
	}
	printk("\n");
}

void unwind_backtrace_cur_without_sp(void)
{
	struct stackframe frame;
	uint32_t sp;
	if(k_is_in_isr()){
		frame.sp = __get_MSP();
	}else{
		frame.sp = __get_PSP();
	}
	sp = frame.sp;
	frame.lr = __get_LR();
	frame.pc = __get_PC();
	frame.fp =  __get_PC();
	printk("show cur stack:\n");
	g_cur_thread_top = frame.sp + 1024;
	unwind_backtrace_print(&frame);

}


void unwind_backtrace_cur(void)
{
	struct stackframe frame;
	uint32_t sp;
	if(k_is_in_isr()){
		frame.sp = __get_MSP();
	}else{
		frame.sp = __get_PSP();
	}
	sp = frame.sp;
	frame.lr = __get_LR();
	frame.pc = __get_PC();
	frame.fp =  __get_PC();
	printk("show cur stack:\n");
	g_cur_thread_top = frame.sp + 1024;
	unwind_backtrace_print(&frame);
	unwind_dump_mem("sp=", sp, 256);
}


void unwind_backtrace(struct k_thread *th)
{
	struct stackframe frame;
	const z_arch_esf_t *esf;
	const struct _callee_saved *callee;
	uint32_t sp;

	if(th == NULL)
		th = k_current_get();

	unwind_stack_end_set(th);
	unwind_print_thread_info(th);
	if(th == k_current_get()) {
		if(k_is_in_isr()){
			callee = &th->callee_saved;
			frame.sp = __get_PSP();
			esf = (z_arch_esf_t *)frame.sp;
			frame.lr = esf->basic.lr;
			frame.pc = esf->basic.pc;
			frame.sp += 32;
			frame.fp = callee->v8;
		}else{
			//printk("running, not backtrace\n");
			unwind_backtrace_cur();
			return;
		}
	}else{
		callee = &th->callee_saved;
		esf = (z_arch_esf_t *)callee->psp;

		frame.fp = callee->v8;
		frame.sp = callee->psp + 32; /*r0-r3, r12/lr/pc/xpsr*/
		frame.lr = esf->basic.lr;
		frame.pc = esf->basic.pc;
	}
	sp = frame.sp;
	unwind_backtrace_print(&frame);
	unwind_dump_mem("sp=", sp+32, 256); 

}



static void thread_backtrace(const struct k_thread *th, void *user_data)
{
	unwind_backtrace((struct k_thread *)th);
}

void dump_stack(void)
{
	unwind_backtrace_cur_without_sp();
}
void show_thread_stack(struct k_thread *thread)
{
	unwind_backtrace(thread);
}

void show_all_threads_stack(void)
{
	k_thread_foreach(thread_backtrace, NULL);
}


static void unwind_backtrace_irq(const z_arch_esf_t *esf)
{
	int i;
	uint32_t sp;
	struct stackframe frame;
	sp  = __get_MSP();
	for(i = 0; i < 128; i++) {// find irq sp context
		if(memcmp((void*)sp, esf, 32) == 0)
			break;
		sp += 4;
	}
	if(i == 128){
		printk("not find irq sp\n");
		return ;
	}
	sp += 32;
	frame.sp = sp;
	frame.lr = esf->basic.lr;
	frame.pc = esf->basic.pc;
	frame.fp =  esf->basic.pc;
	printk("show irq stack:\n");
	g_cur_thread_top = frame.sp + 1024;
	unwind_backtrace_print(&frame);
	unwind_dump_mem("sp=", sp+32, 256);
}


/*exceptions printk*/
void k_sys_fatal_error_handler(unsigned int reason,
				      const z_arch_esf_t *esf)
{
	volatile int flag = true;
	if ((esf != NULL) && arch_is_in_nested_exception(esf)) {
		unwind_backtrace_irq(esf);
	}

	printk("stop system\n");
	k_thread_foreach(thread_backtrace, NULL);
	(void)arch_irq_lock();
	jtag_set();
	while(flag);
}

#ifdef CONFIG_ASSERT
void assert_post_action(const char *file, unsigned int line)
{
	dump_stack();
	k_panic();
}
#endif

