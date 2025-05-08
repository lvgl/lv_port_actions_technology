#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>
//#include "twi_master.h"
#include "hx3605.h"
//#include "SEGGER_RTT.h"

hx3605_wear_msg_code_t hx3605_wear_status = MSG_NO_WEAR;
hx3605_wear_msg_code_t hx3605_wear_status_pre = MSG_NO_WEAR;
extern const int32_t check_mode_unwear_thre;
extern const int32_t check_mode_wear_thre;
uint8_t notouch_cnt = 0;
uint8_t touch_cnt = 0;
extern uint8_t read_fifo_first_flg;

#ifdef CHECK_TOUCH_LIB
void hx3605_check_touch_init(void) //20200615 ericy ppg fs=25hz, phase3 conversion ready interupt en
{
	uint16_t sample_rate = 10; /*config the data rate of chip alps2_fm ,uint is Hz*/
	uint32_t prf_clk_num = 32000 / sample_rate; /*period in clk num, num = Fclk/fs */
	uint8_t ps0_enable = 1; /*ps0_enable  , 1 mean enable ; 0 mean disable */
	uint8_t ps1_enable = 0; /*ps1_enable  , 1 mean enable ; 0 mean disable */
	uint8_t ps0_osr = 1; /* 0 = 128 ; 1 = 256 ; 2 = 512 ; 3 = 1024 */
	uint8_t ps1_osr = 0; /* 0 = 128 ; 1 = 256 ; 2 = 512 ; 3 = 1024 */

	uint8_t ps1_cp_avg_num_sel = 0;
	uint8_t ps0_cp_avg_num_sel = 0;
	uint8_t ps1_avg_num_sel_i2c = 0;
	uint8_t ps0_avg_num_sel_i2c = 0;
	/***********led open enable***********/
	uint8_t dccancel_ps0_data1 = 0; //offset idac   BIT[9:0]  0 31.25nA, 1023 32uA, step 31.25nA
	uint8_t dccancel_ps0_data2 = 0; //offset idac   BIT[9:0]  0 31.25nA, 1023 32uA, step 31.25nA
	uint8_t dccancel_ps1_data1 = 0; //offset idac   BIT[9:0]  0 31.25nA, 1023 32uA, step 31.25nA
	uint8_t dccancel_ps1_data2 = 0; //offset idac   BIT[9:0]  0 31.25nA, 1023 32uA, step 31.25nA

	uint8_t ir_pden_ps0 = 0; //IR_PDEN_PS0
	uint8_t pddrive_ps0 = 8; //PDDRIVE_PS0  0-63

	uint8_t ir_pden_ps1 = 0; //IR_PDEN_PS1
	uint8_t pddrive_ps1 = 0; //PDDRIVE_PS1  0-63
	uint8_t ps1_interval_i2c = 0; // config 0/5

	uint8_t led_en_begin = 1; // 0 = 2 ; 1 = 4 ; 2 = 8 ; 3 = 16 ;
	uint8_t afe_reset = 1; //* 0 = 32clk ; 1 = 64clk ; 2 = 128clk ; 3 = 256clk(d) ;
	uint8_t en2rst_delay = 2;
	uint8_t init_wait_delay = 2; /* 0 = 31clk ; 1 = 64clk ; 2 = 127clk ; 3 = 255clk(d) ;
                                     4 = 511clk; 5 = 1023clk; 6 = 2047; 7 = 2048clk */
	uint8_t thres_int = 1; //thres int enable
	uint8_t data_rdy_int = 8; //[3]:ps1_data2 [2]:ps1_data1 [1]:ps0_data2 [0]:ps0_data1

	uint8_t ldrsel_ps1 = 0; //ps1 LDR SELECT 01:ldr0-IR 02:ldr1-RLED  04:ldr2-GLED
	uint8_t ldrsel_ps0 = 2; //ps0 LDR SELECT 01:ldr0-IR 02:ldr1-RLED  04:ldr2-GLED
	/***********cap *********/
	uint8_t intcapsel_ps1 =
		1; //01=4fp 02=8pf 03=12pf 04=16pf 05=20pf 06=24pf 07=28pf 08=32pf 09=36pf 10=40pf 11=44pf 12=48pf 13=52pf 14=56pf 15=60pf
	uint8_t intcapsel_ps0 =
		1; //01=4fp 02=8pf 03=12pf 04=16pf 05=20pf 06=24pf 07=28pf 08=32pf 09=36pf 10=40pf 11=44pf 12=48pf 13=52pf 14=56pf 15=60pf

	uint8_t led_on_time =
		5; /* 0 = 32clk2=8us ; 8 = 64clk=16us; 16=128clk=32us ; 32 = 256clk=64us ;
                                     64 = 512clk=128us ; 128= 1024clk=256us; 256= 2048clk=512us*/
	uint8_t force_adc_clk_sel = 0;
	uint8_t force_adc_clk_cfg = 0;
	uint8_t force_PEN = 0; //phase enable
	uint8_t force_PEN_cfg = 0;
	uint8_t force_LED_EN = 0;
	uint8_t force_LED_EN_cfg = 0;
	uint8_t force_CKAFEINT_sel = 0;
	uint8_t force_CKAFEINT_cfg = 0;

	uint8_t PDBIASEN = 0;
	uint8_t PDILOADEN = 0;

	hx3605_write_reg(0x01, 0x01);
	hx3605_delay(5);
	hx3605_write_reg(0x01, 0x00);
	hx3605_delay(5);

	hx3605_write_reg(0X10, (ps1_enable << 5 | ps0_enable << 4 | ps1_osr << 2 |
				ps0_osr)); //default 0x00
	hx3605_write_reg(0X11, (uint8_t)prf_clk_num); // prf bit<7:0>6   default 0x00
	hx3605_write_reg(0X12, (uint8_t)(prf_clk_num >> 8)); // prf bit<15:8>  default 0x03
	hx3605_write_reg(0X13, (ps1_interval_i2c)); //default 0x00
	hx3605_write_reg(0X14, (led_en_begin << 6 | afe_reset << 4 | en2rst_delay << 2 |
				init_wait_delay)); //default 0x00
	hx3605_write_reg(0X15, led_on_time); // led_en_num*8     //default 0x00
	hx3605_write_reg(0X16, (ps1_cp_avg_num_sel << 4) | (ps0_cp_avg_num_sel)); //default 0x00
	hx3605_write_reg(0X17, (ps1_avg_num_sel_i2c << 2) | (ps0_avg_num_sel_i2c)); //default 0x00

	hx3605_write_reg(0X18, (uint8_t)dccancel_ps0_data1); //default 0x00
	hx3605_write_reg(0X19, (uint8_t)(dccancel_ps0_data1 >> 8)); //default 0x00
	hx3605_write_reg(0X1a, (uint8_t)dccancel_ps0_data2); //default 0x00
	hx3605_write_reg(0X1b, (uint8_t)(dccancel_ps0_data2 >> 8)); //default 0x00
	hx3605_write_reg(0X1c, (uint8_t)dccancel_ps1_data1); //default 0x00
	hx3605_write_reg(0X1d, (uint8_t)(dccancel_ps1_data1 >> 8)); //default 0x00
	hx3605_write_reg(0X1e, (uint8_t)dccancel_ps1_data2); //default 0x00
	hx3605_write_reg(0X1f, (uint8_t)(dccancel_ps1_data2 >> 8)); //default 0x00

	hx3605_write_reg(0X20, (ir_pden_ps0 << 7 | ir_pden_ps0 << 6 | pddrive_ps0)); //default 0x00
	hx3605_write_reg(0X21, (ir_pden_ps1 << 7 | ir_pden_ps1 << 6 | pddrive_ps1)); //default 0x00
	hx3605_write_reg(0X22, (ldrsel_ps1 << 4 | ldrsel_ps0)); //default 0x00
	hx3605_write_reg(0X23, (intcapsel_ps1 << 4 | intcapsel_ps0)); //default 0x00
	hx3605_write_reg(0X26, (thres_int << 4 | data_rdy_int)); //default 0x0f

	//    hx3605_write_reg(0X34, 0X10);  //w_almost_full ENABLE   /default0x04

	hx3605_write_reg(0X69, (force_adc_clk_sel << 7 | force_adc_clk_cfg << 6 | force_PEN << 5 |
				force_PEN_cfg << 4 | force_LED_EN << 3 | force_LED_EN_cfg << 2 |
				force_CKAFEINT_sel << 1 | force_CKAFEINT_cfg)); //default 0x00
	hx3605_write_reg(0Xc2, 0x0b);
	hx3605_write_reg(0Xc3, (PDBIASEN << 5 | PDILOADEN << 4)); //default 0x27

#if defined(TIMMER_MODE)
	hx3605_write_reg(
		0x2d,
		0x84); //bits<3:0> fifo data sel, 0000 = p1;0001= p2;0010=p3;0011=p4;bits<7> fifo enble
	hx3605_write_reg(0x2e, 0x20); //watermark
	hx3605_write_reg(0x2c, 0x1f); // int_width_i2c
	hx3605_write_reg(0X27, 0x00); // int sel,01=prf int,04=enable almost full int
#else ///////////INT Mode
	hx3605_write_reg(
		0x2d,
		0x00); //bits<3:0> fifo data sel, 0000 = p1;0001= p2;0010=p3;0011=p4;bits<7> fifo enble
	hx3605_write_reg(0x2c, 0x1f); // int_width_i2c
	hx3605_write_reg(0X27, 0x00); // int sel,01=prf ,04=enable almost full
#endif

	hx3605_write_reg(0X68, 0X01); //soft reset
	hx3605_delay(5);
	hx3605_write_reg(0X68, 0X00); //release
	hx3605_delay(5);
	read_fifo_first_flg = 1;
}

void hx3605_check_touch_open(void)
{
	notouch_cnt = 0;
	touch_cnt = 0;
	hx3605_wear_status = MSG_NO_WEAR;
}

SENSOR_ERROR_T hx3605_check_touch_enable(void)
{
	if (!hx3605_chip_check()) {
		AGC_LOG("hx3605 check id failed!\r\n");
		return SENSOR_OP_FAILED;
	}
	AGC_LOG("hx3605_check_touch_enable success!\r\n");

	hx3605_check_touch_open();
	hx3605_check_touch_init();
	hx3605_ppg_timer_cfg(true);

	return SENSOR_OK;
}

hx3605_wear_msg_code_t hx3605_check_touch(int32_t *ir_data, uint8_t data_len)
{
	uint8_t ii;
	for (ii = 0; ii < data_len; ii++) {
		if (ir_data[ii] > check_mode_wear_thre) {
			if (notouch_cnt > 0) {
				notouch_cnt--;
			}
			if (touch_cnt >= 5) {
				hx3605_wear_status = MSG_WEAR;
				touch_cnt = 5;
			} else {
				touch_cnt++;
			}
		} else if (ir_data[ii] < check_mode_unwear_thre) {
			if (touch_cnt > 0) {
				touch_cnt--;
			}
			if (notouch_cnt >= 5) {
				hx3605_wear_status = MSG_NO_WEAR;
				notouch_cnt = 5;
			} else {
				notouch_cnt++;
			}
		}
	}
	return hx3605_wear_status;
}
#endif
