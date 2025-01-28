/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <sys/printk.h>
#include <device.h>
#include <drivers/input/input_dev.h>
#include <ir_data_hal.h>
#include "IR_context.h"
#include <drivers/nvram_config.h>

struct capture_data_handle dp_handle;

int capture_data_getlen(u32_t data_mask)
{
	int result = 0;

	while(data_mask > 0) {
		result++;
		data_mask = data_mask >> 1;
	}

	return result;
}

u32_t capture_data_get_lsb(u32_t data, u32_t mask, u32_t offset)
{
	u32_t result;

	result = data & (mask << offset);

	result = result >> offset;

	return result;
}

u32_t capture_data_get_msb(u32_t data, u32_t mask, u32_t offset)
{
	u32_t result;

	result = data & (mask >> (32 - offset));

	result = result << (32 - offset);

	return result;
}

void capture_data_process(struct capture_protocol *protocol, u32_t *data)
{
	u32_t data0, data1, addr0,addr1, data_offset, addr_offset;
	u32_t data_mask, data_len, data_temp;
	int i;

	data0 = capture_data_get_lsb(protocol->data[0], d_format[protocol->mode].dcode_mask, d_format[protocol->mode].dcode_offset);
	data1 = capture_data_get_msb(protocol->data[1], d_format[protocol->mode].dcode_mask, d_format[protocol->mode].dcode_offset);
	addr0 = capture_data_get_lsb(protocol->data[0], d_format[protocol->mode].ucode_mask, d_format[protocol->mode].ucode_offset);
	addr1 = capture_data_get_lsb(protocol->data[1], d_format[protocol->mode].ucode_mask, d_format[protocol->mode].ucode_offset);

	data_offset = d_format[protocol->mode].dcode_offset;
	addr_offset = d_format[protocol->mode].ucode_offset;

	*data = data1 | data0;
	data_mask = d_format[protocol->mode].dcode_mask;

	if(d_format[protocol->mode].code_endian == Little_Endian) {
		data_len = capture_data_getlen(data_mask);
		data_temp = 0;
		for(i = 0; i < data_len; i++) {
			if(*data & (1 << i))
				data_temp = data_temp | (1 << (data_len - 1 - i));
		}
		*data = data_temp;
	}
}

void capture_data_callback(struct device *dev, struct input_value *val)
{
	struct capture_data_handle *capture_data = &dp_handle;
	struct capture_protocol *protocol_data = &val->ir.protocol;
	struct input_value report_val;
	u32_t cmd_val[2];
	u32_t data_code;

	capture_data_process(protocol_data, &data_code);

	report_val.ir.data.mode = protocol_data->mode;
	report_val.ir.data.onoff = 1;
	cmd_val[0] = data_code;
	cmd_val[1] = 0;
	report_val.ir.data.cmd = cmd_val;

	if(capture_data->capture_notify != NULL)
		capture_data->capture_notify(NULL, &report_val);

}

void capture_ir_init(capture_notify_cb cb)
{
	struct capture_data_handle *capture_data = &dp_handle;

	capture_data->handle = capture_device_open(capture_data_callback,CAPTURE_DEVICE_NAME);

	if(capture_data->handle == NULL)
		printk("can not get right device\n");

	capture_data->capture_notify = cb;
}

