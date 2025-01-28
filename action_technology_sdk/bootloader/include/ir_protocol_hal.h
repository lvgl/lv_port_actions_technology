#ifndef __IR_PROTOCOL_HAL_H__
#define __IR_PROTOCOL_HAL_H__
#include <errno.h>
#include <zephyr/types.h>
#include <device.h>

#define IR_PWM_DEVICE_NAME		"PWM"
#define IR_PWM_DEVICE_CHANS		15
#define pwm_clk_rate                  (12000000)

enum protocol_map {
	PWM_IR_6122 = 0,
	PWM_IR_6122_REPEAT,
	PWM_IR_9012,
	PWM_IR_9012_REPEAT,
	PWM_IR_RC6,
	PWM_IR_50462,
	PWM_IR_M50560,
	PWM_IR_RC5X,
	PWM_IR_7461,
	PWM_IR_7461_REPEAT,
	PWM_IR_3004,
	PWM_IR_RCA,
	PWM_IR_sharp,
//	PWM_IR_261,
};

struct capture_data
{
	u32_t *capture_data;
	u32_t carrier_rate;//khz
};

typedef struct {
	volatile bool ir_lc_check_en;//leader code check enable
	volatile u32_t ir_lc_bit_length;//us
	volatile u32_t ir_lc_code;
	volatile u32_t ir_max_code;//us

	volatile bool ir_cv_check_en;//code value check enable
	volatile u32_t ir_stop_bit;
	volatile u32_t ir_dc_length;//data code length check
	volatile u32_t code_bit_length;
	volatile u32_t ir_0_code;
	volatile u32_t ir_1_code;
	volatile u32_t ir_tr0_code;
	volatile u32_t ir_tr1_code;
	volatile u32_t ir_trc_loc;

	volatile bool ir_cr_check_en;//carrier wave rate check enable
	volatile u8_t ir_cr_rate;//khz

	volatile bool ir_Tf_check_en;//ir Tf check enable(Tf:Minimum cycle length)
	volatile u32_t ir_Tf_length;//us/10(108ms -> 10800)

	volatile bool ir_ic_check_en;//ir inverse code check enable
	volatile u32_t ir_ic_mask;
	volatile u8_t ir_ic_co;//ir code offset
	volatile u8_t ir_ic_ico;//ir inverse code offset

	volatile u8_t ir_asc;//cycle times
	volatile u8_t ir_threshold_val;//The threshold value:ir_threshold_val/100

} ir_receive_param_t;

struct ir_protocol_handle
{
	struct device *input_dev;
	uint32_t chans;
};

typedef struct {
	u32_t ll;
	u16_t ld;
} pwm_ld_t;

struct ir_protocol_value {
	u32_t *data;
	u16_t mode;
	u32_t buf_num;
	pwm_ld_t *lead;
};

struct acts_pwm_ir_protocol_param {
	u32_t buf_num;
	u32_t *data;
	u16_t mode;
	pwm_ld_t *lead;
};

/* IR mode */
#define PWM_IR_REPEAT_MODE			(0 << 8)
#define PWM_IR_CYCLE_MODE			(1 << 8)

#define PWM_IR_MASK					(0xff)

typedef struct {
	volatile u32_t ir_period;
	volatile u32_t ir_duty;
	volatile u32_t ir_lc;
	volatile u32_t ir_pl0_pre;
	volatile u32_t ir_pl0_post;
	volatile u32_t ir_pl1_pre;
	volatile u32_t ir_pl1_post;
	volatile u32_t ir_ll;
	volatile u32_t ir_ld;//01b
	volatile u32_t ir_pl;
	volatile u32_t ir_stop_bit;
	volatile u32_t ir_asc;
	volatile u32_t ir_Tf;
} pwm_ir_mode_param_t;

#define Protocols_MAX		12

#ifdef ir_protocol_param_init
ir_receive_param_t ir_protocol_param[Protocols_MAX] = {
	{
	.ir_lc_check_en = true,
	.ir_lc_bit_length = 2250,
	.ir_lc_code = 60,
	.ir_max_code = 6000,

	.ir_cv_check_en = true,
	.ir_dc_length = 32,//32*2 + 1
	.ir_stop_bit = 1,
	.code_bit_length = 560,
	.ir_0_code = 0x8101,
	.ir_1_code = 0x8103,
	.ir_trc_loc = 0,

	.ir_cr_check_en = true,
	.ir_cr_rate = 38,

	.ir_Tf_check_en = true,
	.ir_Tf_length = 10800,

	.ir_ic_check_en = true,
	.ir_ic_mask = 0xff,
	.ir_ic_co = 8,
	.ir_ic_ico = 0,

	.ir_threshold_val = 10,
	.ir_asc = 1,
	},//nec

	{
	.ir_lc_check_en = true,
	.ir_lc_bit_length = 2250,
	.ir_lc_code = 30,
	.ir_max_code = 6000,

	.ir_cv_check_en = true,
	.ir_dc_length = 0,//32*2 + 1
	.ir_stop_bit = 1,
	.code_bit_length = 560,
	.ir_0_code = 0x8101,
	.ir_1_code = 0x8103,
	.ir_trc_loc = 0,

	.ir_cr_check_en = true,
	.ir_cr_rate = 38,

	.ir_Tf_check_en = true,
	.ir_Tf_length = 10800,

	.ir_ic_check_en = false,
	.ir_ic_mask = 0xff,
	.ir_ic_co = 8,
	.ir_ic_ico = 0,

	.ir_threshold_val = 10,
	.ir_asc = 1,
	},//nec_repeat

	{
	.ir_lc_check_en = true,
	.ir_lc_bit_length = 4500,
	.ir_lc_code = 2,
	.ir_max_code = 6000,

	.ir_cv_check_en = true,
	.ir_dc_length = 32,//32*2 + 1
	.ir_stop_bit = 1,
	.code_bit_length = 560,
	.ir_0_code = 0x8101,
	.ir_1_code = 0x8103,
	.ir_trc_loc = 0,

	.ir_cr_check_en = true,
	.ir_cr_rate = 38,

	.ir_Tf_check_en = true,
	.ir_Tf_length = 10800,

	.ir_ic_check_en = true,
	.ir_ic_mask = 0xff,
	.ir_ic_co = 8,
	.ir_ic_ico = 0,

	.ir_threshold_val = 10,
	.ir_asc = 1,
	},//9012

	{
	.ir_lc_check_en = true,
	.ir_lc_bit_length = 4500,
	.ir_lc_code = 2,
	.ir_max_code = 6000,

	.ir_cv_check_en = true,
	.ir_dc_length = 1,//32*2 + 1
	.ir_stop_bit = 1,
	.code_bit_length = 560,
	.ir_0_code = 0x8101,
	.ir_1_code = 0x8103,
	.ir_trc_loc = 0,

	.ir_cr_check_en = true,
	.ir_cr_rate = 38,

	.ir_Tf_check_en = true,
	.ir_Tf_length = 10800,

	.ir_ic_check_en = false,
	.ir_ic_mask = 0xff,
	.ir_ic_co = 8,
	.ir_ic_ico = 0,

	.ir_threshold_val = 10,
	.ir_asc = 1,
	},//9012_repeat

	{
	.ir_lc_check_en = true,
	.ir_lc_bit_length = 889,
	.ir_lc_code = 14,
	.ir_max_code = 6000,

	.ir_cv_check_en = true,
	.ir_dc_length = 21,
	.ir_stop_bit = 0,
	.code_bit_length = 444,
	.ir_0_code = 0x181,
	.ir_1_code = 0x8101,
	.ir_tr0_code = 0x282,
	.ir_tr1_code = 0x8202,
	.ir_trc_loc = 0x10000,

	.ir_cr_check_en = true,
	.ir_cr_rate = 36,

	.ir_Tf_check_en = true,
	.ir_Tf_length = 2600,

	.ir_ic_check_en = false,//rc6 does not have inverse code

	.ir_threshold_val = 10,
	.ir_asc = 1,
	},//rc6

	{
	.ir_lc_check_en = true,
	.ir_lc_bit_length = 4000,
	.ir_lc_code = 0,
	.ir_max_code = 6000,

	.ir_cv_check_en = true,
	.ir_dc_length = 16,
	.ir_stop_bit = 1,
	.code_bit_length = 260,
	.ir_0_code = 0x8103,
	.ir_1_code = 0x8107,
	.ir_trc_loc = 0,

	.ir_cr_check_en = true,
	.ir_cr_rate = 38,

	.ir_Tf_check_en = true,
	.ir_Tf_length = 4640,

	.ir_ic_check_en = false,//rc6 does not have inverse code

	.ir_threshold_val = 10,
	.ir_asc = 1,
	},//50642

	{
	.ir_lc_check_en = true,
	.ir_lc_bit_length = 4220,
	.ir_lc_code = 6,
	.ir_max_code = 6000,

	.ir_cv_check_en = true,
	.ir_dc_length = 17,
	.ir_stop_bit = 1,
	.code_bit_length = 528,
	.ir_0_code = 0x8101,
	.ir_1_code = 0x8103,
	.ir_tr0_code = 0x8108,
	.ir_tr1_code = 0,
	.ir_trc_loc = 0x100,

	.ir_cr_check_en = true,
	.ir_cr_rate = 38,

	.ir_Tf_check_en = true,
	.ir_Tf_length = 6340,

	.ir_ic_check_en = false,//rc6 does not have inverse code

	.ir_threshold_val = 15,
	.ir_asc = 1,
	},//50560

	{
	.ir_lc_check_en = true,
	.ir_lc_bit_length = 889,
	.ir_lc_code = 6,
	.ir_max_code = 6000,

	.ir_cv_check_en = true,
	.ir_dc_length = 12,
	.ir_stop_bit = 0,
	.code_bit_length = 889,
	.ir_0_code = 0x8101,
	.ir_1_code = 0x181,
	.ir_trc_loc = 0,

	.ir_cr_check_en = true,
	.ir_cr_rate = 36,

	.ir_Tf_check_en = true,
	.ir_Tf_length = 11400,

	.ir_ic_check_en = false,//rc6 does not have inverse code

	.ir_threshold_val = 15,
	.ir_asc = 1,
	},//rc5x

	{
	.ir_lc_check_en = true,
	.ir_lc_bit_length = 4500,
	.ir_lc_code = 6,
	.ir_max_code = 6000,

	.ir_cv_check_en = true,
	.ir_dc_length = 42,
	.ir_stop_bit = 1,
	.code_bit_length = 560,
	.ir_0_code = 0x8101,
	.ir_1_code = 0x8103,
	.ir_trc_loc = 0,

	.ir_cr_check_en = true,
	.ir_cr_rate = 38,

	.ir_Tf_check_en = true,
	.ir_Tf_length = 10800,

	.ir_ic_check_en = true,//rc6 does not have inverse code
	.ir_ic_mask = 0xff,
	.ir_ic_co = 8,
	.ir_ic_ico = 0,

	.ir_threshold_val = 10,
	.ir_asc = 1,
	},//7461

	{
	.ir_lc_check_en = true,
	.ir_lc_bit_length = 4500,
	.ir_lc_code = 6,
	.ir_max_code = 6000,

	.ir_cv_check_en = true,
	.ir_dc_length = 0,
	.ir_stop_bit = 1,
	.code_bit_length = 560,
	.ir_0_code = 0x8101,
	.ir_1_code = 0x8103,
	.ir_trc_loc = 0,

	.ir_cr_check_en = true,
	.ir_cr_rate = 38,

	.ir_Tf_check_en = true,
	.ir_Tf_length = 10800,

	.ir_ic_check_en = false,//rc6 does not have inverse code
	.ir_ic_mask = 0xff,
	.ir_ic_co = 8,
	.ir_ic_ico = 0,

	.ir_threshold_val = 10,
	.ir_asc = 1,
	},//7461_repeat

	{
	.ir_lc_check_en = true,
	.ir_lc_bit_length = 100000,
	.ir_lc_code = 0,
	.ir_max_code = 150000,

	.ir_cv_check_en = true,
	.ir_dc_length = 11,
	.ir_stop_bit = 1,
	.code_bit_length = 140,
	.ir_0_code = 0x8123,
	.ir_1_code = 0x8134,
	.ir_trc_loc = 0,

	.ir_cr_check_en = true,
	.ir_cr_rate = 38,

	.ir_Tf_check_en = true,
	.ir_Tf_length = 12100,

	.ir_ic_check_en = false,//rc6 does not have inverse code
	.ir_ic_mask = 0xff,
	.ir_ic_co = 8,
	.ir_ic_ico = 0,

	.ir_threshold_val = 20,
	.ir_asc = 1,
	},//3004

	{
	.ir_lc_check_en = true,
	.ir_lc_bit_length = 4000,
	.ir_lc_code = 2,
	.ir_max_code = 6000,

	.ir_cv_check_en = true,
	.ir_dc_length = 24,//24*2 + 1
	.ir_stop_bit = 1,
	.code_bit_length = 500,
	.ir_0_code = 0x8102,
	.ir_1_code = 0x8104,
	.ir_trc_loc = 0,

	.ir_cr_check_en = true,
	.ir_cr_rate = 38,

	.ir_Tf_check_en = true,
	.ir_Tf_length = 6400,

	.ir_ic_check_en = true,
	.ir_ic_mask = 0xff,
	.ir_ic_co = 12,
	.ir_ic_ico = 0,

	.ir_threshold_val = 10,
	.ir_asc = 1,
	},//rca

//	{
//	.ir_lc_check_en = true,
//	.ir_lc_bit_length = 4000,
//	.ir_lc_code = 2,

//	.ir_cv_check_en = true,
//	.ir_dc_length = 49,//24*2 + 1
//	.code_bit_length = 500,
//	.ir_0_code = 0x92,
//	.ir_1_code = 0x94,
//	.ir_trc_loc = 0,

//	.ir_cr_check_en = true,
//	.ir_cr_rate = 56,

//	.ir_Tf_check_en = true,
//	.ir_Tf_length = 6400,

//	.ir_ic_check_en = true,
//	.ir_ic_mask = 0xff,
//	.ir_ic_co = 12,
//	.ir_ic_ico = 0,

//	.ir_threshold_val = 10,
//	},//261

};
#else

extern ir_receive_param_t ir_protocol_param[Protocols_MAX];

#endif

void capture_device_close(void * handle);

void capture_device_abort_cb(void * handle);

void capture_device_enable_cb(void * handle);

int pwm_ir_protocol_init(char * dev_name);

int pwm_ir_protocol_transfer(struct ir_protocol_value *val);

#endif	//__IR_PROTOCOL_HAL_H__

