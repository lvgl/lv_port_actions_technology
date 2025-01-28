#include "ap_autotest_rf_rx.h"
#include "mp_btc.h"
#include <bt_production_test.h>

#define MP_BTC_NAME "mp_btc.bin"

//#define MP_BTC_NAME "mp_btc.bin"
#define	UID1                        (0x40000000+0x000000A4)

static u8_t mp_bt_get_ic_pkt(void)
{
	uint32_t val;
	val = sys_read32(UID1);
	return (u8_t)((val >> 24) & 0xF);        /* bit24~27: IC Package */
}

static const char *mp_bt_get_bt_bin_name(void)
{
	u8_t pkt = mp_bt_get_ic_pkt();
	printk("pkt=%d\n", pkt);
	switch (pkt) {
	case 0x8:
		return "mp_btc9.bin";
	case 0xE:
	case 0xF:
		return "mp_btcp.bin";
	default:
		return "mp_btcx.bin";
	}
}

void mp_bt_load_bt_bin(void)
{
	atf_dir_t dir;
	int ret_val;
	const char *mp_bt_name = mp_bt_get_bt_bin_name();
	ret_val = read_atf_sub_file(NULL, 0x10000, (const u8_t *)mp_bt_name, 0, -1, &dir);
	if (ret_val <= 0) {
		printk("read_file %s fail, quit!\n", mp_bt_name);
	}
}

void mp_btc_init(void)
{
	int ret;

	memset(&global_ft_env_var, 0, sizeof(global_ft_env_var));
	global_ft_env_var.ft_udelay = self->api->udelay;
	global_ft_env_var.ft_mdelay = self->api->k_sleep;
	global_ft_env_var.ft_printf = att_buf_printf;
	global_ft_env_var.ft_get_time_ms = self->api->k_uptime_get;
	global_ft_env_var.ft_load_fcc_bin = mp_bt_load_bt_bin;

	ret = fcc_test_main(FCC_BT_ATT_MODE, NULL, NULL);
	if (ret) {
		LOG_E("mp_btc_init failed\n");
	}
}

void mp_btc_deinit(void)
{
	int ret;

	ret = fcc_test_deinit();
	if (ret) {
		LOG_E("mp_btc_deinit failed\n");
	}
}
