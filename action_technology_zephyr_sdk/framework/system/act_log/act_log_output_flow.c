#include "act_log_inner.h"

int act_log_output_flow_init(void)
{
    //Todo
    return 0;
}

int act_log_output_flow_write(log_message_t *log_msg, uint8_t *data, uint32_t len)
{
#ifdef CONFIG_ACTIONS_PRINTK_DMA
    return uart_dma_send_buf(data, len);
#elif defined(CONFIG_PRINTK)
    k_str_out((char *)data, len);
    return len;
#else
    return 0;
#endif
}

