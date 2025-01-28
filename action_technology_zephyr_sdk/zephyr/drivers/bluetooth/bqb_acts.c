#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <kernel.h>
#include <device.h>
#include <init.h>
#include <soc.h>
#include <drivers/ipmsg.h>
#include <sys/printk.h>
#include <sys/byteorder.h>
#include <drivers/uart.h>
#include <drivers/uart_dma.h>
#include <drivers/bluetooth/bt_drv.h>

#ifdef CONFIG_PROPERTY
#include <property_manager.h>
#endif

#if defined(CONFIG_BT_QC_TEST) || defined(CONFIG_BT_CTRL_RF_DEBUG)
#define BQB_USE_UART_0							0
#else
#define BQB_USE_UART_0							1
#endif

#define BT_HCI_OP_VS_WRITE_BB_REG				0xfc8a
#define BT_HCI_OP_VS_READ_BB_REG				0xfc8b
#define BT_HCI_OP_VS_WRITE_RF_REG				0xfc8c
#define BT_HCI_OP_VS_READ_RF_REG				0xfc8d
#define BT_HCI_EVT_VS_READ_BB_REG_REPORT		0x80
#define BT_HCI_EVT_VS_READ_RF_REG_REPORT		0x81
#define BT_HCI_OP_VS_SET_APLL_TEMP_COMP	     0xfc30
#define BT_HCI_OP_VS_DO_APLL_TEMP_COMP	     0xfc31

struct bqb_bb_reg_report {
	uint32_t base_addr;
	uint32_t size;
	uint32_t result[0];
} __packed;

struct bqb_rf_reg_report {
	uint16_t base_addr;
	uint16_t size;
	uint16_t result[0];
} __packed;

#if BQB_USE_UART_0
#define BQB_UART_STA	UART0_STA
#define BQB_UART_SET_BR	115200
#define BLE_BQB_UART	"UART_0"
static struct device *uart_dev;
#else
#define BQB_UART_STA	UART1_STA
#define BLE_BQB_UART	"UART_1"
static struct device *uart_dev;
#endif

struct acts_bqb_data {
	uint8_t tx_data[128];
	uint8_t rx_data[128];
	uint8_t rx_type;
	uint8_t bqb_mode;
	uint8_t bqb_in_test;
};

static struct acts_bqb_data *bqb_info;

static void bqb_vs_evt_handle(uint8_t *buf)
{
    int i;
    struct bqb_bb_reg_report *bb_evt;
    struct bqb_rf_reg_report *rf_evt;

    uint8_t subevt = buf[0];
    buf++;

    if (subevt == BT_HCI_EVT_VS_READ_BB_REG_REPORT) {
        bb_evt = (void *)buf;
        for (i=0; i<bb_evt->size; i++) {
            printk("0x%08x: 0x%08x\n", bb_evt->base_addr + i*4, bb_evt->result[i]);
        }
    } else if (subevt == BT_HCI_EVT_VS_READ_RF_REG_REPORT) {
        rf_evt = (void *)buf;
        for (i=0; i<rf_evt->size; i++) {
            printk("0x%04x: 0x%04x\n", rf_evt->base_addr + i, rf_evt->result[i]);
        }
    } else {
        printk("[BQB] unknown vendor event\n");
    }
}

static void bqb_evt_handle(uint8_t *buf)
{
    uint8_t evt = buf[0];
    uint8_t len = buf[1];

    if (len <= 0) {
        printk("[BQB] buf empty\n");
    }

    if (evt == 0xff) {  //HCI Vendor Event
        bqb_vs_evt_handle(buf + HCI_EVT_HDR_SIZE);
    }
}

int bt_bqb_vs_write_bb_reg(uint32_t addr, uint32_t val)
{
	if (!bqb_info) {
		return -ENOMEM;
	}

    bqb_info->tx_data[0] = (uint8_t)BT_HCI_OP_VS_WRITE_BB_REG;
    bqb_info->tx_data[1] = (uint8_t)(BT_HCI_OP_VS_WRITE_BB_REG >> 8);
    bqb_info->tx_data[2] = 0x08;
    sys_put_le32(addr, &bqb_info->tx_data[3]);
    sys_put_le32(val, &bqb_info->tx_data[7]);

    return btdrv_send(HCI_CMD, bqb_info->tx_data, 11);
}

int bt_bqb_vs_read_bb_reg(uint32_t addr, uint8_t size)
{
	if (!bqb_info) {
		return -ENOMEM;
	}

    bqb_info->tx_data[0] = (uint8_t)BT_HCI_OP_VS_READ_BB_REG;
    bqb_info->tx_data[1] = (uint8_t)(BT_HCI_OP_VS_READ_BB_REG >> 8);
    bqb_info->tx_data[2] = 0x05;
    sys_put_le32(addr, &bqb_info->tx_data[3]);
    bqb_info->tx_data[7] = size;

    return btdrv_send(HCI_CMD, bqb_info->tx_data, 8);
}

int bt_bqb_vs_write_rf_reg(uint16_t addr, uint16_t val)
{
	if (!bqb_info) {
		return -ENOMEM;
	}

    bqb_info->tx_data[0] = (uint8_t)BT_HCI_OP_VS_WRITE_RF_REG;
    bqb_info->tx_data[1] = (uint8_t)(BT_HCI_OP_VS_WRITE_RF_REG >> 8);
    bqb_info->tx_data[2] = 0x04;
    sys_put_le16(addr, &bqb_info->tx_data[3]);
    sys_put_le16(val, &bqb_info->tx_data[5]);

    return btdrv_send(HCI_CMD, bqb_info->tx_data, 7);
}

int bt_bqb_vs_read_rf_reg(uint16_t addr, uint8_t size)
{
	if (!bqb_info) {
		return -ENOMEM;
	}

    bqb_info->tx_data[0] = (uint8_t)BT_HCI_OP_VS_READ_RF_REG;
    bqb_info->tx_data[1] = (uint8_t)(BT_HCI_OP_VS_READ_RF_REG >> 8);
    bqb_info->tx_data[2] = 0x03;
    sys_put_le16(addr, &bqb_info->tx_data[3]);
    bqb_info->tx_data[5] = size;

    return btdrv_send(HCI_CMD, bqb_info->tx_data, 6);
}

#define BT_NAME_LEN		(32+1)	/* 32(len) + 1(NULL) */

static void bt_dut_mode_set_name(void)
{
#ifdef CONFIG_PROPERTY
	uint8_t *cfg_name = CFG_BT_NAME;
	int ret;

	ret = property_get(cfg_name, &bqb_info->tx_data[6], (BT_NAME_LEN-1));
	if (ret < 0) {
		return;
	}

	bqb_info->tx_data[0] = 0x52;
	bqb_info->tx_data[1] = 0x0c;
	bqb_info->tx_data[2] = (uint8_t)(ret + 3 + 2);
	bqb_info->tx_data[3] = 0x00;
	bqb_info->tx_data[4] = (uint8_t)(ret + 3);
	bqb_info->tx_data[5] = 0x09;

	bqb_info->tx_data[6 + ret] = 0x00;
	bqb_info->tx_data[6 + ret + 1] = 0x00;
	btdrv_send(HCI_CMD, bqb_info->tx_data, (uint16_t)(6 + ret + 2));/* HCI set name */
	k_sleep(K_MSEC(20));
#endif
}

void bt_dut_mode(void)
{
	if (!bqb_info) {
		return;
	}

    bqb_info->tx_data[0] = 0x03;
    bqb_info->tx_data[1] = 0x0c;        /* ogf: baseband */
    bqb_info->tx_data[2] = 0x00;        /* len */
    btdrv_send(HCI_CMD, bqb_info->tx_data, 3);   /* HCI reset */
    k_sleep(K_MSEC(50));

	bt_dut_mode_set_name();

	bqb_info->tx_data[0] = 0x1e;	/* write inquiry scan activity */
	bqb_info->tx_data[1] = 0x0c;
	bqb_info->tx_data[2] = 0x04;
	bqb_info->tx_data[3] = 0x00;
	bqb_info->tx_data[4] = 0x02;
	bqb_info->tx_data[5] = 0x60;
	bqb_info->tx_data[6] = 0x00;
	btdrv_send(HCI_CMD, bqb_info->tx_data, 7);
	k_sleep(K_MSEC(20));

	bqb_info->tx_data[0] = 0x1c;	/* write page scan activity */
	bqb_info->tx_data[1] = 0x0c;
	bqb_info->tx_data[2] = 0x04;
	bqb_info->tx_data[3] = 0x00;
	bqb_info->tx_data[4] = 0x04;
	bqb_info->tx_data[5] = 0x80;
	bqb_info->tx_data[6] = 0x00;
	btdrv_send(HCI_CMD, bqb_info->tx_data, 7);
	k_sleep(K_MSEC(20));

	bqb_info->tx_data[0] = 0x43;	/* write inquiry scan type */
	bqb_info->tx_data[1] = 0x0c;
	bqb_info->tx_data[2] = 0x01;
	bqb_info->tx_data[3] = 0x01;
	btdrv_send(HCI_CMD, bqb_info->tx_data, 4);
	k_sleep(K_MSEC(20));

	bqb_info->tx_data[0] = 0x47;	/* write page scan type */
	bqb_info->tx_data[1] = 0x0c;
	bqb_info->tx_data[2] = 0x01;
	bqb_info->tx_data[3] = 0x01;
	btdrv_send(HCI_CMD, bqb_info->tx_data, 4);
	k_sleep(K_MSEC(20));

    bqb_info->tx_data[0] = 0x1a;
    bqb_info->tx_data[1] = 0x0c;
    bqb_info->tx_data[2] = 0x01;
    bqb_info->tx_data[3] = 0x03;
    btdrv_send(HCI_CMD, bqb_info->tx_data, 4);   /* HCI write scan */
    k_sleep(K_MSEC(20));

    bqb_info->tx_data[0] = 0x05;
    bqb_info->tx_data[1] = 0x0c;
    bqb_info->tx_data[2] = 0x03;
    bqb_info->tx_data[3] = 0x02;
    bqb_info->tx_data[4] = 0x00;
    bqb_info->tx_data[5] = 0x02;
    btdrv_send(HCI_CMD, bqb_info->tx_data, 6);   /* HCI set event filter */
    k_sleep(K_MSEC(20));

    bqb_info->tx_data[0] = 0x03;
    bqb_info->tx_data[1] = 0x18;        /* ogf: test */
    bqb_info->tx_data[2] = 0x00;
    btdrv_send(HCI_CMD, bqb_info->tx_data, 3);   /* HCI dut mode */
    k_sleep(K_MSEC(20));
}

static void bqb_uart_send(uint8_t *tx_data, uint16_t len)
{
    int i;

	if (!bqb_info) {
		return;
	}

    for (i = 0; i < len; i++) {
        uart_poll_out(uart_dev, tx_data[i]);
    }
}

static int bqb_uart_tx_hci(uint8_t type, uint8_t *data)
{
    uint16_t len;

    switch (type) {
    case HCI_ACL:
        len = (data[3]<<8 | data[2]) + HCI_ACL_HDR_SIZE;
        break;
    case HCI_SCO:
        len = data[2] + HCI_SCO_HDR_SIZE;
        break;
    case HCI_EVT:
        len = data[1] + HCI_EVT_HDR_SIZE;
        break;
    case HCI_ISO:
        len = (data[3]<<8 | data[2]) + HCI_ISO_HDR_SIZE;
        break;
    default:
        return -EINVAL;
    }

    uart_poll_out(uart_dev, type);
    bqb_uart_send(data, len);

    return 0;
}

static uint16_t bqb_uart_read(uint8_t *rx_data, uint16_t size)
{
    uint16_t i;

	if (!bqb_info) {
		return 0;
	}

    for (i = 0; i < size; i++) {
        uart_poll_in(uart_dev, &rx_data[i]);
    }

    return i;
}

static void bqb_uart_rx_hci(uint8_t *buf)
{
    uint8_t type = 0;
    bool has_hdr = false;
    uint16_t len, rd_len, total = 0;

    uart_poll_in(uart_dev, &type);
    //printk("read type: %d\n", type);

    switch (type) {
    case HCI_CMD:
        rd_len = bqb_uart_read(buf, HCI_CMD_HDR_SIZE);
        if (rd_len == HCI_CMD_HDR_SIZE) {
            has_hdr = true;
            len = buf[2];
        }
        break;
    case HCI_ACL:
        rd_len = bqb_uart_read(buf, HCI_ACL_HDR_SIZE);
        if (rd_len == HCI_ACL_HDR_SIZE) {
            has_hdr = true;
            len = buf[3]<<8 | buf[2];
        }
        break;
    case HCI_SCO:
        rd_len = bqb_uart_read(buf, HCI_SCO_HDR_SIZE);
        if (rd_len == HCI_SCO_HDR_SIZE) {
            has_hdr = true;
            len = buf[2];
        }
        break;
    case HCI_ISO:
        rd_len = bqb_uart_read(buf, HCI_ISO_HDR_SIZE);
        if (rd_len == HCI_ISO_HDR_SIZE) {
            has_hdr = true;
            len = buf[3]<<8 | buf[2];
        }
        break;
    default:
        printk("unknown type: %d\n", type);
        break;
    }

    //printk("read hdr: %d %d\n", rd_len, len);
    if (!has_hdr) {
        printk("uart rx incomplete\n");
        return;
    }

    total += rd_len;

    rd_len = bqb_uart_read(buf+rd_len, len);
    if (rd_len != len) {
        printk("uart rx incomplete\n");
        return;
    }

    total += rd_len;
    btdrv_send(type, buf, total);
}

static void bqb_uart_isr(const struct device *dev, void *user_data)
{
    ARG_UNUSED(dev);
	ARG_UNUSED(user_data);

    if (uart_irq_rx_ready(uart_dev)) {
		if (bqb_info) {
			bqb_uart_rx_hci(bqb_info->tx_data);
		}
    }

    /* Clear the interrupt */
    sys_write32(0x1, BQB_UART_STA);
}

#if BQB_USE_UART_0
#ifdef CONFIG_UART_CONSOLE
static int _stdout_hook_null(int c)
{
	(void)(c);

	return EOF;
}
#endif

static int bqb_uart_init(void)
{

	struct uart_config cfg;

	uart_dev = (struct device *)device_get_binding(BLE_BQB_UART);
	if (uart_dev == NULL) {
		printk("cannot get device %s\n", BLE_BQB_UART);
		return -ENODEV;
	}
#ifdef CONFIG_UART_CONSOLE
	extern void __printk_hook_install(int (*fn)(int));
	extern void __stdout_hook_install(int (*fn)(int));
	__printk_hook_install(_stdout_hook_null);
	__stdout_hook_install(_stdout_hook_null);
#endif

	uart_config_get(uart_dev, &cfg);
	cfg.baudrate = BQB_UART_SET_BR;
	uart_configure(uart_dev, &cfg);
#ifdef CONFIG_ACTIONS_PRINTK_DMA
	uart_dma_send_stop(uart_dev);
	uart_fifo_switch(uart_dev, 1, UART_FIFO_TYPE_CPU);
	uart_dma_receive_stop(uart_dev);
	uart_fifo_switch(uart_dev, 0, UART_FIFO_TYPE_CPU);
#endif
	uart_irq_rx_disable(uart_dev);
	uart_irq_callback_set(uart_dev, bqb_uart_isr);
	uart_irq_rx_enable(uart_dev);
	return 0;
}
#else
#ifdef CONFIG_BOARD_LARK_DVB_EARPHONE
#define BT_UART_MFP_SEL 6
static const struct acts_pin_config bqb_pin_cfg[] = {
	{12, BT_UART_MFP_SEL | GPIO_CTL_PADDRV_LEVEL(1) | GPIO_CTL_PULLUP},
	{13, BT_UART_MFP_SEL | GPIO_CTL_PADDRV_LEVEL(1) | GPIO_CTL_PULLUP},
};
#else
#define BT_UART_MFP_SEL 6
static const struct acts_pin_config bqb_pin_cfg[] = {
	{16, BT_UART_MFP_SEL | GPIO_CTL_PADDRV_LEVEL(1) | GPIO_CTL_PULLUP},
	{17, BT_UART_MFP_SEL | GPIO_CTL_PADDRV_LEVEL(1) | GPIO_CTL_PULLUP},
};
#endif
static int bqb_uart_init(void)
{
    acts_pinmux_setup_pins(bqb_pin_cfg, ARRAY_SIZE(bqb_pin_cfg));

    uart_dev = (struct device *)device_get_binding(BLE_BQB_UART);
    if (uart_dev == NULL) {
        printk("cannot get device %s\n", BLE_BQB_UART);
        return -ENODEV;
    }

    uart_irq_callback_set(uart_dev, bqb_uart_isr);
	uart_irq_rx_enable(uart_dev);

    return 0;
}
#endif

static void *bqb_init_info(void)
{
	uint32_t id;
	struct acts_bqb_data *bqb_info;

	id = ipmsg_create(RBUF_RAW, sizeof(struct acts_bqb_data));
	if (id == 0) {
		return NULL;
	}

	rbuf_t *rbuf = RBUF_FR_OF(id);
	bqb_info = (void *)RBUF_FR_OF(rbuf->buf_off);

	memset(bqb_info, 0, sizeof(struct acts_bqb_data));
	return bqb_info;
}

static uint8_t *bqb_get_buf(uint8_t type, uint8_t evt, uint16_t exp_len)
{
	if (bqb_info) {
		bqb_info->rx_type = type;
		return bqb_info->rx_data;
	} else {
		return NULL;
	}
}

static int bqb_recv(uint16_t len)
{
	if (bqb_info) {
		if (bqb_info->bqb_mode != 0) {
	        bqb_uart_tx_hci(bqb_info->rx_type, bqb_info->rx_data);
	    }

	    if (bqb_info->rx_type == HCI_EVT) {
	        bqb_evt_handle(bqb_info->rx_data);
	    }
	}

    return 0;
}

#ifdef CONFIG_BT_CTRL_DEBUG
#define BT_ANT_SW_SEL	23
static const struct acts_pin_config pin_cfg_le_bqb_dbg[] = {
	{56, BT_ANT_SW_SEL | GPIO_CTL_GPIO_OUTEN},
	{57, BT_ANT_SW_SEL | GPIO_CTL_GPIO_OUTEN},
};
#endif

static const btdrv_hci_cb_t cb = {
    .get_buf = bqb_get_buf,
    .recv = bqb_recv,
};

/* BQB mode [0:BR BQB Test, 1:BLE BQB Test, 2:BR/BLE dual BQB Test] */
int bqb_init(int bqb_mode)
{
	int err, need_uart;

	if (bqb_mode > 2) {
		printk("Error bqb_mode %d\n", bqb_mode);
		return -EINVAL;
	}

	bqb_info = bqb_init_info();
	if (!bqb_info) {
		printk("Error bqb_init_info failed!\n");
		return -ENOMEM;
	}

	bqb_info->bqb_in_test = 1;
	bqb_info->bqb_mode = (uint8_t)bqb_mode;
	need_uart = (bqb_info->bqb_mode == 0) ? 0 : 1;

	err = btdrv_init((btdrv_hci_cb_t *)&cb);
	if (err) {
		return -EINVAL;
	}

	if (need_uart) {
		err = bqb_uart_init();
		if (err) {
			printk("bqb uart init failed %d\n", err);
			return -EINVAL;
		}
	}

#ifdef CONFIG_BT_CTRL_DEBUG
	if (bqb_info->bqb_mode != 0) {
		acts_pinmux_setup_pins(pin_cfg_le_bqb_dbg, ARRAY_SIZE(pin_cfg_le_bqb_dbg));
	}
#endif

	if (bqb_info->bqb_mode != 1) {
		bt_dut_mode();
	}

	printk("BQB mode %d [0:BR BQB Test, 1:BLE BQB Test, 2:BR/BLE dual BQB Test]\n", bqb_mode);
	return 0;
}

bool bt_bqb_is_in_test(void)
{
	if (bqb_info) {
		return bqb_info->bqb_in_test ? true : false;
	} else {
		return false;
	}
}

int bt_bqb_vs_set_apll_temp_comp(uint8_t enable)
{
    if(!bt_bqb_is_in_test()){
        return -EIO;
    }
 
    bqb_info->tx_data[0] = (uint8_t)BT_HCI_OP_VS_SET_APLL_TEMP_COMP;
    bqb_info->tx_data[1] = (uint8_t)(BT_HCI_OP_VS_SET_APLL_TEMP_COMP >> 8);
    bqb_info->tx_data[2] = 0x01;
    bqb_info->tx_data[3] = enable;

    return btdrv_send(HCI_CMD, bqb_info->tx_data, 4);   
}

int bt_bqb_vs_do_apll_temp_comp(void)
{
    if(!bt_bqb_is_in_test()){
        return -EIO;
    }
 
    bqb_info->tx_data[0] = (uint8_t)BT_HCI_OP_VS_DO_APLL_TEMP_COMP;
    bqb_info->tx_data[1] = (uint8_t)(BT_HCI_OP_VS_DO_APLL_TEMP_COMP >> 8);
    bqb_info->tx_data[2] = 0x0;

    return btdrv_send(HCI_CMD, bqb_info->tx_data, 3);
}

