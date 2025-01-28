/*******************************************************************************
 * @file    sensor_algo_common.c
 * @author  MEMS Application Team
 * @version V1.0
 * @date    2023-12-29
 * @brief   sensor algorithm common api
*******************************************************************************/

/******************************************************************************/
//includes
/******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <soc.h>
#include <kernel.h>
#include <sys/sys_heap.h>

/******************************************************************************/
//constants
/******************************************************************************/
#define CONFIG_SENSOR_HEAP_SIZE		(0x6800)

/******************************************************************************/
//variables
/******************************************************************************/

/* sensor heap */
static struct sys_heap sensor_heap;
static uint8_t sensor_heap_mem[CONFIG_SENSOR_HEAP_SIZE] __aligned(4);

/******************************************************************************/
//functions
/******************************************************************************/

uint64_t sensor_get_timestamp_ms(void)
{
	return soc_sys_uptime_get();
}

uint64_t sensor_get_timestamp_us(void)
{
	return soc_sys_uptime_get() * 1000ull;
}

uint64_t sensor_get_timestamp_ns(void)
{
	return soc_sys_uptime_get() * 1000000ull;
}

void sensor_udelay(uint32_t us)
{
	soc_udelay(us);
}

void sensor_mdelay(uint32_t ms)
{
	if (soc_in_sleep_mode()) {
		soc_udelay(ms * 1000);
	} else {
		k_msleep(ms);
	}
}

void *sensor_malloc(size_t size)
{
	return sys_heap_alloc(&sensor_heap, size);
}

void sensor_free(void *ptr)
{
	sys_heap_free(&sensor_heap, ptr);
}

static int sensor_heap_init(const struct device *arg)
{
	sys_heap_init(&sensor_heap, sensor_heap_mem, CONFIG_SENSOR_HEAP_SIZE);

	return 0;
}

SYS_INIT(sensor_heap_init, APPLICATION, 90);

