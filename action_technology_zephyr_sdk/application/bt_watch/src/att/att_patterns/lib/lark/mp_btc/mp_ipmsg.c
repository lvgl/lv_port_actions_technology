#include "att_pattern_test.h"
#include "mp_lark.h"
#include "rbuf/rbuf_msg.h"
#include "byteorder.h"
#include "soc_atp.h"
#include "mp_btc_inner.h"

#ifndef BIT
#define BIT(n)  (1UL << (n))
#endif

static struct btc_dev{
	/* message send to bt cpu */
	rbuf_msg_t msg;
	uint32_t msg_tx_id;
	uint32_t msg_rx_id;
	uint32_t hci_tx_id;
	uint32_t hci_rx_id;
	uint32_t log_rx_id;
} btc;

static unsigned int rbuf_put_data(uint32_t id, void *data, uint16_t len)
{
	uint32_t size;
	void *buf = NULL;

	if (!data || !len) {
		return 0;
	}

	buf = rbuf_put_claim(RBUF_FR_OF(id), len, &size);
	if (!buf) {
		return 0;
	}

	memcpy(buf, data, MIN(len, size));
	rbuf_put_finish(RBUF_FR_OF(id), size);

	return size;
}

static unsigned int rbuf_get_data(uint32_t id, void *dst, uint16_t len)
{
	uint32_t size;
	void *buf = NULL;

	if (!dst || !len) {
		return 0;
	}

	buf = rbuf_get_claim(RBUF_FR_OF(id), len, &size);
	if (!buf) {
		return 0;
	}

	memcpy(dst, buf, MIN(len, size));
	rbuf_get_finish(RBUF_FR_OF(id), size);

	return size;
}

void ipmsg_btc_update_bt_table(void *table_addr)
{
	int err;
	uint32_t calib_val = 0;
	uint8_t *start = table_addr;

	LOG_I("Befor set 0x42 = 0x%x 0x43 = 0x%x 0x44 = 0x%x\n", start[0x42], start[0x43], start[0x44]);

    //if(!opt){
    	err = soc_atp_get_rf_calib(EFUSE_AVDD_IF, &calib_val);
    	if (err == 0) {
    		sys_put_le32(calib_val, &start[BT_CFG_EFUSE_AVDD_IF_POS]);
			LOG_D("get efuse avddif 0x%x\n", calib_val);
    	} else {
    		LOG_E("get efuse avdd if failed %d", err);
    	}

    	err = soc_atp_get_rf_calib(EFUSE_POWER_VER, &calib_val);
    	if (err == 0) {
    		sys_put_le32(calib_val, &start[BT_CFG_EFUSE_POWER_VER_POS]);
			LOG_D("get efuse power version 0x%x\n", calib_val);
    	} else {
    		LOG_E("get efuse power version failed %d", err);
    	}

    	err = soc_atp_get_rf_calib(EFUSE_RF, &calib_val);
    	if (err == 0) {
    		sys_put_le32(calib_val, &start[BT_CFG_EFUSE_RF_POS]);
			LOG_D("get efuse rf 0x%x\n", calib_val);
    	} else {
    		LOG_E("get efuse rf failed %d", err);
    	}

    	start[BT_CFG_EFUSE_SET_POS] = (start[BT_CFG_EFUSE_SET_POS] | BIT(BT_CFG_EFUSE_SET_BIT));
    //}

#ifdef CONFIG_IPMSG_BTC_SEL_32K
	start[BT_CFG_SEL_32768_POS] = (start[BT_CFG_SEL_32768_POS] | (0x1 << BT_CFG_SEL_32768_BIT));		/* Set 1 */
#else
	start[BT_CFG_SEL_32768_POS] = (start[BT_CFG_SEL_32768_POS] & (~(0x1 << BT_CFG_SEL_32768_BIT)));		/* Set 0 */
#endif

	start[BT_CFG_FORCE_LIGHT_SLEEP_POS] = (start[BT_CFG_FORCE_LIGHT_SLEEP_POS] & (~(0x1 << BT_CFG_FORCE_LIGHT_SLEEP_BIT)));		/* Set 0 */

#ifdef CONFIG_BT_POWER_TEST_MODE
	start[BT_CFG_LE_USING_US_TIMER_POS] = (start[BT_CFG_LE_USING_US_TIMER_POS] & (~(0x1 << BT_CFG_LE_USING_US_TIMER_BIT)));		/* Set 0 */
#endif

#ifdef CONFIG_BT_CTRL_LOG
	start[BT_CFG_FORCE_UART_PRINT_POS] = (start[BT_CFG_FORCE_UART_PRINT_POS] & (~(0x1 << BT_CFG_FORCE_UART_PRINT_BIT)));			/* Set 0 */
#else
	start[BT_CFG_FORCE_UART_PRINT_POS] = (start[BT_CFG_FORCE_UART_PRINT_POS] | (0x1 << BT_CFG_FORCE_UART_PRINT_BIT));			/* Set 1 */
#endif

#ifdef CONFIG_BT_TRANSMIT
	start[BT_CFG_TRANS_EN_POS] = (start[BT_CFG_TRANS_EN_POS] | (0x1 << BT_CFG_TRANS_EN_BIT));
#endif

	start[BT_CFG_SEL_RC32K_POS] = (start[BT_CFG_SEL_RC32K_POS] & (~(0x1 << BT_CFG_SEL_RC32K_BIT)));
	//start[BT_CFG_WAKEUP_ADVANCE_POS] = 0x04;

#ifndef CONFIG_BOARD_LARK_DVB_EARPHONE
	start[BT_CFG_UART_BAUD_RATE_POS] = (uint8_t)(BT_CFG_UART_BAUD_RATE & 0xFF);
	start[BT_CFG_UART_BAUD_RATE_POS + 1] = (uint8_t)((BT_CFG_UART_BAUD_RATE >> 8) & 0xFF);
	start[BT_CFG_UART_BAUD_RATE_POS + 2] = (uint8_t)((BT_CFG_UART_BAUD_RATE >> 16) & 0xFF);
	start[BT_CFG_UART_BAUD_RATE_POS + 3] = (uint8_t)((BT_CFG_UART_BAUD_RATE >> 24) & 0xFF);
	LOG_I("Bt uart rate %d\n", BT_CFG_UART_BAUD_RATE);
#endif

	//if (btc_set_param.set_max_rf_power) {
	//	start[BT_MAX_RF_POWER_POS_1] = btc_set_param.bt_max_rf_tx_power;
	//	start[BT_MAX_RF_POWER_POS_2] = btc_set_param.bt_max_rf_tx_power;
	//	LOG_I("max rf power %d\n", btc_set_param.bt_max_rf_tx_power);

#if 0
		/* Fixed tx power */
		#define BT_FIX_POWER_LEVEL	38		/* 8db */
		start[BT_MAX_RF_POWER_POS_1] = BT_FIX_POWER_LEVEL;
		start[BT_MAX_RF_POWER_POS_2] = BT_FIX_POWER_LEVEL;
		start[BT_DEFAULT_TX_POWER] = BT_FIX_POWER_LEVEL;
		start[BT_CFG_FIX_MAX_PWR_POS] = start[BT_CFG_FIX_MAX_PWR_POS] | (0x1 << BT_CFG_FIX_MAX_PWR_BIT);
		LOG_D("After set 0x22 = %d 0x23 = %d 0x25 = %d\n", start[0x22], start[0x23], start[0x25]);
		LOG_D("After set 0x40 = 0x%x\n", start[BT_CFG_FIX_MAX_PWR_POS]);
#endif
	//}

	//if (btc_set_param.set_ble_rf_power) {
	//	start[BT_BLE_RF_POWER_POS] = btc_set_param.ble_rf_tx_power;
	//	LOG_I("ble rf tx power %d\n", start[BT_BLE_RF_POWER_POS]);
	//}

	LOG_I("After set 0x42 = 0x%x 0x43 = 0x%x 0x44 = 0x%x\n", start[0x42], start[0x43], start[0x44]);
}



void mp_btc_ipmsg_init(void)
{
	memset(&btc, 0, sizeof(btc));
	rbuf_pool_init((char*)RB_ST_POOL, RB_SZ_POOL);

	btc.msg_tx_id = rbuf_msg_create(CPU, BT, RB_MSG_SIZE);
	btc.msg_rx_id = rbuf_msg_create(BT, CPU, RB_MSG_SIZE);

	LOG_I("msg tx id: %d, msg rx id: %d\n", btc.msg_tx_id, btc.msg_rx_id);

	btc.log_rx_id = ipmsg_create(RBUF_RAW, BTC_LOG_SIZE);
	LOG_I("log rx id: %d\n", btc.log_rx_id);

	btc.hci_rx_id = ipmsg_create(RBUF_RAW, CONFIG_BT_HCI_RX_RBUF_SIZE);
	btc.hci_tx_id = ipmsg_create(RBUF_RAW, CONFIG_BT_HCI_TX_RBUF_SIZE);
	LOG_I("hci rx id: %d, hci tx id: %d\n", btc.hci_rx_id, btc.hci_tx_id);

	btc.msg.type = MSG_BT_HCI_BUF;
	btc.msg.data.w[0] = btc.hci_rx_id;  /* bt -> main */
	btc.msg.data.w[1] = btc.hci_tx_id;  /* main -> bt */
	btc.msg.data.w[2] = btc.log_rx_id;  /* bt -> main */
	rbuf_put_data(btc.msg_tx_id, &btc.msg, sizeof(rbuf_msg_t));

	while(1){
		if(mp_btc_log_buf_process()){
			break;
		}
	}
}

static char log_buf[BTC_LOG_SIZE + 1];

static int btc_log_handler(void *context, void *data, unsigned int size)
{
	memcpy((void *)log_buf, data, size);
	log_buf[size] = '\0';
	printf("%s", log_buf);

	return size;
}

/* BTC logbuf handler */
int mp_btc_log_buf_process(void)
{
	int log_data = false;

	if (ipmsg_pending(btc.log_rx_id)) {
		while(1){
			if (!ipmsg_pending(btc.log_rx_id)) {
				break;
			}

			ipmsg_recv(btc.log_rx_id, BTC_LOG_SIZE, btc_log_handler, NULL);
		}

		log_data = true;
	}

	return log_data;

}

typedef struct tx_data {
	uint32_t tx_id;
	/* total length of hci data, round up to 4-byte */
	uint16_t total;
	/* current offset of hci packet */
	uint16_t offset;
	uint8_t *data;
} tx_data_info_t;

unsigned int send_buf(tx_data_info_t *info)
{
	uint8_t *buf = NULL;
	uint16_t copy_len;
	unsigned int size = 0;

	buf = rbuf_put_claim(RBUF_FR_OF(info->tx_id), info->total, &size);
	if (buf == NULL) {
		printf("alloc hci tx rbuf failed");
		return 0;
	}

	copy_len = MIN(size, info->total - info->offset);

	memcpy(buf, info->data + info->offset, copy_len);
	info->offset += copy_len;

	rbuf_put_finish(RBUF_FR_OF(info->tx_id), size);

	info->total -= size;

	return size;
}

/* data fmt: hdr(4) + payload(n) */
int btdrv_send(uint32_t type, uint8_t *data, uint16_t len)
{
	int err = 0;
	unsigned int size = 0;
	//uint32_t cycle = k_cycle_get_32();

	tx_data_info_t data_info;

	if (!data || !len) {
		return -1;
	}

	memset(&data_info, 0, sizeof(data_info));

	data_info.data = data;
	data_info.total = len;

	if(type == 0){
		data_info.tx_id = btc.msg_tx_id;
	}else if(type == 1){
		data_info.tx_id = btc.hci_tx_id;
	}

	do {
		size = send_buf(&data_info);
	} while (size && data_info.total);

	if (data_info.total) {
		err = -1;
		printf("send hci rbuf failed");
	}

	//LOG_DBG("tx cycle: %u\n", k_cycle_get_32() - cycle);
	return err;
}

enum {
	PENDING_READ_BB_REG = (1 << 0),
	PENDING_READ_RF_REG = (1 << 1),
	NUM_REG_FLAGS,
};



static struct {
	int flag;
	uint16_t size;
	/* baseband register address */
	uint32_t bb_reg;
	uint16_t rf_reg;
} btc_reg;

static void btc_read_reg(void)
{
	unsigned int size, i;
	void *buf;

	buf = rbuf_get_claim(RBUF_FR_OF(btc.msg_rx_id), btc_reg.size, &size);
	if (!buf) {
		return;
	}

	if (btc_reg.flag & PENDING_READ_BB_REG) {
		uint32_t *reg_32 = buf;
		for (i = 0; i < btc_reg.size/4; i++) {
			printf("0x%08x: 0x%08x", btc_reg.bb_reg + i*4, reg_32[i]);
		}
		btc_reg.flag &= (~PENDING_READ_BB_REG);
	} else if(btc_reg.flag & PENDING_READ_RF_REG) {
		uint16_t *reg_16 = buf;
		for (i = 0; i < btc_reg.size/4; i++) {
			printf("0x%04x: 0x%04x", btc_reg.rf_reg + i, reg_16[i]);
		}
		btc_reg.flag &= (~PENDING_READ_RF_REG);
	} else {
		printf("Unknown reg to read");
	}

	rbuf_get_finish(RBUF_FR_OF(btc.msg_rx_id), size);
}

static void btc_msg_handler(void)
{
	uint16_t ret;
	rbuf_msg_t rx_msg = {0};

	ret = rbuf_get_data(btc.msg_rx_id, &rx_msg, sizeof(rbuf_msg_t));
	if (ret != sizeof(rbuf_msg_t)) {
		return;
	}

	printf("msg type: %x, data size: %u\n", rx_msg.type, rx_msg.data.w[0]);

	switch (rx_msg.type) {
	case MSG_BT_HCI_OK:
		printf("hci init ok\n");
		break;

	case MSG_BT_READ_BB_REG:
		btc_reg.size = rx_msg.data.w[0];
		btc_reg.bb_reg = rx_msg.data.w[1];
		btc_reg.flag |= PENDING_READ_BB_REG;
		btc_read_reg();
		break;
	case MSG_BT_READ_RF_REG:
		btc_reg.size = rx_msg.data.w[0];
		btc_reg.rf_reg = rx_msg.data.w[1];
		btc_reg.flag |= PENDING_READ_RF_REG;
		btc_read_reg();
		break;

	default:
		printf("unknown msg(type %x)!", rx_msg.type);
		break;
	}
}


/* BTC interupt handler */
void btc_recv_cb(void)
{
	if (ipmsg_pending(btc.msg_rx_id)) {
		btc_msg_handler();
	}

	if (ipmsg_pending(btc.hci_rx_id)) {
		btc_msg_handler();
	}

	if (ipmsg_pending(btc.log_rx_id)) {
		while(1){
			if (!ipmsg_pending(btc.log_rx_id)) {
				break;
			}

			ipmsg_recv(btc.log_rx_id, BTC_LOG_SIZE, btc_log_handler, NULL);
		}
	}

}

int mp_btc_read_hci_data(uint8_t *data, uint32_t len)
{
	int ret;
	while(1){

		mp_btc_log_buf_process();

		if(ipmsg_pending(btc.hci_rx_id)){
			ret = rbuf_get_data(btc.hci_rx_id, data, len);

			if(ret == len){
				return ret;
			}

			break;
		}
	}

	return -1;
}

int mp_btc_clear_hci_rx_buffer(void)
{
	uint32_t size;
	int id = btc.hci_rx_id;
	int loop = 0;
	while(1){

		mp_btc_log_buf_process();

		if(ipmsg_pending(id)){
			rbuf_get_claim(RBUF_FR_OF(id), 0x10000, &size);

			rbuf_get_finish(RBUF_FR_OF(id), size);
		}else{
			loop++;

			udelay(100);

			if(loop == 10){
				break;
			}
		}
	}

	return 0;
}


