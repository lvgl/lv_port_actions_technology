/*
 * Copyright (c) 2019 Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief trace file APIs.
*/

#ifndef TRACE_H_
#define TRACE_H_

typedef enum
{
    TRACE_MODE_DISABLE = 0,
    TRACE_MODE_CPU,
    TRACE_MODE_DMA,
}trace_mode_e;

int trace_mode_set(unsigned int trace_mode);

int trace_irq_print_set(unsigned int irq_enable);

int trace_dma_print_set(unsigned int dma_enable);

#ifdef CONFIG_TRACE_EVENT

/* task trace function */
extern void _trace_task_switch(struct k_thread  *from, struct k_thread  *to);

/* malloc trace function */
extern void _trace_malloc(uint32_t address, uint32_t malloc_size, uint32_t caller);

extern void _trace_free(uint32_t address, uint32_t caller);

/* task trace */
#define TRACE_TASK_SWITCH(from, to)                    _trace_task_switch(from, to)


/* malloc trace */
#define TRACE_MALLOC(address, size, caller)            _trace_malloc(address, size, caller)

#define TRACE_FREE(address, caller)                    _trace_free(address, caller) 

#else

/* task trace */
#define TRACE_TASK_SWITCH(from, to)

/* malloc trace */
#define TRACE_MALLOC(address, size, caller) 

#define TRACE_FREE(address, caller)  

#endif


#endif /* TRACE_H_ */
