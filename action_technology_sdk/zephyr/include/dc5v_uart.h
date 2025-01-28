/*!
 * \file      dc5v_uart.h
 * \brief     DC5V_UART driver header
 * \details
 * \author    
 * \date
 * \copyright Actions
 */

#include <soc.h>
#include <string.h>
#include "os_common_api.h"
#include "config.h"
#include <sys/ring_buffer.h>
#include <drivers/uart_dma.h>
#include <drivers/uart.h>


#define PMU_SYS_SET_REG			    (0x40004000 + 0x108)
#define PMU_WKUP_CTL_REG            (0x40004000 + 0x110)
#define PMU_WKUP_PD_REG             (0x40004000 + 0x114)
#define GPIO_DC5V_CTL_REG			(0x40068000 + 0x310)

/* register bits start */

#define DC5V_CTL_50KPU_EN           BIT(11)
#define DC5V_CTL_10KPU_EN           BIT(8)

#define DC5V_CTL_AD_SEL             BIT(4)
#define DC5V_CTL_MFP_SHIFT          0
#define DC5V_CTL_MFP_MASK           (0x0f << DC5V_CTL_MFP_SHIFT)
#define DC5V_CTL_MFP_SET(x)         ((x) << DC5V_CTL_MFP_SHIFT)
#define DC5V_CTL_MFP_GPIO           DC5V_CTL_MFP_SET(0)
#define DC5V_CTL_MFP_UART1TX        DC5V_CTL_MFP_SET(1)
#define DC5V_CTL_MFP_UART1RX        DC5V_CTL_MFP_SET(2)
#define DC5V_CTL_MFP_UART2TX        DC5V_CTL_MFP_SET(3)
#define DC5V_CTL_MFP_UART2RX        DC5V_CTL_MFP_SET(4)

#define SYSSET_UART_PWR_SEL         BIT(14)
#define SYSSET_UART_SW_MODE         BIT(13)
#define SYSSET_UART_SW_SEL          BIT(12)

#define WKEN_CTL_DC5V_LHV_VOL_SHIFT    24
#define WKEN_CTL_DC5V_LHV_VOL_MASK     (0x07 << WKEN_CTL_DC5V_LHV_VOL_SHIFT)
#define WKEN_CTL_DC5VLV_WKEN_SHIFT     10
#define WKEN_CTL_DC5VLV_WKEN           BIT(10)

/* register bits end */


#define UART_CTRL_RX_DMA_BUF_SIZE      200
#define UART_CTRL_RX_DATA_BUF_SIZE     256

#define UART_RX_DATADEAL_STACK         1024
#define UART_RX_THREAD_PRIO            8

#define DC5V_UART_DMA_IRQ_TC                         (0) /* DMA completion flag */
#define DC5V_UART_DMA_IRQ_HF                         (1) /* DMA half-full flag */

/* 定时一小段时间后将 DMA 接收的数据保存至 data_buf
 */
#define UART_CTRL_RX_DMA_TIMER_US  300

/* DMA 启动后再没有接收到数据后维持一段时间再停止
 */
#define UART_CTRL_RX_DMA_KEEP_MS  20



typedef enum
{
    DC5V_UART_RX_MODE = 0,
	DC5V_UART_TX_MODE,
}dc5v_uart_mode_e;


typedef enum
{
    UART_SET_BAUDRATE = 1,
    UART_INQUIRY_STATE,
    UART_CHECK_WRITE_FINISH,

    UART_IS_RX_FIFO_EMPTY = 10,
    UART_READ_BYTE_DATA,
    UART_RX_DMA_ACCESS_SWITCH,
    UART_RX_DRQ_SWITCH,
    UART_RX_IRQ_SWITCH,
    UART_CLEAR_RX_IRQ_PENDING,
}uart_ioctl_cmd_e;


typedef enum
{
    DC5V_UART_INIT = 1,
    DC5V_UART_SUSPEND,
    DC5V_UART_RESUME,
    DC5V_UART_SET_RX_DATA_HANDLER,
    DC5V_UART_SET_RX_BUF_SIZE,
    DC5V_UART_READ,
    DC5V_UART_WRITE,
    DC5V_UART_IOCTL,
    DC5V_UART_CHECK_IO_TIME,
    DC5V_UART_SET_ENABLE,
    DC5V_UART_RUN_RXDEAL,
    DC5V_UART_STOP_RXDEAL,
}DC5V_UART_OPS;


typedef struct
{
    u8_t   dma_buf [UART_CTRL_RX_DMA_BUF_SIZE];
    u8_t   data_buf[UART_CTRL_RX_DATA_BUF_SIZE];

	const struct device *rx_dev; 

	struct ring_buf rx_rbuf;
    struct k_timer  rx_timer;	
    
    //u16_t  wait_count;

	bool (*rx_data_handler)(u8_t byte);
} uart_ctrl_rx_context_t;


typedef struct
{
    const struct device *tx_dev;

    u16_t  tx_len;
	u8_t*  tx_buf;

    bool   tx_switch_finish;
    struct k_timer  tx_switch_timer;
    struct k_timer  tx_check_timer;	
} uart_ctrl_tx_context_t;


typedef struct
{
    CFG_Type_DC5V_UART_Comm_Settings  cfg;  

    u8_t   dc5v_uart_init;
    u8_t   trx_mode;                                 
    u8_t   redirect_console_print;                   
    u8_t   suspended;
    u8_t   enabled;
	u8_t   rxdeal_need_quit;
	u8_t   rxdeal_quited;

    u32_t  last_io_time;
	u32_t  baud_rate;

	u32_t bak_PMU_SYSTEM_SET_SVCC;
	u32_t bak_GPIO_DC5V_CTL;
	char* rxdeal_thread_stack;

	uart_ctrl_tx_context_t  tx_context;              
    uart_ctrl_rx_context_t  rx_context;   

	u32_t  disable_delay_ms;
    struct k_timer  disable_timer;
} dc5v_uart_context_t;


extern dc5v_uart_context_t  dc5v_uart_context;


extern int dc5v_uart_operate(u32_t cmd, void* param1, u32_t param2, u32_t param3);

