/*
 * Copyright (c) 2020 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <os_common_api.h>
#include <shell/shell.h>

#include "key_hal.h"

void * key_device_open(key_notify_cb cb,char * dev_name)
{
	struct key_handle * handle = mem_malloc(sizeof(struct key_handle));
	if(handle == NULL)
	{
		SYS_LOG_ERR("adc key in mem_malloc failed  need %d  bytes ",(int)sizeof(struct key_handle));
		return NULL;
	}

	handle->input_dev = (struct device *)device_get_binding(dev_name);

	if (!handle->input_dev) {
		printk("cannot found key dev %s\n",dev_name);
		mem_free(handle);
		return NULL;
	}

	input_dev_enable(handle->input_dev);

	input_dev_register_notify(handle->input_dev, cb);

	handle->key_notify = cb;

	return handle;
}

void key_device_close(void * handle)
{
	struct key_handle * key = (struct key_handle *)handle;

	input_dev_unregister_notify(key->input_dev, key->key_notify);

	input_dev_disable(key->input_dev);

	mem_free(key);
}


void key_device_abort_cb(void * handle)
{
	struct key_handle * key = (struct key_handle *)handle;

	input_dev_unregister_notify(key->input_dev, key->key_notify);
}

void key_device_enable_cb(void * handle)
{
	struct key_handle * key = (struct key_handle *)handle;

	input_dev_register_notify(key->input_dev, key->key_notify);
}

int key_device_inquiry(void * handle, struct input_value *val)
{
	struct key_handle * key = (struct key_handle *)handle;

	if (val)
	{
		input_dev_inquiry(key->input_dev, val);
	}
	return 0;
}
