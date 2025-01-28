#include <init.h>
#include <kernel.h>
#include <drivers/uart.h>
#include <drivers/uart_dma.h>
#include <sys/ring_buffer.h>
#include "aic_cfg.h"
#include "aic_type.h"
#include <board_cfg.h>

//#define CONFIG_UART_IRQ_TTRIGER
//#define AIC_UART_DEBUG
#ifdef AIC_UART_DEBUG
#define dbg_print       printk
#else
#define dbg_print(...)
#endif
#define DMA_RX_CHECK_TIMER_MS       2
#define DMA_RX_START_CHECK_MS       4
#define DMA_UART_RX_BUFFER_LEN      0x1800
#define UART_RX_RING_BUFFER_LEN     0x2000

#define TIMER_FOREVER (-1)

static char __attribute__ ((aligned(32))) s_rx_dma_buf[DMA_UART_RX_BUFFER_LEN];
static char __attribute__ ((aligned(32))) s_rx_ring_buf[UART_RX_RING_BUFFER_LEN];

typedef struct uart_dma_handle{
    struct device *dev;
    struct k_mutex mutex;
    struct k_sem dma_tx_completion;
    struct k_sem rx_fifo_data;
    uint8_t *rx_dma_buf; //rx DMA buffer addr
    struct ring_buf rx_ringbuf;
    uint8_t *rx_ring_buf; //rx cache buf
    uint32_t recv_remain_prev;         //bytes remain in fifo last time
    struct k_delayed_work rx_dma_check_work;
}uart_dma_handle_t;

static size_t uart_read_timedwait(void *handle, void *buffer, size_t size,
                                    uint32_t ms)
{
    uart_dma_handle_t *h = (uart_dma_handle_t *)handle;
    size_t recv_size;
    int ret;

    do {
        recv_size = ring_buf_get(&h->rx_ringbuf, buffer, size);
        if (recv_size <= 0) {
            ret = k_sem_take(&h->rx_fifo_data, K_MSEC(ms));
            if (ret) {
                printk("aic uart read: err(%d)\n", ret);
                return 0;
            }
        }
    } while (recv_size <= 0);

    dbg_print("aic_uart_read: size(%d), need readsize(%d)\n", recv_size, size);

    return recv_size;
}

size_t aic_uart_read(void *handle, void *buffer, size_t size)
{
    return uart_read_timedwait(handle, buffer, size, TIMER_FOREVER);
}

size_t aic_uart_write(void *handle, const void *buffer, size_t size)
{
    int overt_ms, ret;
    size_t send_size = size;
    uart_dma_handle_t *h = (uart_dma_handle_t *)handle;
    struct uart_config cfg;

    uart_config_get(h->dev, &cfg);

    overt_ms = 10*10000*size /cfg.baudrate + 2;

    k_mutex_lock(&h->mutex, K_FOREVER);

    /* reset sem to tx dma irq callback*/
    k_sem_reset(&h->dma_tx_completion);

    uart_dma_send(h->dev, (char *)buffer, size);

    ret = k_sem_take(&h->dma_tx_completion, Z_TIMEOUT_MS(overt_ms));
    if (ret) {
        /* wait timeout */
        printk("aic_uart_write err(%d)\n", ret);
        send_size = 0;
    }

    uart_dma_send_stop(h->dev);

    k_mutex_unlock(&h->mutex);

    dbg_print("aic_uart_write: tx_size(%d),size(%d)\n", send_size, size);

    return send_size;
}

#ifdef CONFIG_UART_IRQ_TTRIGER
static void aic_uart_irq_handler(const struct device *dev, void *user_data)
{
    uart_dma_handle_t *handle = (uart_dma_handle_t *)user_data;
    if(!uart_irq_rx_ready(handle->dev))// if rx fifo empty, return
        return;
    dbg_print("aic uart rx irq\n");
    uart_irq_rx_disable(handle->dev); // disable rx irq
    uart_rx_dma_switch(handle->dev, 1, NULL, NULL); // rx fifo accessed by dma
    uart_dma_receive(handle->dev, handle->rx_dma_buf, DMA_UART_RX_BUFFER_LEN);// start rx dma
    handle->recv_remain_prev = DMA_UART_RX_BUFFER_LEN;
    k_delayed_work_submit(&handle->rx_dma_check_work, K_MSEC(DMA_RX_CHECK_TIMER_MS));
}
#endif

/*dma rx complete handler*/
static void aic_uart_dma_rx_handler(const struct device *dev, void *priv_data, uint32_t channel, int status)
{
    uint32_t size;
    uart_dma_handle_t *handle =(uart_dma_handle_t *)priv_data;
    uart_dma_receive_stop(handle->dev);
    size = ring_buf_put(&handle->rx_ringbuf, handle->rx_dma_buf, DMA_UART_RX_BUFFER_LEN); //push to ring buf
    k_sem_give(&handle->rx_fifo_data);
    if(size != DMA_UART_RX_BUFFER_LEN){
        printk("err: rx buf full lost=0x%x \n", DMA_UART_RX_BUFFER_LEN-size);
    }
    uart_dma_receive(handle->dev, handle->rx_dma_buf, DMA_UART_RX_BUFFER_LEN);// start rx dma
    handle->recv_remain_prev = DMA_UART_RX_BUFFER_LEN;
    k_delayed_work_cancel(&handle->rx_dma_check_work);
    k_delayed_work_submit(&handle->rx_dma_check_work, K_MSEC(DMA_RX_CHECK_TIMER_MS));
    dbg_print("...rx dma_rx_handler\n");
}

/*dma send complete handler*/
static void aic_uart_dma_tx_handler(const struct device *dev, void *priv_data, uint32_t channel, int status)
{
    uart_dma_handle_t *handle =(uart_dma_handle_t *)priv_data;
    uart_dma_send_stop(handle->dev);
    k_sem_give(&handle->dma_tx_completion);
    dbg_print("*** tx dma_tx_handler\n");
    return;
}

/* final consumer in loopback test, run every 5ms */
static void aic_uart_dma_rx_check_work(struct k_work *work)
{
    uart_dma_handle_t *handle = CONTAINER_OF(work, uart_dma_handle_t, rx_dma_check_work);;
    uint32_t remain, size, put_size;
    remain = uart_dma_receive_remain(handle->dev);
    if(handle->recv_remain_prev != remain){
        dbg_print("aic_uart rx work: %d!=%d\n", handle->recv_remain_prev, remain);
        handle->recv_remain_prev = remain;
        k_delayed_work_submit(&handle->rx_dma_check_work, K_MSEC(DMA_RX_CHECK_TIMER_MS));
    }else{
#ifdef CONFIG_UART_IRQ_TTRIGER
        uart_dma_receive_stop(handle->dev);
        uart_rx_dma_switch(handle->dev, 0, NULL, NULL);// set uart rx fifo accecced by cpu
        put_size = DMA_UART_RX_BUFFER_LEN-remain;
        size = ring_buf_put(&handle->rx_ringbuf, handle->rx_dma_buf, put_size); //push to ring buf
        if(size != put_size){
            printk("err:w rx buf full lost=0x%x \n", put_size-size);
        }
        k_sem_give(&handle->rx_fifo_data);
        uart_irq_rx_enable(handle->dev);// use irq rx
        dbg_print("aic_uart rx work:len=0x%x rc %d, stop dma\n", put_size, remain);
#else
        if(remain != DMA_UART_RX_BUFFER_LEN){
            uart_dma_receive_stop(handle->dev);
            put_size = DMA_UART_RX_BUFFER_LEN-remain;
            size = ring_buf_put(&handle->rx_ringbuf, handle->rx_dma_buf, put_size); //push to ring buf
            if(size != put_size){
                printk("err:w rx buf full lost=0x%x \n", put_size-size);
            }
            k_sem_give(&handle->rx_fifo_data);
            uart_dma_receive(handle->dev, handle->rx_dma_buf, DMA_UART_RX_BUFFER_LEN);// start rx dma
            handle->recv_remain_prev = DMA_UART_RX_BUFFER_LEN;
            dbg_print("aic_uart rx work:len=0x%x rc %d\n", put_size, remain);
        }
        k_delayed_work_submit(&handle->rx_dma_check_work, K_MSEC(DMA_RX_START_CHECK_MS));
#endif
    }
}

/*
uart 读写是按dma 方式来收发，需要再board_cfg.h  里配置dma
com 对应的串口num，比如com 为1 ，需要对应板型下的board_cfg.h：
#define CONFIG_UART_1           				1
#define CONFIG_UART_1_NAME      			"UART_1"
#define CONFIG_UART_1_USE_TX_DMA   1
#define CONFIG_UART_1_TX_DMA_CHAN  0xff
#define CONFIG_UART_1_TX_DMA_ID    2
#define CONFIG_UART_1_USE_RX_DMA   1
#define CONFIG_UART_1_RX_DMA_CHAN  0xff
#define CONFIG_UART_1_RX_DMA_ID    2

需要在board.c 里配置mfp：
    PIN_MFP_SET(GPIO_16,  UART1_MFP_CFG),
    PIN_MFP_SET(GPIO_17,  UART1_MFP_CFG),

*/
const char * aic_uart_dev_name(int com)
{
    if(com == 0)
        return CONFIG_UART_0_NAME;
    else  if(com == 1)
        return CONFIG_UART_1_NAME;
    else  if(com == 2)
        return CONFIG_UART_2_NAME;
    else  if(com == 3)
        return CONFIG_UART_3_NAME;
    else  if(com == 4)
        return CONFIG_UART_4_NAME;
    else
        return NULL;
}

void *aic_uart_open(int com)
{
    const struct device *uart_dev;
    const char *dev_name;
    aic_uart_cfg_t *uart_cfg;
    struct uart_config cfg;
    uart_dma_handle_t *handle;
    int ret;

    com = 1;
    dev_name = aic_uart_dev_name(com);
    if(dev_name == NULL){
        printk("aic_uart_open() com=%d not exist\n", com);
        return NULL;
    }
    uart_cfg = aic_get_uart_cfg();
    uart_dev = device_get_binding(dev_name);
    if(uart_dev == NULL){
        printk("aic_uart_open() com=%d binding fail\n", com);
        return NULL;
    }
    /* malloc space for uart  */
    handle = (uart_dma_handle_t *)aos_malloc(sizeof(uart_dma_handle_t));
    if(handle == NULL){
        printk("aic_uart_open() com=%d malloc size:%d ail\n", com, sizeof(uart_dma_handle_t));
        return NULL;
    }

    handle->dev = (struct device *)uart_dev;
    handle->rx_ring_buf = s_rx_ring_buf;
    handle->rx_dma_buf = s_rx_dma_buf;
    ring_buf_init(&handle->rx_ringbuf, UART_RX_RING_BUFFER_LEN,  handle->rx_ring_buf);
    k_delayed_work_init(&handle->rx_dma_check_work, aic_uart_dma_rx_check_work);
    k_sem_init(&handle->dma_tx_completion, 0, UINT_MAX);
    k_sem_init(&handle->rx_fifo_data, 0, UINT_MAX);
    k_mutex_init(&handle->mutex);

    uart_config_get(uart_dev, &cfg);
    cfg.baudrate = uart_cfg->baud;
    uart_configure(uart_dev, &cfg);

    /*init tx uart*/
    ret = uart_dma_send_init(handle->dev, aic_uart_dma_tx_handler, handle);// register dma callback function
    if(ret)	{
        printk("uart%d dma send init err\n", com);
        return NULL;
    }

    /*init recv*/
    ret = uart_dma_receive_init(handle->dev, aic_uart_dma_rx_handler, handle);// register drq callback function
    if(ret) {
        printk("uart%d dma recv init err\n", com);
        return NULL;
    }

    /*disable irq*/
    uart_irq_tx_disable(handle->dev);
    uart_irq_rx_disable(handle->dev);

    /*cpu rx*/
    uart_rx_dma_switch(handle->dev, 0, NULL, NULL);// set uart rx fifo accecced by cpu

    /*dma tx*/
    uart_tx_dma_switch(handle->dev, 1, NULL, NULL);// set uart tx fifo accecced by dma

#ifdef CONFIG_UART_IRQ_TTRIGER
    /*set rx irq callback func*/
    uart_irq_callback_set(handle->dev, aic_uart_irq_handler);
    uart_irq_callback_user_data_set(handle->dev, aic_uart_irq_handler, handle);
    uart_irq_rx_enable(handle->dev);
#else
    uart_irq_rx_disable(handle->dev); // disable rx irq
    uart_rx_dma_switch(handle->dev, 1, NULL, NULL); // rx fifo accessed by dma
    uart_dma_receive(handle->dev, handle->rx_dma_buf, DMA_UART_RX_BUFFER_LEN);// start rx dma
    //uart1_stat = sys_read32(UART1_STA);
    handle->recv_remain_prev = DMA_UART_RX_BUFFER_LEN;
    k_delayed_work_submit(&handle->rx_dma_check_work, K_MSEC(DMA_RX_START_CHECK_MS));
#endif

    dbg_print("aic_uart_open() success\n");
    return handle;
}

void aic_uart_close(void *handle)
{
    uart_dma_handle_t *h = handle;
    /*disable irq*/
    k_delayed_work_cancel(&h->rx_dma_check_work);
    uart_irq_tx_disable(h->dev);
    uart_irq_rx_disable(h->dev);
    uart_dma_send_stop(h->dev);
    uart_dma_send_exit(h->dev);
    uart_dma_receive_stop(h->dev);
    uart_dma_receive_exit(h->dev);
    aos_free(handle);
    dbg_print("aic_uart_close() success\n");
}
