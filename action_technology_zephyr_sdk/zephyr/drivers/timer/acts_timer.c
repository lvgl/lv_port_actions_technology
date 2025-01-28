/*
 * Copyright (c) 2017 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <drivers/timer/system_timer.h>
#include <sys_clock.h>
#include <spinlock.h>
#include <soc.h>

#define USE_T2_FOR_CYCLE

#define CYC_PER_TICK (sys_clock_hw_cycles_per_sec()	\
		      / CONFIG_SYS_CLOCK_TICKS_PER_SEC)
#define MAX_TICKS ((0x40000000 / CYC_PER_TICK) - 1)
#define MAX_CYCLES (MAX_TICKS * CYC_PER_TICK)

static struct k_spinlock lock;
/*
 * This local variable holds the amount of SysTick HW cycles elapsed
 * and it is updated in z_clock_isr() and sys_clock_set_timeout().
 *
 * Note:
 *  At an arbitrary point in time the "current" value of the SysTick
 *  HW timer is calculated as:
 *
 * t = cycle_counter + elapsed();
 */
static uint32_t cycle_count;
/*
 * This local variable holds the amount of elapsed SysTick HW cycles
 * that have been announced to the kernel.
 */
static uint32_t announced_cycles;

/**/
static uint32_t last_cycle_count;



static uint32_t acts_timer_elapsed(void)
{
	return	sys_read32(T0_CNT)-last_cycle_count;
}

#ifdef USE_T2_FOR_CYCLE
static void timer_cyc_init(void)
{
#ifdef CONFIG_SOC_SERIES_LEOPARD
	acts_clock_peripheral_enable(CLOCK_ID_TIMER2);
	acts_reset_peripheral(CLOCK_ID_TIMER2);
#endif
	sys_write32(0x0, CMU_TIMER2CLK); //select hosc
	sys_write32(0x1, T2_CTL); //clear pending stop timer
	timer_reg_wait();
	sys_write32(TIMER_MAX_CYCLES_VALUE, T2_VAL);
	sys_write32(0x824, T2_CTL); /* enable counter up,  reload,  notirq */
}
#endif

static void acts_timer_set_next_irq(uint32_t cycle)
{
	if(cycle < CYC_PER_TICK/100)  //min: 10us
		cycle = CYC_PER_TICK/100;
	if(cycle > MAX_CYCLES)
		cycle = MAX_CYCLES;
	sys_write32(sys_read32(T0_CNT)+cycle, T0_VAL);
}

/* Callout out of platform assembly, not hooked via IRQ_CONNECT... */
void acts_timer_isr(void *arg)
{
	ARG_UNUSED(arg);
	uint32_t dticks, cycle;
	/* Increment the amount of HW cycles elapsed (complete counter
	 * cycles) and announce the progress to the kernel.
	 */
	sys_write32(sys_read32(T0_CTL), T0_CTL); // clear penddig;
	cycle = sys_read32(T0_CNT);
	cycle_count += cycle-last_cycle_count;
	last_cycle_count = cycle;

	#if defined(CONFIG_TICKLESS_KERNEL)
		dticks = (cycle_count - announced_cycles) / CYC_PER_TICK;
		announced_cycles += dticks * CYC_PER_TICK;
		//acts_timer_set_next_irq(sys_clock_hw_cycles_per_sec()*10); //Guarantee 10 seconds of interruption
		sys_clock_announce(dticks);
	#else
		acts_timer_set_next_irq(CYC_PER_TICK);
		sys_clock_announce(1);
	#endif
	//printk("-ie=0x%x, 0x%x, 0x%x\n", cycle_count, sys_read32(T0_VAL), sys_read32(T0_CNT));
}


int sys_clock_driver_init(const struct device *device)
{
	/* init timer0 as clock event device */
	sys_write32(0x0, CMU_TIMER0CLK); //select hosc
#ifdef CONFIG_SOC_SERIES_LEOPARD
	acts_clock_peripheral_enable(CLOCK_ID_TIMER0);
#else
	acts_clock_peripheral_enable(CLOCK_ID_TIMER);
#endif
	sys_write32(0x1, T0_CTL); //clear pending stop timer
	timer_reg_wait();
	sys_write32(0xe22, T0_CTL); /* enable counter up, continue mode, irq */

	#ifdef USE_T2_FOR_CYCLE
	timer_cyc_init();
	#endif
	acts_timer_set_next_irq(CYC_PER_TICK);
	//printk("sys_clock_driver_init, cnt=0x%x, val=0x%x\n", sys_read32(T0_CNT), sys_read32(T0_VAL));
	IRQ_CONNECT(IRQ_ID_TIMER0, 0, acts_timer_isr, 0, 0);
	irq_enable(IRQ_ID_TIMER0);

	return 0;
}

#define RC32K_MUTIPLE soc_rc32K_mutiple_hosc()

void sys_clock_set_timeout(int32_t ticks, bool idle)
{

#if defined(CONFIG_TICKLESS_KERNEL)
	k_spinlock_key_t key = k_spin_lock(&lock);
	uint32_t delay;
	ticks = (ticks == K_TICKS_FOREVER) ? MAX_TICKS : ticks;
	if(!ticks)
		ticks = 1;
	uint32_t unannounced = cycle_count - announced_cycles +  acts_timer_elapsed();
	/* Desired delay in the future */
	delay = ticks * CYC_PER_TICK;
	if(unannounced > delay){
		acts_timer_set_next_irq(20); // now is irq
	}else{
		delay = delay - unannounced + 1;
		acts_timer_set_next_irq(delay);
	}
	k_spin_unlock(&lock, key);
#endif

}

#if defined(CONFIG_PM_DEVICE)
#include <wait_q.h>
static uint32_t sleep_st_cycle;
void acts_exit_sleep(void);

int sys_clock_device_ctrl(const struct device *device, enum pm_device_action action)
{
	uint32_t cycle;
	#ifdef CONFIG_TIME_SLEEP_COMP
	uint32_t st_cycle;
	#endif

	if(action == PM_DEVICE_ACTION_RESUME){
		cycle = sys_read32(T0_CNT);
		sys_write32(0x0, CMU_TIMER0CLK); //select hosc
		last_cycle_count = cycle; /*The sleep time is not compensated */
		#ifdef CONFIG_TIME_SLEEP_COMP
		cycle -= sleep_st_cycle;
		st_cycle = cycle*RC32K_MUTIPLE+(RC32K_MUTIPLE/3);// compensated RC32K_MUTIPLE/3
		sl_dbg("t0:sleep=%d us, cycle=%d(32k)\n", k_cyc_to_us_ceil32(st_cycle), cycle);
		cycle_count += st_cycle; // /*The sleep time is not compensated */
		acts_exit_sleep();
		#endif
		sys_clock_set_timeout(z_get_next_timeout_expiry(), 0);
	} else if (action == PM_DEVICE_ACTION_SUSPEND){
		acts_clock_peripheral_disable(CLOCK_ID_TIMER0); // for switch clk source
		sys_write32(0x4, CMU_TIMER0CLK); //select rc32k
		sleep_st_cycle = sys_read32(T0_CNT);
		acts_clock_peripheral_enable(CLOCK_ID_TIMER0);
		#ifdef CONFIG_TIME_SLEEP_COMP
		cycle = sleep_st_cycle-last_cycle_count;// before change to rc32k, cal hosc clk
		cycle_count += cycle;
		last_cycle_count = sleep_st_cycle;
		sl_dbg("t0 suspend: tcnt=%d, t0 used cyc=%d\n", sleep_st_cycle, cycle);
		#endif
	}
	return 0;
}

#endif

uint32_t sys_clock_elapsed(void)
{
	#if defined(CONFIG_TICKLESS_KERNEL)
		k_spinlock_key_t key = k_spin_lock(&lock);
		uint32_t cyc = acts_timer_elapsed() + cycle_count - announced_cycles;
		k_spin_unlock(&lock, key);
		return cyc / CYC_PER_TICK;
	#else
		return 0 ;
	#endif
}


uint32_t sys_clock_cycle_get_32(void)
{
#ifdef USE_T2_FOR_CYCLE
	return sys_read32(T2_CNT) + soc_sleep_cycle();
#else
	k_spinlock_key_t key = k_spin_lock(&lock);
	uint32_t ret = acts_timer_elapsed() + cycle_count ;
	k_spin_unlock(&lock, key);
	return ret;
#endif
}


void sys_clock_idle_exit(void)
{


}
void sys_clock_disable(void)
{

}


