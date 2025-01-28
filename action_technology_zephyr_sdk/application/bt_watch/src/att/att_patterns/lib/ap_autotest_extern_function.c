/*
 * Copyright (c) 2021 Actions Semi Co., Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Author: wuyufan<wuyufan@actions-semi.com>
 *
 * Change log:
 *	2021/2/17: Created by wuyufan.
 */

#include "att_pattern_test.h"

struct att_env_var *self;

//new

void printk(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	self->api->vprintk(fmt, ap);
	va_end(ap);

	return ;
}


#ifdef __UVISION_VERSION
int  $Sub$$__2printf(const char *fmt, ...)
#else
int printf(const char *fmt, ...)
#endif
{
	va_list ap;

	va_start(ap, fmt);
	self->api->vprintk(fmt, ap);
	va_end(ap);

	return 0;
}


int property_get(const char *key, char *value, int value_len)
{
	return self->api->property_get(key, value, value_len);
}

int property_set(const char *key, char *value, int value_len)
{
	return self->api->property_set(key, value, value_len);
}

int property_set_factory(const char *key, char *value, int value_len)
{
	return self->api->property_set_factory(key, value, value_len);
}

int stub_write_packet(uint16_t opcode, uint32_t op_para, uint8_t *data_buffer, uint32_t data_len)
{
    return self->api->stub_write_packet(opcode, op_para, data_buffer, data_len);
}

int stub_read_packet(uint16_t opcode, uint32_t op_para, uint8_t *data_buffer, uint32_t data_len)
{
    return self->api->stub_read_packet(opcode, op_para, data_buffer, data_len);
}

int stub_status_inquiry(void)
{
    return self->api->stub_status_inquiry();
}

int read_atf_sub_file(u8_t *dst_addr, u32_t dst_buffer_len, const u8_t *sub_file_name, s32_t offset, s32_t read_len, atf_dir_t *sub_atf_dir)
{
	return self->api->read_atf_sub_file(dst_addr, dst_buffer_len, sub_file_name, offset, read_len, sub_atf_dir);
}

void *malloc(size_t size)
{
	return self->api->malloc(size);
}

void free(void *ptr)
{
	self->api->free(ptr);
}

void k_sleep(s32_t duration)
{
	self->api->k_sleep(duration);
}

u32_t k_uptime_get(void)
{
	return self->api->k_uptime_get();
}

void udelay(u32_t delay_val)
{
	self->api->udelay(delay_val);
}

void mdelay(unsigned int msec)
{
    msec <<= 1;
	while (msec-- != 0){
		udelay(500);
    }
}