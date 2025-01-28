#include "ap_autotest_rf_rx.h"
#include <soc.h>
#include <bt_production_test.h>

uint8 bt_rx_cmd[] = {
    K_CMD_SET_FREQ,
    // rf_channel
    36,

#if 0
    K_CMD_CFG_AGC,
    // m_bAgcEn
    0,
#endif

    K_CMD_SET_ACCESSCODE,
    // nAccessCode0
    0xe2,
    0x3a,
    0x1a,
    0x33,
    // nAccessCode1
    0xce,
    0x2c,
    0x7a,
    0x4e,

    K_CMD_SET_PAYLOAD,
    // nPayloadMode
    K_FCC_01010101,

    K_CMD_SET_RX_MODE,
    K_RX_MODE_DH1,
};


static int search_cmd(u8_t cmd)
{
	int i;
	for(i = 0; i < sizeof(bt_rx_cmd); i++){
		if(bt_rx_cmd[i] == cmd){
			return i;
		}
	}

	return -1;
}

int mp_btc_rx_init(uint8_t channel)
{
	int index, ret;

	index = search_cmd(K_CMD_SET_FREQ);
	bt_rx_cmd[index + 1] = channel;
	index = search_cmd(K_CMD_SET_RX_MODE);
	//bt_rx_cmd[index + 1] = K_RX_MODE_DH1;
	bt_rx_cmd[index + 1] = K_RX_MODE_DH1;
	index = search_cmd(K_CMD_SET_PAYLOAD);
	//bt_rx_cmd[index + 1] = K_FCC_PN9;
	bt_rx_cmd[index+1] = K_FCC_01010101;

	ret = fcc_test_send_cmd(bt_rx_cmd, sizeof(bt_rx_cmd), 1);
	if (ret) {
		LOG_E("mp_btc_rx_init failed\n");
	}
}


int mp_btc_rx_begin(void)
{
	uint8_t tx_data[2];
	int ret;

	tx_data[0] = K_CMD_RX_EXCUTE;
	//0:single receive 1:continus receive
	tx_data[1] = 1;
	ret = fcc_test_send_cmd(tx_data, sizeof(tx_data), 0);
	if (ret) {
		LOG_E("mp_btc_rx_begin failed\n");
	}
}

int mp_btc_rx_stop(void)
{
	uint8_t tx_data;
	int ret;

	tx_data = K_CMD_RX_EXCUTE_STOP;
	ret = fcc_test_send_cmd(&tx_data, sizeof(tx_data), 1);
	if (ret) {
		LOG_E("mp_btc_rx_stop failed\n");
	}
}

int mp_btc_get_report(uint8_t *buf)
{
	uint8_t tx_data;
	int ret;

	fcc_test_clear_rx_buf();

	tx_data = K_CMD_GET_RX_REPORT;
	ret = fcc_test_get_report(tx_data, buf, 16);
	if (ret) {
		LOG_E("mp_btc_get_report failed\n");
	}

	return ret;
}

int mp_btc_get_cfo_rssi(int16_t *cfo, int16_t *rssi)
{
	uint8_t tx_data, buf[5];
	uint16_t value;
	int ret;

	fcc_test_clear_rx_buf();

	tx_data = K_CMD_GET_RX_CFO_RSSI;
	ret = fcc_test_get_report(tx_data, buf, 5);
	if (ret) {
		LOG_E("mp_btc_get_cfo_rssi failed\n");
	} else {
		if (buf[0] == 0x00) {
			return -1;
		}

		value = buf[1] | (buf[2] << 8);
		*cfo = (int16_t)value;
		value = buf[3] | (buf[4] << 8);
		*rssi = (int16_t)value;
		//LOG_I("mp_btc_get cfo %d rssi %d\n", *cfo, *rssi);
	}

	return ret;
}
