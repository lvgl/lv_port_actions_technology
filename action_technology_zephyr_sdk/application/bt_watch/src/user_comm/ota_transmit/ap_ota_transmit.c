/*!
 * \file	  
 * \brief	  ota middleware layer
 * \details   
 * \author	  Lion Li
 * \date		 
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
#include <app_config.h>
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
#include <user_comm/app_utility.h>
#include <user_comm/ap_ota_transmit.h>
#include <user_comm/ap_status.h>
#include <user_comm/sys_comm.h>
#include "list.h"
#include "ap_ota_private.h"
#include "ota_mw.h"

#define APOTA_SEND_MAX_TIME (5)

#define TWS_OTA_ROLE_SINGLE 0x00
#define TWS_OTA_ROLE_MASTER 0x01
#define TWS_OTA_ROLE_SLAVE 0x10

#define TWS_OTA_INFO 0x58
#define TWS_OTA_DATA 0x59

typedef struct
{
	struct list_head node;
	u16_t data_len;
	u8_t *data_buf;
	u8_t fn;
} ota_tws_data_list_t;

typedef struct
{
	struct list_head  ota_data_list;
	os_delayed_work ota_delay_work;
} ota_work_st;

typedef struct
{
	void		   *ota_hdl;
	tws_ota_sync_info_t across_info;
	tws_ota_sync_info_t cur_info;

	u32_t arrive_size;
	u32_t uniqueid;
	u8_t role;
	u8_t discard;
	u8_t exiting;
	u8_t fw_check_already;
	u8_t role_exit;
	//u8_t cur_sync_num;
	//u8_t across_sync_num;
	ota_rs_breakponit_t breakpoint;

	//os_mutex read_mutex;
	os_sem read_sem;
	//os_mutex write_mutex;

} tws_ota_rs_handle_t;

tws_ota_rs_handle_t *tws_ota_hal = NULL;
static u32_t record_unique_id = 0;
ota_work_st *work_st = NULL;

//static OS_MUTEX_DEFINE(apota_mutex);

tws_ota_rs_handle_t * ota_transmit_handle_get(void)
{
	return tws_ota_hal;
}

u32_t ota_ap_offset_calc(ota_rs_breakponit_t *breakpoint)
{
	u8_t file_id = 0;
	u8_t breakpoint_exist = 0;
	u32_t offset = 0;
	int i;
	struct ota_upgrade_info *ota_info = NULL;
	ota_info = ota_app_info_get();

	for (i = 0; i < OTA_FW_MAX_FILE;  i++) 
	{
		ota_breakpoint_dump(&breakpoint->image_bk);
		if (OTA_BP_FILE_STATE_WRITING == breakpoint->image_bk.file_state[i].state)
		{
			file_id = breakpoint->image_bk.file_state[i].file_id;
			if (file_id == breakpoint->image_bk.cur_file.file_id)
				breakpoint_exist = 1;
		}
	}
	if (!breakpoint_exist)
	{
		SYS_LOG_ERR("breakpoint NO exist.");
		return 0;
	}
	
	for (i = 0; i < 16;	i++)
	{
		//SYS_LOG_ERR("size %x %x.",ota_info->img->dir.entrys[i].length,breakpoint->image_bk.cur_file.size);
		//SYS_LOG_ERR("offset %x %x.",ota_info->img->dir.entrys[i].offset,breakpoint->image_bk.cur_file.offset);
		//SYS_LOG_ERR("checksum %x %x.",ota_info->img->dir.entrys[i].checksum,breakpoint->image_bk.cur_file.checksum);
		if ((0 == memcmp(ota_info->img->dir.entrys[i].filename, breakpoint->image_bk.cur_file.name, 12)) &&
			(ota_info->img->dir.entrys[i].length == breakpoint->image_bk.cur_file.size) &&
			/*(ota_info->img->dir.entrys[i].offset == breakpoint->image_bk.cur_file.offset) &&*/
			(ota_info->img->dir.entrys[i].checksum == breakpoint->image_bk.cur_file.checksum))
		{
			offset = ota_info->img->dir.entrys[i].offset + breakpoint->image_bk.cur_file_write_offset;
			if (breakpoint->image_bk.cur_file_write_offset > ota_info->img->dir.entrys[i].length)
			{
				SYS_LOG_ERR("compare bk fail.");
				return 0;
			}

			break;
		}
	}

	return offset;
}

void ota_ap_breakpoint_clear(void)
{
	struct ota_breakpoint bp;

	record_unique_id = 0;
	ota_breakpoint_init_default_value(&bp);
	ota_breakpoint_save(&bp);
	ota_app_init();
	//clear ota vram 
}

u8_t ota_ap_breakpoint_init(ota_rs_breakponit_t *breakpoint)
{
	u8_t ret = OTA_TWS_CONTINUE;
	int i, cksum=0;
	u8_t *pbk = (u8_t *)breakpoint;
	struct ota_upgrade_info *ota_info = NULL;
	
	ota_info = ota_app_info_get();
	if ((breakpoint->unique_id != record_unique_id) || 
		(OTA_BP_STATE_UPGRADE_WRITING != ota_info->bp.state))
	{
		ota_ap_breakpoint_clear();
		memset(breakpoint, 0, sizeof(ota_rs_breakponit_t));
		ret = OTA_TWS_SUCCESS;
	}
	else if ((OTA_FAIL == ota_info->state) ||
			(OTA_INIT == ota_info->state))
	{
		breakpoint->wflag = TWS_OTA_BREAKPOINT_MAGIC;
		ota_breakpoint_load(&breakpoint->image_bk);

		for (i = 0; i < sizeof(ota_rs_breakponit_t) - 4; i++)
		{
			cksum += pbk[i];
		}
		breakpoint->checksum = cksum;
		ret = OTA_TWS_CONTINUE;
	}
	else
	{
		ret = OTA_TWS_DEVICE_INFO_ERROR;
	}

	return ret;
}

static void ap_ota_tws_info_process(tws_ota_sync_info_t *sync)
{
	print_hex("tws_ota_info_send: ", sync, sizeof(tws_ota_sync_info_t)); 
	tws_ota_rs_handle_t *handle = ota_transmit_handle_get();

	if (handle)
		memcpy(&handle->across_info,sync,sizeof(tws_ota_sync_info_t));

	if ((handle) && (TWS_OTA_ROLE_MASTER == handle->role))
	{
		os_sem_give(&handle->read_sem);
		SYS_LOG_INF("status %d pn %d.",sync->pn,sync->status);
		return;
	}

	switch (sync->status)
	{
		case OTA_STATUS_READY:
			if (!handle)
			{
				ota_transmit_st st;
				u32_t offset = 0;
				void *resume = NULL;
				memset(&st, 0 , sizeof(ota_transmit_st));
				st.fw_size = (sync->fw_size | 0x80000000);
				st.fw_ver = sync->fw_ver;
				st.unique_id = sync->unique_id;
				st.tws_enable = 1;
				if (sync->bk_checksum)
				{
					st.checksum = sync->bk_checksum;
					resume = (void *)&offset;
				}
				ap_status_set(AP_STATUS_OTA_RUNING);
				ota_transmit_init((void *)&tws_ota_hal, &st, resume);
			}

			break;
		case OTA_STATUS_INQUIRE:
			SYS_LOG_INF("pn %d.",sync->pn);

			break;
		case OTA_STATUS_FW_CHECK:
			if (OTA_MW_SUCCESS == ota_transmit_fw_check((void *)handle))
			{
			}
			break;

		case OTA_STATUS_HALT:
			ota_transmit_deinit((void *)&tws_ota_hal);
			ap_status_set(AP_STATUS_NONE);
			break;
	}
}


static u8_t  ap_ota_data_parse(u8_t *buf, u16_t len)
{
	int  pos = 0;
	int  ctx_len = 0;
	u32_t fn;
	u8_t type;
	tws_ota_sync_info_t tinfo;
	tws_ota_rs_handle_t *handle = ota_transmit_handle_get();

	if (handle && (handle->role_exit))
		return 1;

	_GET_BUF_U8(buf, pos, type);
	_GET_BUF_U16(buf, pos, ctx_len);
	_GET_BUF_U32(buf, pos, fn);
	SYS_LOG_INF("%d_%d %d.",ctx_len,fn,__LINE__);
	u32_t i, sum = 0,real_sum = 0;

	if (2048 < ctx_len)
	{
		SYS_LOG_ERR("%d_%d.",ctx_len,fn);
		goto exit_data;
	}

	for (i = 0; i < ctx_len; i++)
	{
		sum += buf[pos + i];
	}
	
	memcpy(&real_sum,
		buf + pos + ctx_len,sizeof(u32_t)); 

	if (real_sum != sum)
	{
		SYS_LOG_ERR("real_sum %d sum %d.\n",real_sum,sum);
		goto exit_data;
	}
	
	if (TWS_OTA_INFO == type)
	{
		if (ctx_len != sizeof(tws_ota_sync_info_t))
		{
			SYS_LOG_ERR("ctx_len %d.",ctx_len);
			goto exit_data;
		}
		
		memcpy(&tinfo,
				buf + pos,sizeof(tws_ota_sync_info_t));
		ap_ota_tws_info_process(&tinfo);
	}
	else
	{
		if ((handle->cur_info.cur_fn + 1) != fn)
		{
			SYS_LOG_ERR("%d_%d.",handle->cur_info.cur_fn,fn);
			goto exit_data;
		}
		if (ota_transmit_data_process(
			(void *)handle, buf + pos, ctx_len))
		{
			goto exit_data;
		}
	}
	
	return 0;
	
exit_data:
	if (handle)
		handle->role_exit = 1;
	
	return 1;
}

static void  ap_ota_tws_data_process(os_work *work)
{
	if (!work_st)
		return;

	ota_work_st* ota_data = 
		CONTAINER_OF(work, ota_work_st, ota_delay_work);

	if (!ota_data)
		return;

	ota_tws_data_list_t*  packet = NULL;
	ota_tws_data_list_t*  next = NULL;

	list_for_each_entry_safe(packet, next, &ota_data->ota_data_list, node)
	{
		SYS_LOG_INF("packet->pn %d.",packet->fn);

		ap_ota_data_parse(packet->data_buf, packet->data_len);
			
		list_del(&packet->node);
		
		app_mem_free(packet->data_buf);
		app_mem_free(packet);

		break;
	}
	return;
}

u8_t  ap_ota_tws_access_receive(u8_t* buf, u16_t len)
{
	ota_tws_data_list_t *pkt = NULL;

	if (!work_st)
	{
		work_st = (ota_work_st *)app_mem_malloc(sizeof(ota_work_st));
		INIT_LIST_HEAD(&work_st->ota_data_list);
		os_delayed_work_init(&work_st->ota_delay_work, ap_ota_tws_data_process);
	}

	if (2048 < len)
	{
		SYS_LOG_ERR("len %d.",len);
		return 1;
	}

	pkt = (ota_tws_data_list_t *)app_mem_malloc(sizeof(ota_tws_data_list_t));
	pkt->data_len = len;
	pkt->data_buf = (u8_t *)app_mem_malloc(len);
	memcpy (pkt->data_buf, buf, len);
	list_add_tail(&pkt->node, &work_st->ota_data_list);
	os_delayed_work_submit(&work_st->ota_delay_work, 0);
	return 0;
}

static void ap_ota_tws_data_send(u8_t *data, u16_t len, u8_t type, u32_t cur_fn)
{
	int  buf_len;
	int  pos = 0;
	u8_t  *data_buf;
	u8_t retry_times = 0;
	int ret;
	//print_hex(CONST("ap_ota_tws_data_send:"), data, len);
	// OSSchedLock();
	buf_len = len + sizeof(u8_t) + sizeof(u16_t) + sizeof(u32_t) + sizeof(u32_t);
	data_buf = (u8_t *)app_mem_malloc(buf_len);

	_PUT_BUF_U8(data_buf, pos, type);
	_PUT_BUF_U16(data_buf, pos, len);
	_PUT_BUF_U32(data_buf, pos, cur_fn);
	memcpy(data_buf + pos, data, len);
	pos += len;

	u32_t i,sum = 0;
	for (i = 0; i < len; i++)
	{
		sum += data[i];
	}
	printf("cur_fn %d len %d dsum %d.\n",cur_fn,len,sum);
	_PUT_BUF_U32(data_buf, pos, sum);

	while (retry_times < APOTA_SEND_MAX_TIME) 
	{
		ret = bt_manager_tws_send_message(TWS_LONG_MSG_EVENT, TWS_EVENT_APOTA_SYNC, data_buf, buf_len);
		if (ret != buf_len) {
			SYS_LOG_ERR("123Failed to send data!");
			retry_times++;
			//os_sleep(10);
		}
		else 
		{
			break;
		}
	}

	//printf("fn %d len %d dsum %d .\n",tp->cur_info.cur_fn+1,payload_len,sum);
	//print_hex(CONST("data_arrive:"), (ctx_buf + pos), ctx_len);
	//bt_manager_tws_send_event_long(TWS_EVENT_TWS_OTA, data_buf, buf_len);

	app_mem_free(data_buf);
}

u8_t ota_transmit_init(void **phdl, ota_transmit_st *ota_st, u32_t *offset)
{
	tws_ota_rs_handle_t *tp;
	u8_t position = 0;
	u8_t sync_err = 0;
	u8_t ret_code;
	CFG_Struct_OTA_Settings cfg;

	if (!phdl)
	{
		SYS_LOG_ERR("phdl 0x%p.",phdl);
		return OTA_TWS_PARAM_INVALID;
	}
	
	tp = (*(tws_ota_rs_handle_t **)phdl);
	if (tws_ota_hal && tp)
	{
		SYS_LOG_ERR("breakpoint continue tp 0x%p.",tp);
		return OTA_TWS_CONTINUE;
	}

	app_config_read
	(
		CFG_ID_OTA_SETTINGS,
		&cfg,0,sizeof(CFG_Struct_OTA_Settings)
	);
	
	if ((0 == cfg.Enable_Single_OTA_Without_TWS) &&
		(BTSRV_TWS_MASTER > bt_manager_tws_get_dev_role()))
	{
		SYS_LOG_ERR("please tws pair.");
		return OTA_TWS_DEVICE_INFO_ERROR;
	}
	else if ((1 == cfg.Enable_Single_OTA_Without_TWS) &&
		(BTSRV_TWS_NONE == bt_manager_tws_get_dev_role()))
	{
		ota_st->tws_enable = 0;
	}

	tp = (tws_ota_rs_handle_t *)app_mem_malloc(sizeof(tws_ota_rs_handle_t));
	if (!tp)
	{
		SYS_LOG_ERR("ota middleware ota_rs_handle_t malloc fail.");
		return OTA_TWS_MEMORY_ERROR;
	}
	memset(tp, 0, sizeof(tws_ota_rs_handle_t));
	tp->role = TWS_OTA_ROLE_SINGLE;
	tp->uniqueid = ota_st->unique_id;
	
	tp->breakpoint.unique_id = ota_st->unique_id;
	ret_code = ota_ap_breakpoint_init(&tp->breakpoint);
	if (OTA_TWS_DEVICE_INFO_ERROR == ret_code)
	{
		SYS_LOG_ERR("breakpoint err.");
		app_mem_free(tp);
		return OTA_TWS_DEVICE_INFO_ERROR;
	}
	else if (OTA_TWS_SUCCESS == ret_code)
	{
		position = 1;
	}

	tp->cur_info.bk_checksum = tp->breakpoint.checksum;

	if (ota_resource_init(&tp->ota_hdl, (ota_st->fw_size & 0x7FFFFFFF), ota_st->fw_ver, NULL))
	{
		SYS_LOG_ERR("resource init fail.");
		app_mem_free(tp);
		return OTA_TWS_MEMORY_ERROR;
	}

	if (ota_st->tws_enable)
	{
		if (ota_st->fw_size & 0x80000000)
			tp->role = TWS_OTA_ROLE_SLAVE;
		else
			tp->role = TWS_OTA_ROLE_MASTER;

		os_sem_init(&tp->read_sem, 0, 1);
		tws_ota_hal = tp;
		tp->cur_info.fw_size = (ota_st->fw_size & 0x7FFFFFFF);
		tp->cur_info.fw_ver = ota_st->fw_ver;//ota_transmit_version_get();
		tp->cur_info.status = OTA_STATUS_READY;
		tp->cur_info.unique_id = ota_st->unique_id;
		ap_ota_tws_data_send((u8_t *)&tp->cur_info,
			sizeof(tws_ota_sync_info_t), TWS_OTA_INFO, tp->cur_info.pn);

		if (TWS_OTA_ROLE_MASTER == tp->role)
		{
			os_sem_reset(&tp->read_sem);
			os_sem_take(&tp->read_sem, 5000);
			
			if ((OTA_STATUS_READY != tp->across_info.status) ||
				(tp->cur_info.pn != tp->across_info.pn))
				goto ota_exit;
		}
		else
		{
			tp->across_info.bk_checksum = ota_st->checksum;
		}
		tp->cur_info.pn++;

		if (tp->cur_info.bk_checksum != tp->across_info.bk_checksum)
		{
			SYS_LOG_ERR("breakpoint is error.");
			sync_err = 1;
		}
	}

	if (((!position) && sync_err) ||
		((!position) && (!offset)))
	{
		ota_ap_breakpoint_clear();
		memset(&tp->breakpoint, 0, sizeof(ota_rs_breakponit_t));
	}
		
	if (offset)
	{
		tp->arrive_size = ota_ap_offset_calc(&tp->breakpoint);
		*offset = tp->arrive_size;
	}

	*phdl = tp;
	tws_ota_hal = tp;

	ota_resource_start(tp->ota_hdl, tp->arrive_size, (void *)&tp->breakpoint);

	return OTA_TWS_SUCCESS;

ota_exit:
	if (tws_ota_hal)
		app_mem_free(tws_ota_hal);
	tws_ota_hal = NULL;

	return OTA_TWS_DEVICE_INFO_ERROR;
}

u8_t ota_transmit_deinit(void **hdl)
{
	tws_ota_rs_handle_t *tp;

	if ((!tws_ota_hal) || (!hdl))
	{
		SYS_LOG_WRN("hdl 0x%p.",hdl);
		return OTA_TWS_PARAM_INVALID;
	}

	tp = (*(tws_ota_rs_handle_t **)hdl);
	if ((!tp) || (tp->exiting))
	{
		SYS_LOG_ERR("tp 0x%p.",tp);
		return OTA_TWS_PARAM_INVALID;
	}
	tp->exiting = 1;
	
	if (TWS_OTA_ROLE_MASTER == tp->role)
	{
		tp->cur_info.status = OTA_STATUS_HALT;
		ap_ota_tws_data_send((u8_t *)&tp->cur_info,
			sizeof(tws_ota_sync_info_t), TWS_OTA_INFO, 0);
	}

	ota_resource_deinit(
		&tp->ota_hdl, NULL);
	
	record_unique_id = tp->uniqueid;

	if (tws_ota_hal)
		app_mem_free(tws_ota_hal);
	tws_ota_hal = NULL;
	*hdl = NULL;

	return OTA_TWS_SUCCESS;
}

u8_t ota_transmit_fw_check(void *hdl)
{
	tws_ota_rs_handle_t *tp = (tws_ota_rs_handle_t *)hdl;

	if ((!tws_ota_hal) || (!tp))
	{
		SYS_LOG_ERR("hdl 0x%p.",tp);
		return OTA_TWS_PARAM_INVALID;
	}
	tp->discard = 1;

	if(ota_fw_check(tp->ota_hdl))
	{
		return OTA_TWS_FAIL;
	}

	if (TWS_OTA_ROLE_MASTER == tp->role)
	{
		tp->cur_info.status = OTA_STATUS_FW_CHECK;
		ap_ota_tws_data_send((u8_t *)&tp->cur_info,
			sizeof(tws_ota_sync_info_t), TWS_OTA_INFO, 0);
	}
	
	tp->fw_check_already = 1;
	//clear ota vram
	return OTA_TWS_SUCCESS;
}

u8_t ota_transmit_data_process(void *hdl, u8_t* payload, int payload_len)
{
	tws_ota_rs_handle_t *tp = (tws_ota_rs_handle_t *)hdl;
	u8_t ret = OTA_MW_SUCCESS;
	
	if ((!tws_ota_hal) || (!tp))
	{
		SYS_LOG_ERR("hdl 0x%p.",tp);
		return OTA_TWS_PARAM_INVALID;
	}	

	ret = ota_data_process(tp->ota_hdl,payload,payload_len);
	if (ret)
	{
		SYS_LOG_ERR(".");
		if (OTA_MW_PARAM_INVALID != ret)
			tp->discard = 1;

		return OTA_TWS_FAIL;
	}

	if (TWS_OTA_ROLE_MASTER == tp->role)
	{
		ap_ota_tws_data_send(payload, (u16_t)payload_len, TWS_OTA_DATA, tp->cur_info.cur_fn+1);
		//print_hex(CONST("data arrive:"), payload, payload_len);
	}

	if (TWS_OTA_ROLE_SINGLE != tp->role) 
	{
		if ((0 == tp->cur_info.cur_fn%5) ||
			(tp->arrive_size + 0x400 >= tp->cur_info.fw_size))
		{
			tp->cur_info.status = OTA_STATUS_INQUIRE;
			ap_ota_tws_data_send((u8_t *)&tp->cur_info,
				sizeof(tws_ota_sync_info_t), TWS_OTA_INFO, tp->cur_info.pn);
		
			if (TWS_OTA_ROLE_MASTER == tp->role)
			{
				SYS_LOG_INF("Waiting.");
				os_sem_reset(&tp->read_sem);
				os_sem_take(&tp->read_sem, 5000);
				SYS_LOG_INF("running.");
				if ((OTA_STATUS_INQUIRE != tp->across_info.status) ||
					(tp->cur_info.pn != tp->across_info.pn))
					return OTA_MW_PARSE_ERROR;
			}
			tp->cur_info.pn++;
		}

	}

	tp->cur_info.cur_fn++;
	tp->arrive_size += payload_len;

	return OTA_TWS_SUCCESS;
}

void ota_transmit_resource_release(u8_t tws)
{
	ota_tws_data_list_t*  packet = NULL;
	ota_tws_data_list_t*  next = NULL;

	if (tws_ota_hal)
	{
		ota_transmit_deinit((void *)&tws_ota_hal);
	}
	
	if (work_st)
	{
		list_for_each_entry_safe(packet, next, &work_st->ota_data_list, node)
		{
			SYS_LOG_WRN("release pn %d.",packet->fn);
			list_del(&packet->node);
			app_mem_free(packet->data_buf);
			app_mem_free(packet);
		}
	
		os_delayed_work_cancel(&work_st->ota_delay_work);
		app_mem_free(work_st);
		work_st = NULL;
	}
}

