#ifndef __AP_AUTOTEST_PORTING_H
#define __AP_AUTOTEST_PORTING_H

#include <string.h>
//#include <time.h>
//#include <watchdog.h>
//#include <utils.h>

typedef struct {
	uint8 connect_flag;
	uint8 err_status;
} compensation_status_t;

#if 0
struct att_env_var {
	u8_t  *rw_temp_buffer;
	u8_t  *return_data_buffer;
	u16_t test_id;
	u16_t arg_len;
	u16_t test_mode;	//see test_mode_e
	u16_t exit_mode;	//see att_exit_mode_e
};


typedef struct
{
    u8_t stub_head[6];
    u16_t test_id;
    u8_t test_result;
    u8_t timeout;
    /*Variable length array*/
    u16_t return_arg[0];
} return_result_t;
#endif

typedef struct
{
    u8_t channel[3];

    // 是否测试频偏
    u8_t cfo_test_swtich;

    // 电容值写入区域
    u8_t cap_write_region;
    u8_t reserved[3];

    s8_t cfo_low_limit;
    s8_t cfo_high_limit;
    s16_t upt_cfo_offset;

    // 最大测试次数
    u8_t max_test_count;
    // 测试项结果超时时间(ms unit)
    u32_t result_timeout;

    u8_t  ber_test;
    u32_t ber_threshold_low;
    u32_t ber_threshold_high;

    u8_t  rssi_test;
    s16_t rssi_low_limit;
    s16_t rssi_high_limit;
	
	u8_t ref_cap_index;
    u8_t cap_index_low;            //Minimum index value
    u8_t cap_index_high;           //Maximum index value    
} _bt_mp_test_arg_t;

int act_test_read_rf_rx_arg(void);

bool act_test_report_rf_rx_result(bool ret_val);

#endif
