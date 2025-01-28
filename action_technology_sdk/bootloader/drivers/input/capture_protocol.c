/*
 * Copyright (c) 2017 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief TP Keyboard driver for Actions SoC
 */
#define ir_protocol_param_init
#define capture_param_init

#include <shell/shell.h>
#include <drivers/input/input_dev.h>
#include <ir_data_hal.h>
#include <ir_protocol_hal.h>
#include <logging/log.h>
#include "IR_context.h"

struct capture_handle cp_handle;

static int capture_acts_rate_check(u32_t code_rate, u32_t carrier_rate)
{
	if((code_rate - carrier_rate) * (code_rate - carrier_rate) > 4)
		return -1;

	return 0;
}

static int capture_acts_lc_length(u32_t code)
{
	u32_t lc_num = 0;
	u32_t temp_val = 0;

	while(code) {
		temp_val = code & 1;
		code = code >> 1;
		if((code & 1) != temp_val)
			lc_num++;
	}

	return lc_num;
}

static int capture_acts_halfcode_compare(uint32_t code, uint32_t cp, u32_t threshold)
{
	int temp_val = 0;
	u32_t threshold_val = 0;

	temp_val = (code & 0x7fff) - (cp & 0x7fff);

	threshold_val = (cp&0x7fff)*threshold/100;

	if(temp_val * temp_val > threshold_val * threshold_val)
		return -EIO;

	return 0;
}

static int capture_acts_lead_check(u32_t *data, u32_t bit_length, u32_t code, u32_t threshold)
{
	u32_t lc_num, temp_val, lc_length;
	u32_t irpro_buf[32];

	memset(irpro_buf, 0, 32);
	lc_num = 0;

	while(code) {
		irpro_buf[lc_num] = irpro_buf[lc_num] + 1;
		temp_val = code & 1;
		code = code >> 1;

		if((code & 1) != temp_val)
			lc_num++;
	}

	for(int j = 0; j < lc_num; j++) {
		lc_length = irpro_buf[lc_num - 1 - j] * bit_length;
		temp_val = data[j] & 0x7fff;

		if(j == (lc_num - 1)) {
			if(temp_val > (lc_length + threshold*bit_length/100))
				return temp_val -lc_length - threshold*bit_length/200;
		}

		if((temp_val - lc_length) * (temp_val - lc_length) > (threshold*bit_length/100)*(threshold*bit_length/100))
			return -2;
	}

	return 0;
}

static int capture_acts_code_compare(uint32_t code_pre, uint32_t code_post, uint32_t cp_pre, uint32_t cp_post, u32_t threshold)
{
	int temp_val = 0;
	u32_t threshold_val = 0;

	if((code_pre & 0x8000) != (cp_pre & 0x8000))
		return -EIO;

	if((code_post & 0x8000) != (cp_post & 0x8000))
		return -EIO;

	temp_val = (code_pre & 0x7fff) - (cp_pre & 0x7fff);

	threshold_val = (cp_pre&0x7fff)*threshold/100;

	if(temp_val * temp_val > threshold_val * threshold_val)
		return -EIO;

	threshold_val = (cp_post&0x7fff)*threshold/100;

	if(code_post < (cp_post + threshold_val))//belong to a complete message
		return 0;

	temp_val = (code_post & 0x7fff) - (cp_post & 0x7fff) - (cp_post & 0x7fff)*threshold/200;//post including a next message

	return temp_val;
}

static int capture_acts_cv(u32_t *data, u32_t offset, ir_receive_param_t *param, u32_t threshold, u32_t leader_hc, u32_t *decode)
{
	int data_pre, data_post;
	uint32_t code_off;
	int temp_val, temp_data;
	uint32_t code0_pre, code0_post, code1_pre, code1_post;
	uint32_t tr0_pre, tr0_post, tr1_pre, tr1_post;
	int cp_result[4];
	bool code1_discriminate = false;

	code0_pre = (param->ir_0_code & 0x8000) | ((param->ir_0_code & 0x7f00) >> 8) * param->code_bit_length;
	code0_post = (param->ir_0_code & 0x80) << 8 | (param->ir_0_code & 0x7f) * param->code_bit_length;
	code1_pre = (param->ir_1_code & 0x8000) | ((param->ir_1_code & 0x7f00) >> 8) * param->code_bit_length;
	code1_post = (param->ir_1_code & 0x80) << 8 | (param->ir_1_code & 0x7f) * param->code_bit_length;

	if(param->ir_trc_loc) {
		tr0_pre = (param->ir_tr0_code & 0x8000) | ((param->ir_tr0_code & 0x7f00) >> 8) * param->code_bit_length;
		tr0_post = (param->ir_tr0_code & 0x80) << 8 | (param->ir_tr0_code & 0x7f) * param->code_bit_length;
		tr1_pre = (param->ir_tr1_code & 0x8000) | ((param->ir_tr1_code & 0x7f00) >> 8) * param->code_bit_length;
		tr1_post = (param->ir_tr1_code & 0x80) << 8 | (param->ir_tr1_code & 0x7f) * param->code_bit_length;
	}

	code_off = decode[0] = decode[1] = 0;

	if(leader_hc) {
		offset = offset - 1;
		data_pre = (data[offset] & 0x8000) | leader_hc;
	} else
		data_pre = data[offset];// a complete message including data_pre | data_ post

	data_post = data[offset + 1];

	for(int j = offset; j < 99; j++) {

		code_off++;

		if(data[j] == 0)
			break;

		if(((data[j] & 0x8000) == 0) && ((data[j] & 0x7fff) > param->ir_max_code))
			break;

		if(code_off > param->ir_dc_length + 1)
			return -3;

		if(code_off > param->ir_dc_length)
			continue;

		if(j > offset)
			data_post = data[j + 1];

		temp_val = capture_acts_code_compare(data_pre, data_post, code0_pre, code0_post, threshold);

		if(temp_val > 0) {
			cp_result[0] = capture_acts_halfcode_compare((data_post & 0x8000) | temp_val, code0_pre, threshold);
			cp_result[1] = capture_acts_halfcode_compare((data_post & 0x8000) | temp_val, code1_pre, threshold);
			cp_result[2] = capture_acts_halfcode_compare((data_post & 0x8000) | temp_val, tr0_pre, threshold);
			cp_result[3] = capture_acts_halfcode_compare((data_post & 0x8000) | temp_val, tr1_pre, threshold);

			if(cp_result[3]&&cp_result[2]&&cp_result[1]&&cp_result[0])
				temp_val = -5;
		}

		if(temp_val != 0) {
			temp_data = capture_acts_code_compare(data_pre, data_post, code1_pre, code1_post, threshold);
			if(temp_data > 0) {
				cp_result[0] = capture_acts_halfcode_compare((data_post & 0x8000) | temp_data, code0_pre, threshold);
				cp_result[1] = capture_acts_halfcode_compare((data_post & 0x8000) | temp_data, code1_pre, threshold);
				cp_result[2] = capture_acts_halfcode_compare((data_post & 0x8000) | temp_data, tr0_pre, threshold);
				cp_result[3] = capture_acts_halfcode_compare((data_post & 0x8000) | temp_data, tr1_pre, threshold);
			
				if(cp_result[3]&&cp_result[2]&&cp_result[1]&&cp_result[0])
					temp_data = -5;
			}
			if(temp_data >= 0) {
				temp_val = temp_data;
				code1_discriminate = true;
			}
		}

		if(temp_val >= 0) {
			if(code1_discriminate) {
				if(param->ir_dc_length - code_off > 32)
					decode[1] |= 1 << (param->ir_dc_length - code_off - 32);
				else
					decode[0] |= 1 << (param->ir_dc_length - code_off);
			}
			code1_discriminate = false;
		}

		if(temp_val > 0) {//compare status and including next message
			data_pre = (data_post & 0x8000) | temp_val;
			continue;
		} else if(temp_val == 0) {//campare status
			j++;
			data_pre = data[j + 1];
			continue;
		}

		if(param->ir_trc_loc) {
			temp_val = capture_acts_code_compare(data_pre, data_post, tr0_pre, tr0_post, threshold);

			if(temp_val > 0) {
				cp_result[0] = capture_acts_halfcode_compare((data_post & 0x8000) | temp_val, code0_pre, threshold);
				cp_result[1] = capture_acts_halfcode_compare((data_post & 0x8000) | temp_val, code1_pre, threshold);
				cp_result[2] = capture_acts_halfcode_compare((data_post & 0x8000) | temp_val, tr0_pre, threshold);
				cp_result[3] = capture_acts_halfcode_compare((data_post & 0x8000) | temp_val, tr1_pre, threshold);

				if(cp_result[3]&&cp_result[2]&&cp_result[1]&&cp_result[0])
					temp_val = -5;
			}

			if(temp_val != 0) {
				temp_data = capture_acts_code_compare(data_pre, data_post, tr1_pre, tr1_post, threshold);

				if(temp_data > 0) {
					cp_result[0] = capture_acts_halfcode_compare((data_post & 0x8000) | temp_data, code0_pre, threshold);
					cp_result[1] = capture_acts_halfcode_compare((data_post & 0x8000) | temp_data, code1_pre, threshold);
					cp_result[2] = capture_acts_halfcode_compare((data_post & 0x8000) | temp_data, tr0_pre, threshold);
					cp_result[3] = capture_acts_halfcode_compare((data_post & 0x8000) | temp_data, tr1_pre, threshold);
				
					if(cp_result[3]&&cp_result[2]&&cp_result[1]&&cp_result[0])
						temp_data = -5;
				}

				if(temp_data >= 0) {
					temp_val = temp_data;
					code1_discriminate = true;
				}
			}

			if(temp_val >= 0) {
				if(code1_discriminate) {
					if(param->ir_dc_length - code_off > 32)
						decode[1] |= 1 << (param->ir_dc_length - code_off - 32);
					else
						decode[0] |= 1 << (param->ir_dc_length - code_off);
				}
				code1_discriminate = false;
			}

			if(temp_val > 0) {//compare status and including next message
				data_pre = (data_post & 0x8000) | temp_val;
				continue;
			} else if(temp_val == 0) {//campare status
				j++;
				data_pre = data[j + 1];
				continue;
			}
		}

		return -3;
	}
	if(decode[1])
		printk("decode:0x%x%x\n", decode[1], decode[0]);
	else
		printk("decode:0x%x\n", decode[0]);

	return 0;
}

static int capture_acts_Tf(u32_t *data, u32_t Tf, u32_t lc_bit_length, u32_t threshold)
{
	int data_val, temp_val;
	int j = 0;

	temp_val = 0;

	for(j = 0; j < 100; j++) {
		data_val = data[j] & 0x7fff;
		temp_val += data_val/10;

		if(data_val == 0)
			break;

		if(((data[j] & 0x8000) == 0) && ((data[j] & 0x7fff) > lc_bit_length))
			break;
	}

	for(; j < 100; j++) {
		data_val = data[j] & 0x8000;
		if(data_val != 0) {
			break;
		}
		data_val = data[j] & 0x7fff;
		if(data_val == 0)
			temp_val += 3277/3;
		else
			temp_val += data_val/10;
	}

	if(temp_val < Tf - Tf*threshold/100)
		return -4;

	return 0;
}

static u32_t capture_acts_ic(u32_t *decode, ir_receive_param_t *param)
{
	u32_t data[2];

	data[0] = ((decode[0] >> param->ir_ic_co) | (decode[1] << (32 - param->ir_ic_co)))&param->ir_ic_mask;
	data[1] = (decode[0] >> param->ir_ic_ico)&param->ir_ic_mask;

	if(data[0] & data[1])
		return -5;

	return 0;
}

static int capture_acts_discriminate(struct capture_data *data, u32_t *decode)
{

	u32_t irpro_param1, irpro_param2, irpro_param3;
	int i, result;
	u32_t leader_hc;
	
	result = 0;

	for(i = 0; i < Protocols_MAX; i++) {

		irpro_param3 = ir_protocol_param[i].ir_threshold_val;
		/* rate check */
		if(ir_protocol_param[i].ir_cr_check_en) {
			if(capture_acts_rate_check(ir_protocol_param[i].ir_cr_rate ,
									data->carrier_rate)) {
				result = -1;
				continue;
			}
		}

		/* lead check */
		if(ir_protocol_param[i].ir_lc_check_en) {
			irpro_param1 = ir_protocol_param[i].ir_lc_bit_length;
			irpro_param2 = ir_protocol_param[i].ir_lc_code;
			leader_hc = capture_acts_lead_check(data->capture_data, irpro_param1,
									irpro_param2, irpro_param3);
			if(leader_hc < 0) {
				result = -2;
				continue;
			}
		}

		/* code check */
		if(ir_protocol_param[i].ir_cv_check_en) {

			irpro_param1 = capture_acts_lc_length(ir_protocol_param[i].ir_lc_code);
			result = capture_acts_cv(data->capture_data, irpro_param1,
							&ir_protocol_param[i], irpro_param3, leader_hc, decode);

			if(result == -3) {
				continue;
			}
		}

		/* Tf check */
		if(ir_protocol_param[i].ir_Tf_check_en) {
			irpro_param1 = ir_protocol_param[i].ir_Tf_length;
			if(capture_acts_Tf(data->capture_data, irpro_param1,
							ir_protocol_param[i].ir_max_code, irpro_param3)) {
				result = -4;
				continue;
			}
		}

		/* ir inverse code check */
		if(ir_protocol_param[i].ir_ic_check_en) {
			irpro_param1 = capture_acts_lc_length(ir_protocol_param[i].ir_lc_code);
			if(capture_acts_ic(decode, &ir_protocol_param[i])) {
				result = -5;
				continue;
			}
		}

		break;
	}

	if(i < Protocols_MAX)
		return i;
	else
		return result;

}

static int capture_acts_protocol(int ret)
{
	u32_t protocol;

	switch(ret) {
	case PWM_IR_6122:
		protocol = IR_uPD6121;
		printk("IR protocol is nec\n");
		break;

	case PWM_IR_9012:
		protocol = IR_9012;
		printk("IR protocol is 9012\n");
		break;

	case PWM_IR_RC6:
		protocol = IR_RC6_dat1;
		printk("IR protocol is RC6\n");
		break;

	case PWM_IR_50462:
		protocol = IR_50462;
		printk("IR protocol is 50462\n");
		break;

	case PWM_IR_M50560:
		protocol = IR_M50560_dat1;
		printk("IR protocol is 50560\n");
		break;

	case PWM_IR_RC5X:
		protocol = IR_RC5;
		printk("IR protocol is rc5x\n");
		break;

	case PWM_IR_7461:
		protocol = IR_7461;
		printk("IR protocol is 7461\n");
		break;

	case PWM_IR_3004:
		protocol = IR_3004;
		printk("IR protocol is 3004\n");
		break;

	case PWM_IR_RCA:
		protocol = IR_RCA;
		printk("IR protocol is RCA\n");
		break;

	default:
		printk("can not find right protocol: %d\n", ret);
		return -ENOTSUP;
	}

	return protocol;
}

static void capture_data_dump(struct capture_data *data)
{
	printk("rate:%dkhz\n", data->carrier_rate);

	for(int i = 0;i < 100;i++) {
		data->capture_data[i] = (data->capture_data[i] & 0x8000) | ((data->capture_data[i] & 0x7fff)/3);//cycle to us
		if(i%10 == 9) {
			printk("%d0%d\n", ((data->capture_data[i] & 0x8000) >> 15), (data->capture_data[i] & 0x7fff));
			continue;
		}
		printk("%d0%d ", ((data->capture_data[i] & 0x8000) >> 15), (data->capture_data[i] & 0x7fff));
	}
}

void capture_notify_callback(struct device *dev, struct input_value *val)
{
	struct capture_handle * handle = &cp_handle;
	struct input_value report_val;
	uint32_t decode_data[2] = {0x11,0x22};
	struct capture_data data;
	int result;

	data.capture_data = val->ir.protocol.data;
	data.carrier_rate = val->ir.protocol.carry_rate;

	capture_data_dump(&data);

	result = capture_acts_discriminate(&data, decode_data);
	result = capture_acts_protocol(result);

	report_val.ir.protocol.mode = result;
	report_val.ir.protocol.data = decode_data;

	handle->capture_notify(NULL, &report_val);
}

struct capture_handle * capture_device_open(capture_notify_cb cb, char * dev_name)
{
	struct capture_handle * handle = &cp_handle;
	if(handle == NULL)
	{
		printk("capture device in mem_malloc failed  need %d  bytes ",(int)sizeof(struct capture_handle));
		return NULL;
	}

	handle->input_dev = (struct device *)device_get_binding(dev_name);

	if (!handle->input_dev) {
		printk("cannot found capture dev %s\n",dev_name);
		return NULL;
	}

	input_dev_enable(handle->input_dev);

	input_dev_register_notify(handle->input_dev, capture_notify_callback);

	handle->capture_notify = cb;

	return handle;
}

void capture_device_close(void * handle)
{
	struct capture_handle * capture = &cp_handle;

	input_dev_unregister_notify(capture->input_dev, capture->capture_notify);

	input_dev_disable(capture->input_dev);
}


void capture_device_abort_cb(void * handle)
{
	struct capture_handle * capture = &cp_handle;

	input_dev_unregister_notify(capture->input_dev, capture->capture_notify);
}

void capture_device_enable_cb(void * handle)
{
	struct capture_handle * capture = &cp_handle;

	input_dev_register_notify(capture->input_dev, capture->capture_notify);
}

