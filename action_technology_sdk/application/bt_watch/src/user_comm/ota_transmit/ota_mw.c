/*!
 * \file	  ota_mw.c
 * \brief	  ota middleware layer
 * \details   
 * \author	  Lion Li
 * \date			04/17/2020
 * \copyright Actions
 */

#include <mem_manager.h>
#include <app_manager.h>
#include <srv_manager.h>
#include <volume_manager.h>
#include <msg_manager.h>
#include <thread_timer.h>
#include <media_player.h>
#include <audio_system.h>
#include <audio_policy.h>
#include <zephyr.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stream.h>
#include "app_defines.h"
#include "sys_manager.h"
#include "app_ui.h"

#include "tts_manager.h"
#include "buffer_stream.h"
#include "ringbuff_stream.h"
#include "media_mem.h"
	
#ifdef CONFIG_PROPERTY
#include "property_manager.h"
#endif
	
#ifdef CONFIG_TOOL
#include "tool_app.h"
#endif
#include "ota_mw.h"
#include "ap_ota_private.h"

#define OTA_HEAD_XML_SPLICE_LEN (1024+1536)

typedef struct
{
	struct ota_backend *backend;
} ota_rs_handle_t;

extern void ota_app_backend_callback(struct ota_backend *backend, int cmd, int state);

u8_t ota_resource_init(void **phdl, u32_t fw_size, u32_t fw_ver, void *param)
{
	ota_rs_handle_t *tp;
		
	if (!phdl)
	{
		SYS_LOG_ERR("phdl 0x%p.",phdl);
		return OTA_MW_PARAM_INVALID;
	}

	if ((0xffffffff != fw_ver) && (ota_transmit_version_get() >= fw_ver))
	{
		SYS_LOG_ERR("new ver 0x%x, old ver 0x%x.", fw_ver, ota_transmit_version_get());
		return OTA_MW_VER_ERROR;
	}

	tp = (*(ota_rs_handle_t **)phdl);
	if (tp)
	{
		// log_debug("breakpoint continue tp 0x%p.",tp);
		return OTA_MW_CONTINUE;
	}

	tp = (ota_rs_handle_t *)app_mem_malloc(sizeof(ota_rs_handle_t));
	if (!tp)
	{
		//log_error("ota middleware ota_rs_handle_t malloc fail.");
		return OTA_MW_MEMORY_ERROR;
	}
	memset(tp, 0, sizeof(ota_rs_handle_t));
	*phdl = tp;

	tp->backend = ota_backend_apota_init(ota_app_backend_callback, NULL);

	if (!tp->backend) {
		app_mem_free(tp);
		SYS_LOG_INF("failed to init ap ota");
		return OTA_MW_DEVICE_INFO_ERROR;
	}

	//if (tp->backend) {
		//ota_backend_apota_start(tp->backend);
	//	apota_stream_connected();
	//}

	return OTA_MW_SUCCESS;
}

u8_t ota_resource_start(void *hdl, u32_t offset, void *ptr)
{
	ota_rs_handle_t *tp;
	int cache_size= 0;
	u16_t len = 0;
	u8_t *buf = NULL;
	ota_rs_breakponit_t *breakpoint = ptr;

	tp = (ota_rs_handle_t *)hdl;
	if (!tp)
	{
		//log_error("hdl 0x%p.",tp);
		return OTA_MW_PARAM_INVALID;
	}

	if (tp->backend) 
	{
		if (offset)
		{
			buf = app_mem_malloc(OTA_HEAD_XML_SPLICE_LEN);
			memset(buf, 0, OTA_HEAD_XML_SPLICE_LEN);
			len = ota_ap_head_xml_get(breakpoint, buf);
		}

		apota_stream_connected(len, offset);

		if (len)
		{
			os_sleep(100);
			cache_size = ota_backend_apota_write_stream(tp->backend, buf, len);
			SYS_LOG_INF("cache_size %d\n", cache_size);		
		}
	}
	
	if (buf)
		app_mem_free(buf);

	return OTA_MW_SUCCESS;
}

u8_t ota_resource_deinit(void **hdl, void *param)
{
	ota_rs_handle_t *tp;
	
	if (!hdl)
	{
		// log_error("hdl 0x%p.",hdl);
		return OTA_MW_PARAM_INVALID;
	}

	tp = (*(ota_rs_handle_t **)hdl);
	if (!tp)
	{
		// log_error("tp 0x%p.",tp);
		return OTA_MW_PARAM_INVALID;
	}

	if (tp->backend) {
		//ota_backend_apota_stop(tp->backend);
		apota_stream_disconnected();
	}

	app_mem_free(tp);
	*hdl = NULL;

	return OTA_MW_SUCCESS;
}

u8_t ota_fw_check(void *hdl)
{
	ota_rs_handle_t *tp;

	tp = (ota_rs_handle_t *)hdl;
	if (!tp)
	{
		//log_error("hdl 0x%p.",tp);
		return OTA_MW_PARAM_INVALID;
	}

	/* need reply ota success or not */
	if (ota_backend_apota_reply(tp->backend))
		return OTA_MW_SUCCESS;
	else
		return OTA_MW_VER_ERROR;
}

u8_t ota_data_process(void *hdl, u8_t* payload, int payload_len)
{
	ota_rs_handle_t *tp;
	int cache_size= 0;

	tp = (ota_rs_handle_t *)hdl;
	if (!tp)
	{
		SYS_LOG_WRN("hdl 0x%p.",tp);
		return OTA_MW_PARAM_INVALID;
	}

	if (tp->backend) {
		cache_size = ota_backend_apota_write_stream(tp->backend, payload, payload_len);
		SYS_LOG_INF("cache_size %d\n", cache_size);
	}
	
	//if (real_len != payload_len)
	//{
	//	SYS_LOG_INF("payload_len %d real_len %d\n", payload_len, real_len);
	//	return OTA_MW_PARSE_ERROR;
	//}

	return OTA_MW_SUCCESS;
}
