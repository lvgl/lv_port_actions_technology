/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sys/printk.h>
#include <kernel_internal.h>
#include <ksched.h>

#ifdef CONFIG_TRACING_IRQ_PROFILER

#define RUNNING_CYCLES(end, start)	((uint32_t)((long)(end) - (long)(start)))

#ifdef CONFIG_TRACING_IRQ_PROFILER_IRQLOADING

#define RUNNING_TIMES(end, start)	((uint32_t)((long)(end) - (long)(start)))

extern void idle(void *unused1, void *unused2, void *unused3);

static u32_t cpuload_isr_total_cycles;
static u32_t cpuload_isr_in_idle_cycles;

static u32_t irq_pre_total_us[IRQ_TABLE_SIZE];

#endif

void isr_wrapper_with_profiler(u32_t irqnum)
{
	u32_t ts_irq_enter, irq_cycles;
	u32_t stop_cycle;
#ifdef CONFIG_TRACING_IRQ_PROFILER_IRQLOADING
	static u32_t pre_time, curr_time;
	u32_t i, print_flag = 0;
#endif
    struct _isr_table_entry *ite;

	irqnum >>= 3;

	ts_irq_enter = k_cycle_get_32();

	ite = &_sw_isr_table[irqnum];

	if(ite->isr){
	    ite->isr(ite->arg);
	}

	ite->irq_cnt++;
	stop_cycle = k_cycle_get_32();
	irq_cycles = RUNNING_CYCLES(stop_cycle, ts_irq_enter);
	ite->irq_total_us += k_cyc_to_ns_floor64(irq_cycles) / 1000;
	if (irq_cycles > ite->max_irq_cycles)
		ite->max_irq_cycles = irq_cycles;

#ifdef CONFIG_TRACING_IRQ_PROFILER_MAX_LATENCY
	if(irq_cycles >= CONFIG_TRACING_IRQ_PROFILE_MAX_LATENCY_CYCLES){
		printk("irq@%d run %dus not meet latency\n", irqnum, (u32_t)(k_cyc_to_ns_floor64(irq_cycles) / 1000));
	}
#endif

#ifdef CONFIG_TRACING_IRQ_PROFILER_IRQLOADING
	cpuload_isr_total_cycles += RUNNING_CYCLES(stop_cycle, ts_irq_enter);
	if (_current->entry.pEntry == idle) {
		cpuload_isr_in_idle_cycles += RUNNING_CYCLES(stop_cycle, ts_irq_enter);
	}

	curr_time = k_uptime_get_32();
	if ((curr_time - pre_time) > 1000) {
		pre_time = curr_time;

		for (i=0; i<IRQ_TABLE_SIZE; i++) {
			ite = &_sw_isr_table[i];

			if (ite->isr != z_irq_spurious) {
				if ((RUNNING_TIMES(ite->irq_total_us, irq_pre_total_us[i]) > 60000)) {
					printk("current irq@%d overtime run %dus\n", i, RUNNING_TIMES(ite->irq_total_us, irq_pre_total_us[i]));
					print_flag = 1;
				}

				irq_pre_total_us[i] = ite->irq_total_us;
			}
		}

		/* > (300ms)  (9677000/1000)*31 */
		/* SYS_CLOCK_HW_CYCLES_TO_NS_AVG(1000, 1000) = 31us */
		if ((cpuload_isr_total_cycles > 9677000) || print_flag) {
			printk("(%u)total isr %d us, in idle %d us\n", curr_time,
					SYS_CLOCK_HW_CYCLES_TO_NS_AVG(cpuload_isr_total_cycles, 1000),
					SYS_CLOCK_HW_CYCLES_TO_NS_AVG(cpuload_isr_in_idle_cycles, 1000));
		}
		cpuload_isr_total_cycles = 0;
		cpuload_isr_in_idle_cycles = 0;
	}
#endif

}
#endif /* CONFIG_TRACING_CPU_STATS_LOG */
