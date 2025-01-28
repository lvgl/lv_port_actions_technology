#include "act_log_inner.h"

/* Memory pool for log use */
#ifdef CONFIG_ACTLOG_USE_NANOLOG
K_MEM_SLAB_DEFINE(log_slab, sizeof(nano_log_message_t),
        CONFIG_ACTLOG_NANO_LOG_NUM, 4);
#else
K_MEM_SLAB_DEFINE(log_slab, sizeof(hexdump_log_message_t),
        CONFIG_ACTLOG_NORMAL_LOG_NUM, 4);
#endif

void *log_memory_alloc(uint32_t size)
{
    void *ptr = NULL;
    if (k_mem_slab_alloc(&log_slab, (void **) &ptr, K_NO_WAIT) == 0) {
        (void) memset(ptr, 0, size);
    }

    return ptr;
}

void log_memory_free(void *ptr)
{
    k_mem_slab_free(&log_slab, (void *) &ptr);
}

