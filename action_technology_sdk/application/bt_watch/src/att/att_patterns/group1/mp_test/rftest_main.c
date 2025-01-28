/**
 *  ***************************************************************************
 *  Copyright (c) 2014-2020 Actions (Zhuhai) Technology. All rights reserved.
 *
 *  \file       adfus_main.c
 *  \brief      adfu server main
 *  \author     cailizhen
 *  \version    1.00
 *  \date       2020/5/7
 *  ***************************************************************************
 *
 *  \History:
 *  Version 1.00
 *       Initial release
 */
#if 0
#include "ap_autotest_rf_rx.h"
#include "att_testid_def.h"

bool pattern_main(struct att_env_var *para);
void soc_init(void);


struct att_env_var env_var;

/*!
 * \brief
 *      测试程序返回数据结构体指针
 */
typedef struct
{
    UINT8 *info_buf;
    UINT32 data_length;
}_return_info;

extern cfo_return_arg_t g_cfo_return_arg;
_return_info g_return_info;


int rf_rx_test_wrapper(void)
{
	env_var.test_id = TESTID_MP_TEST;
	//env_var.test_id = TESTID_MP_READ_TEST;
	env_var.arg_len = 1;
	env_var.test_mode = 0;
	env_var.return_data_buffer = NULL;
	env_var.rw_temp_buffer = NULL;
	env_var.exit_mode = 0;

	memset(&g_cfo_return_arg, 0, sizeof(g_cfo_return_arg));

	return pattern_main(&env_var);
}

extern u8_t __BSS_START__[];
extern u8_t __BSS_END__[];
char return_str_buffer[10 * 1024];
int str_index;

int log_to_pc(char *str, int len)
{
	if(str_index + len < sizeof(return_str_buffer)){
		memcpy(&return_str_buffer[str_index], str, len);
		str_index += len;
		return len;
	}

	return 0;
}

__attribute__((long_call, section(".entry"))) _return_info *rftest_main(void)
{
	u32_t baud_backup, uart_ctl_backup;

    u32_t test_time;

	memset(__BSS_START__, 0, __BSS_END__ - __BSS_START__);

	baud_backup = sys_read32(UART0_BR);
	uart_ctl_backup = sys_read32(UART0_CTL);

	soc_init();

	test_time = timestamp_get_us();

	//sys_write32(0x00100011, JTAG_CTL);

	rf_rx_test_wrapper();

	//LOG_I("test end dead loop\n");

	//while(1){
//
	//}


	test_time = timestamp_get_us() - test_time;

	att_buf_printf("rftest report id:%d result:%d \ncfo:%d khz index %d \nrssi %d ber %d pwr %d\ntest time:%dms", \
		g_cfo_return_arg.test_id, g_cfo_return_arg.test_result, g_cfo_return_arg.cfo_val, \
		g_cfo_return_arg.cap_index, g_cfo_return_arg.rssi_val, g_cfo_return_arg.ber_val, \
		g_cfo_return_arg.pwr_val, test_time/1000);

	g_return_info.info_buf = (uint8_t *)&return_str_buffer;
	g_return_info.data_length = str_index;

	LOG_I("return strlen %d\n", str_index);

	uart_init(0, 1, 1000000);

	sys_write32(baud_backup, UART0_BR);
	sys_write32(uart_ctl_backup, UART0_CTL);

	return &g_return_info;
}
#endif
