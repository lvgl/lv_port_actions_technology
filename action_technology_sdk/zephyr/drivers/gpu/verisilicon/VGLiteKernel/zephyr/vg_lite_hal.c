/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "vg_lite_platform.h"
#include "../vg_lite_debug.h"
#include "../vg_lite_kernel.h"
#include "../vg_lite_hal.h"
#include "../vg_lite_hw.h"
#include "../vg_lite_type.h"

#include <assert.h>
#include <soc.h>
#include <zephyr.h>
#include <device.h>
#include <spicache.h>
#include <board_cfg.h>
#include <sys/sys_heap.h>
#include <tracing/tracing.h>
#if defined(CONFIG_CPU_CORTEX_M)
#  include <arch/arm/aarch32/cortex_m/cmsis.h>
#endif

#include <logging/log.h>
LOG_MODULE_REGISTER(VGLite, LOG_LEVEL_INF);

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* For GPU only, 64 bytes is enough except that Verisilicon DC avaiable */
#undef VGLITE_MEM_ALIGNMENT
#define VGLITE_MEM_ALIGNMENT 64

#define VG_LITE_K_MEM_POOL_SIZE  \
    (CONFIG_VG_LITE_K_MEM_POOL_SIZE + VGLITE_MEM_ALIGNMENT - 1) & ~(VGLITE_MEM_ALIGNMENT - 1)

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct vg_lite_dev_data {
    struct sys_heap heap;
    struct k_spinlock heap_lock;
    struct k_sem wait_sem;

    volatile uint32_t int_flags;

    vg_lite_gpu_execute_state_t gpu_execute_state;
    int8_t gpu_clken_cnt;
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

/*
 * 2 command buffer and 1 tess buffer:
 * 1) command buffer default size VG_LITE_COMMAND_BUFFER_SIZE
 * 2) tess_size = VG_LITE_ALIGN(tess_height, 16) * 128
 */
__aligned(VGLITE_MEM_ALIGNMENT) __in_section_unique(vglite.noinit.mem_pool)
uint8_t vg_lite_heap_mem[VG_LITE_K_MEM_POOL_SIZE];

static struct vg_lite_dev_data * gp_dev_data = NULL;

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void vg_lite_hal_delay(uint32_t milliseconds)
{
    /* cannot sleep during suspend/resume */
    k_busy_wait(milliseconds);
}

void vg_lite_hal_barrier(void)
{
    /* TODO: Memory barrier. */
    /* flush the write buffer for uncache and write through memory */
    spi1_cache_ops(SPI_WRITEBUF_FLUSH, (void *)SPI1_UNCACHE_ADDR, 0);

    sys_trace_void(SYS_TRACE_ID_VGLITE_DRAW);
}

void vg_lite_hal_initialize(void)
{
    /* Hold reset. */
    acts_reset_peripheral_assert(RESET_ID_GPU);

    /* Turn on the clock. */
    vg_lite_set_gpu_clock_state(1);

    /* Turn on the power (clock is not required). */
    soc_powergate_set(POWERGATE_GPU_PG_DEV, true);

    /* Release reset. */
    acts_reset_peripheral_deassert(RESET_ID_GPU);

    /*
     * Harware issue:
     * After powergate on, the interrupt line is undetermined,
     * so clear the pending here.
     */
#if defined(CONFIG_CPU_CORTEX_M)
    NVIC_ClearPendingIRQ((IRQn_Type)IRQ_ID_GPU);
#endif

    /* Enable interrupt. */
    irq_enable(IRQ_ID_GPU);

    vg_lite_hal_trace("GPU power on\n");
}

void vg_lite_hal_deinitialize(void)
{
    /* Disable interrupt. */
    irq_disable(IRQ_ID_GPU);

    /* Hold reset. */
    acts_reset_peripheral_assert(RESET_ID_GPU);

    /* Remove power. */
    soc_powergate_set(POWERGATE_GPU_PG_DEV, false);

    /* Remove clock. */
    vg_lite_set_gpu_clock_state(0);

    vg_lite_hal_trace("GPU power off\n");
}

void vg_lite_hal_print(char *format, ...)
{
    va_list args;
    va_start(args, format);
    vprintk(format, args);
    va_end(args);
}

void vg_lite_hal_trace(char *format, ...)
{
#if gcdVG_ENABLE_DEBUG
    va_list args;
    va_start(args, format);
    vprintk(format, args);
    va_end(args);
#endif
}

const char *vg_lite_hal_Status2Name(vg_lite_error_t status)
{
    switch (status) {
    case VG_LITE_SUCCESS:
        return "VG_LITE_SUCCESS";
    case VG_LITE_INVALID_ARGUMENT:
        return "VG_LITE_INVALID_ARGUMENT";
    case VG_LITE_OUT_OF_MEMORY:
        return "VG_LITE_OUT_OF_MEMORY";
    case VG_LITE_NO_CONTEXT:
        return "VG_LITE_NO_CONTEXT";
    case VG_LITE_TIMEOUT:
        return "VG_LITE_TIMEOUT";
    case VG_LITE_OUT_OF_RESOURCES:
        return "VG_LITE_OUT_OF_RESOURCES";
    case VG_LITE_GENERIC_IO:
        return "VG_LITE_GENERIC_IO";
    case VG_LITE_NOT_SUPPORT:
        return "VG_LITE_NOT_SUPPORT";
    case VG_LITE_ALREADY_EXISTS:
        return "VG_LITE_ALREADY_EXISTS";
    case VG_LITE_NOT_ALIGNED:
        return "VG_LITE_NOT_ALIGNED";
    case VG_LITE_FLEXA_TIME_OUT:
        return "VG_LITE_FLEXA_TIME_OUT";
    case VG_LITE_FLEXA_HANDSHAKE_FAIL:
        return "VG_LITE_FLEXA_HANDSHAKE_FAIL";
    case VG_LITE_SYSTEM_CALL_FAIL:
        return "VG_LITE_SYSTEM_CALL_FAIL";
    default:
        return "nil";
    }
}

vg_lite_error_t vg_lite_hal_allocate(uint32_t size, void **memory)
{
    struct vg_lite_dev_data *data = gp_dev_data;
    k_spinlock_key_t key;

    assert(data != NULL);

    if (size == 0 || NULL == memory) {
        return VG_LITE_INVALID_ARGUMENT;
    }

    key = k_spin_lock(&data->heap_lock);
    *memory = sys_heap_alloc(&data->heap, size);
    k_spin_unlock(&data->heap_lock, key);

    if (NULL == *memory) {
        return VG_LITE_OUT_OF_MEMORY;
    }

    return VG_LITE_SUCCESS;
}

vg_lite_error_t vg_lite_hal_free(void *memory)
{
    struct vg_lite_dev_data *data = gp_dev_data;
    vg_lite_error_t error = VG_LITE_SUCCESS;
    k_spinlock_key_t key;

    assert(data != NULL);

    if (memory) {
        key = k_spin_lock(&data->heap_lock);
        sys_heap_free(&data->heap, memory);
        k_spin_unlock(&data->heap_lock, key);
    }

    return error;
}

vg_lite_error_t vg_lite_hal_allocate_contiguous(unsigned long size,
        vg_lite_vidmem_pool_t pool, void **logical, void **klogical,
        uint32_t *physical, void **node)
{
    struct vg_lite_dev_data *data = gp_dev_data;
    unsigned long aligned_size;
    k_spinlock_key_t key;

    assert(data != NULL);

    /* Align the size to 64 bytes. */
    aligned_size = VG_LITE_ALIGN(size, VGLITE_MEM_ALIGNMENT);

    key = k_spin_lock(&data->heap_lock);
    *node = sys_heap_aligned_alloc(&data->heap, VGLITE_MEM_ALIGNMENT, aligned_size);
    k_spin_unlock(&data->heap_lock, key);

    if (*node == NULL) {
        vg_lite_hal_print("VGLite alloc %lu bytes failed\n", size);
        return VG_LITE_OUT_OF_MEMORY;
    }

    *logical = *node;
    *klogical = *node;
    *physical = (uint32_t)cache_to_uncache(*node);

    return VG_LITE_SUCCESS;
}

void vg_lite_hal_free_contiguous(void * memory_handle)
{
    struct vg_lite_dev_data *data = gp_dev_data;
    k_spinlock_key_t key;

    assert(data != NULL);

    key = k_spin_lock(&data->heap_lock);
    sys_heap_free(&data->heap, memory_handle);
    k_spin_unlock(&data->heap_lock, key);
}

void vg_lite_hal_free_os_heap(void)
{
    /* TODO: Remove unfree node. */
}

VG_LITE_ATTRIBUTE_FAST_FUNC
uint32_t vg_lite_hal_peek(uint32_t address)
{
    /* Read data from the GPU register. */
    return sys_read32(GPU_REG_BASE + address);
}

VG_LITE_ATTRIBUTE_FAST_FUNC
void vg_lite_hal_poke(uint32_t address, uint32_t data)
{
    /* Write data to the GPU register. */
    sys_write32(data, GPU_REG_BASE + address);
}

vg_lite_error_t vg_lite_hal_query_mem(vg_lite_kernel_mem_t *mem)
{
    mem->bytes = 0;
    return VG_LITE_NOT_SUPPORT;
}

vg_lite_error_t vg_lite_hal_map_memory(vg_lite_kernel_map_memory_t *node)
{
    node->physical = (uint32_t)cache_to_uncache((void *)node->physical);
    node->logical = uncache_to_wt_wna_cache((void *)node->physical);

    return VG_LITE_SUCCESS;
}

vg_lite_error_t vg_lite_hal_unmap_memory(vg_lite_kernel_unmap_memory_t *node)
{
    return VG_LITE_SUCCESS;
}

VG_LITE_ATTRIBUTE_FAST_FUNC
void * vg_lite_hal_map(uint32_t flags, uint32_t bytes, void *logical,
        uint32_t physical, int32_t dma_buf_fd, uint32_t *gpu)
{
    if (flags != VG_LITE_HAL_MAP_USER_MEMORY) {
        return NULL;
    }

    if (logical == NULL) {
        logical = uncache_to_wt_wna_cache((void *)physical);
    }

    if (buf_is_nor(logical) || buf_is_nor_un(logical)) {
        return NULL;
    }

    *gpu = (uint32_t)cache_to_uncache(logical);

    return logical;
}

VG_LITE_ATTRIBUTE_FAST_FUNC
void vg_lite_hal_unmap(void * handle)
{
    (void) handle;
}

vg_lite_error_t vg_lite_hal_operation_cache(void *handle, vg_lite_cache_op_t cache_op)
{
    switch (cache_op) {
    case VG_LITE_CACHE_CLEAN:
        if (buf_is_psram_cache(handle)) {
            spi1_cache_ops(SPI_CACHE_FLUSH_ALL, handle, 0);
        }
        break;
    case VG_LITE_CACHE_FLUSH:
    case VG_LITE_CACHE_INVALIDATE:
        if (buf_is_psram(handle)) {
            spi1_cache_ops(SPI_CACHE_FLUSH_INVALID_ALL, handle, 0);
        }
        break;
    default:
        break;
    }

    return VG_LITE_SUCCESS;
}

vg_lite_error_t vg_lite_hal_memory_export(int32_t *fd)
{
    return VG_LITE_SUCCESS;
}

void vg_lite_set_gpu_execute_state(vg_lite_gpu_execute_state_t state)
{
    struct vg_lite_dev_data *data = gp_dev_data;
    assert(data != NULL);

    if (state != data->gpu_execute_state) {
        data->gpu_execute_state = state;
        vg_lite_set_gpu_clock_state(state == VG_LITE_GPU_RUN);
    }
}

VG_LITE_ATTRIBUTE_FAST_FUNC
void vg_lite_set_gpu_clock_state(int enabled)
{
    struct vg_lite_dev_data *data = gp_dev_data;
    assert(data != NULL);

    if (enabled) {
        assert(data->gpu_clken_cnt < INT8_MAX);

        if (++data->gpu_clken_cnt == 1) {
            acts_clock_peripheral_enable(CLOCK_ID_GPU);
            /**
             * FIXME: should add any delay to wait clock stable ?
             *
             * k_busy_wait(10);
             */
        }
    } else {
        assert(data->gpu_clken_cnt > 0);

        if (--data->gpu_clken_cnt == 0) {
            acts_clock_peripheral_disable(CLOCK_ID_GPU);
        }
    }
}

int32_t vg_lite_hal_wait_interrupt(uint32_t timeout, uint32_t mask, uint32_t * value)
{
    struct vg_lite_dev_data *data = gp_dev_data;
    assert(data != NULL);

    int result = k_sem_take(&data->wait_sem,
            (timeout == VG_LITE_INFINITE) ? K_FOREVER : K_MSEC(timeout));

    /* Report the event(s) got. */
    if (value != NULL) {
        *value = data->int_flags & mask;
    }

    data->int_flags = 0;

    /*
     * FIXME: Flush GPU write buffer here ?
     *
     * Synchronization will finally go to this routine.
     */
    sys_set_bit(SPI1_GPU_CTL, 24);

    return (result == 0) ? 1 : 0;
}

/****************************************************************************
 * Private Functions
 ****************************************************************************/

VG_LITE_ATTRIBUTE_FAST_FUNC
static void vg_lite_dev_isr(const void *arg)
{
    const struct device *dev = arg;
    struct vg_lite_dev_data *data = dev->data;

    /* Read interrupt status. */
    uint32_t flags = sys_read32(GPU_REG_BASE + VG_LITE_INTR_STATUS);

    if (flags) {
        sys_trace_end_call(SYS_TRACE_ID_VGLITE_DRAW);

#if gcdVG_RECORD_HARDWARE_RUNNING_TIME
        record_running_time();
#endif

        /* Combine with current interrupt flags. */
        data->int_flags |= flags;

        /* Wake up any waiters. */
        k_sem_give(&data->wait_sem);
    }

#if 0
    if (flags = VGLITE_EVENT_FRAME_END) {
        /* A callback function can be added here to inform that gpu is idle.*/
        (*callback)();
    }
#endif
}

DEVICE_DECLARE(gpu);

static int vg_lite_dev_init(const struct device *dev)
{
    struct vg_lite_dev_data *data = dev->data;

    data->gpu_execute_state = VG_LITE_GPU_STOP;

    sys_heap_init(&data->heap, cache_to_wt_wna_cache(vg_lite_heap_mem), sizeof(vg_lite_heap_mem));
    k_sem_init(&data->wait_sem, 0, 1);

    IRQ_CONNECT(IRQ_ID_GPU, 0, vg_lite_dev_isr, DEVICE_GET(gpu), 0);

    clk_set_rate(CLOCK_ID_GPU, KHZ(CONFIG_GPU_CLOCK_KHZ));

    gp_dev_data = data;
    return 0;
}

#ifdef CONFIG_PM_DEVICE
static int vg_lite_dev_suspend(const struct device *dev)
{
    struct vg_lite_dev_data *data = dev->data;

    if (data->gpu_execute_state == VG_LITE_GPU_RUN) {
        vg_lite_hal_trace("GPU suspend fail!\n");
        return -EBUSY;
    }

#if gcdVG_ENABLE_DELAY_RESUME
    if (vg_lite_kernel(VG_LITE_QUERY_DELAY_RESUME, NULL)) {
        return 0;
    } else {
        vg_lite_kernel(VG_LITE_SET_DELAY_RESUME, NULL);
    }
#endif

    vg_lite_set_gpu_clock_state(1);

    /* shutdown gpu */
    vg_lite_kernel(VG_LITE_CLOSE, NULL);

    /* shutdown power and clock  */
    vg_lite_hal_deinitialize();

    return 0;
}

static int vg_lite_dev_resume(const struct device *dev)
{
#if !gcdVG_ENABLE_DELAY_RESUME
    vg_lite_hal_initialize();

    /* open gpu interrupt and recovery gpu register */
    vg_lite_kernel(VG_LITE_RESET, NULL);

    vg_lite_set_gpu_clock_state(0);
#endif

    return 0;
}

static int vg_lite_dev_pm_control(const struct device *dev, enum pm_device_action action)
{
    int ret = 0;

    switch (action) {
    case PM_DEVICE_ACTION_SUSPEND:
    case PM_DEVICE_ACTION_FORCE_SUSPEND:
        ret = vg_lite_dev_suspend(dev);
        break;
    case PM_DEVICE_ACTION_RESUME:
        ret = vg_lite_dev_resume(dev);
        break;
    default:
        break;
    }

    return ret;
}
#endif /* CONFIG_PM_DEVICE */

#if CONFIG_GPU_DEV
static struct vg_lite_dev_data vg_lite_dev_data;

DEVICE_DEFINE(gpu, CONFIG_GPU_DEV_NAME, vg_lite_dev_init,
        vg_lite_dev_pm_control, &vg_lite_dev_data, NULL, POST_KERNEL,
        CONFIG_KERNEL_INIT_PRIORITY_DEVICE, NULL);
#endif /* CONFIG_GPU_DEV */
