#if 1//def CFG_TUYA_BLE_APP_SUPPORT


#include "tuya_ble_stdlib.h"
#include "tuya_ble_type.h"
#include "tuya_ble_heap.h"
#include "tuya_ble_mem.h"
#include "tuya_ble_api.h"
#include "tuya_ble_port.h"
#include "tuya_ble_main.h"
#include "tuya_ble_secure.h"
#include "tuya_ble_data_handler.h"
#include "tuya_ble_storage.h"
#include "tuya_ble_sdk_version.h"
#include "tuya_ble_utils.h"
#include "tuya_ble_event.h"
#include "tuya_ble_app_demo.h"
#include "tuya_ble_demo_version.h"
#include "tuya_ble_log.h"
#include "tuya_ble_mutli_tsf_protocol.h"
#include <bt_manager.h>
#include <user_comm/ap_status.h>
#include <user_comm/ap_ota_transmit.h>

#define MAX_DFU_DATA_LEN  512//(128+64)



typedef struct
{
    /***********ota start************/
    void           *ota_hdl;
    u32_t          rx_length_already;
    u16_t           last_fn;
    u16_t           curr_fn;
    u16_t           end_fn;
	//u8_t  		   repeat_send_timer;
	//u8_t  		   timeout_times;
    u32_t 		   fw_size;
	u32_t 		   fw_version;
	u32_t 		   crc32;
	u32_t 		   last_bk_crc32;
	u8_t           *data_buf;
	u16_t           temp_data_len;
	//tuya_ota_bk_t tuya_bk;
    /***********ota end************/
} tuy_ota_ctx_t;

extern void tuya_ble_ota_status_set(tuya_ble_ota_status_t status);
tuya_ble_ota_status_t tuya_ble_ota_status_get(void);


tuy_ota_ctx_t *ota_ctx = NULL;
static u32_t last_crc_length = 0;
static u32_t last_rx_crc = 0;

#if 1
static uint32_t crc32_compute(uint8_t const * p_data, uint32_t size, uint32_t const * p_crc)
{
	uint32_t crc;
	uint32_t i = 0, j = 0;

    crc = (p_crc == NULL) ? 0xFFFFFFFF : ~(*p_crc);
    for (i = 0; i < size; i++)
    {
        crc = crc ^ p_data[i];
        for (j = 8; j > 0; j--)
        {
            crc = (crc >> 1) ^ (0xEDB88320U & ((crc & 1) ? 0xFFFFFFFF : 0));
        }
    }
    return ~crc;
}
#endif
void tuya_ota_resource_release(void)
{
    tuy_ota_ctx_t *p = ota_ctx;

    if (p)
    {
        if (p->ota_hdl)
        {
			ota_transmit_deinit(&p->ota_hdl);
        }

		if (p->data_buf)
			tuya_ble_free(p->data_buf);
		
		tuya_ble_free((u8_t *)p);
		ota_ctx = NULL;
		
		if ((AP_STATUS_OTA_RUNING == ap_status_get()) ||
			(AP_STATUS_ERROR == ap_status_get()))
			ap_status_set(AP_STATUS_AUTH);

		tuya_ble_ota_status_set(TUYA_BLE_OTA_STATUS_NONE);
    }
}

void tuya_ota_start_req(uint8_t*recv_data,uint32_t recv_len)
{
    uint8_t p_buf[20];
    uint32_t current_version = TY_APP_VER_NUM; //V0.2.0
	tuya_ble_ota_response_t tuya_ble_ota_data_response;
	u16_t dfu_length;

    if(tuya_ble_ota_status_get()!=TUYA_BLE_OTA_STATUS_NONE)
    {
        TUYA_APP_LOG_ERROR("current ota status is not TUYA_OTA_STATUS_NONE  and is : %d !",tuya_ble_ota_status_get());
        return;
    }

	if (bt_manager_get_ble_mtu() < 128)
	{
	    TUYA_APP_LOG_ERROR("mtu not support ota.");
        return;	
	}
	
	dfu_length = MAX_DFU_DATA_LEN;//ble_att_get_mtu_size() - 64;


    p_buf[0] = 0x00;
    p_buf[1] = 0x03;
    p_buf[2] = 0;
    p_buf[3] = current_version>>24;
    p_buf[4] = current_version>>16;
    p_buf[5] = current_version>>8;
    p_buf[6] = current_version;
    p_buf[7] = dfu_length>>8;
    p_buf[8] = dfu_length;
    tuya_ble_ota_status_set(TUYA_BLE_OTA_STATUS_START);

	tuya_ble_ota_data_response.type = TUYA_BLE_OTA_REQ;
	tuya_ble_ota_data_response.data_len = 9;
	tuya_ble_ota_data_response.p_data = p_buf;
	print_hex("[otaTx]", tuya_ble_ota_data_response.p_data,tuya_ble_ota_data_response.data_len);
    tuya_ble_ota_response(&tuya_ble_ota_data_response);
}

void tuya_ota_file_info_req(uint8_t*recv_data,uint32_t recv_len)
{
	tuy_ota_ctx_t *p = NULL;

    uint8_t p_buf[30];
    uint32_t file_version;
    uint32_t file_length;
    uint32_t file_crc;
	//u8_t md5_value[16];
	uint32_t i;
	uint32_t cksum = 0;
    uint8_t state = 0;
	//tuya_ota_bk_t tuya_bk;
	u8_t  *sptr;

	tuya_ble_ota_response_t tuya_ble_ota_data_response;
	
    if(tuya_ble_ota_status_get()!=TUYA_BLE_OTA_STATUS_START)
    {
        TUYA_APP_LOG_ERROR("current ota status is not TUYA_OTA_STATUS_START  and is : %d !",tuya_ble_ota_status_get());
        return;
    }

    if(recv_data[0]!=0x00)
    {
        TUYA_APP_LOG_ERROR("current ota fm type is not 0!");
        return;
    }

    file_version = recv_data[1+8]<<24;
    file_version += recv_data[2+8]<<16;
    file_version += recv_data[3+8]<<8;
    file_version += recv_data[4+8];
    file_length = recv_data[29]<<24;
    file_length += recv_data[30]<<16;
    file_length += recv_data[31]<<8;
    file_length += recv_data[32];

    file_crc = recv_data[33]<<24;
    file_crc += recv_data[34]<<16;
    file_crc += recv_data[35]<<8;
    file_crc += recv_data[36];

	//memcpy(&file_version, &recv_data[1+8], sizeof(file_version));
	//memcpy(&md5_value[0], &recv_data[1+8+4], sizeof(md5_value));
	//memcpy(&file_length, &recv_data[1+8+4+16], sizeof(file_length));
	//memcpy(&file_crc, &recv_data[1+8+4+16+4], sizeof(file_crc));
	TUYA_APP_LOG_INFO("V 0x%x R 0x%x C 0x%x", file_version,file_length,file_crc);

    if (memcmp(&recv_data[1], (void *)&tuya_ble_current_para->pid[0], 8) == 0)
    {
        if (file_version > TY_APP_VER_NUM)
        {
			state = 0;
        }
        else
        {
            if(file_version <= TY_APP_VER_NUM)
            {
                TUYA_APP_LOG_ERROR("ota file version error !");
                state = 2;
            }
            else
            {
                TUYA_APP_LOG_ERROR("ota file length is bigger than rev space !");
                state = 3;
            }
        }

    }
    else
    {
        TUYA_APP_LOG_ERROR("ota pid error !");
        state = 1;
    }

    memset(p_buf,0,sizeof(p_buf));
    p_buf[0] = 0x00;
    if(state==0)
    {
		if (!ota_ctx)
		{
			ota_ctx = (tuy_ota_ctx_t *) tuya_ble_malloc (sizeof(tuy_ota_ctx_t));
		}
		p = ota_ctx;

		if (p->ota_hdl)
		{
			tuya_ota_resource_release();
			state = 4;
		}
		else
		{
			memset(p, 0 ,sizeof(tuy_ota_ctx_t));
			p->fw_size = file_length;
			p->crc32 = file_crc;

			p_buf[2] = last_crc_length>>24;
			p_buf[3] = last_crc_length>>16;
			p_buf[4] = last_crc_length>>8;
			p_buf[5] = last_crc_length;
			p_buf[6] = last_rx_crc>>24;
			p_buf[7] = last_rx_crc>>16;
			p_buf[8] = last_rx_crc>>8;
			p_buf[9] = last_rx_crc;

			tuya_ble_ota_status_set(TUYA_BLE_OTA_STATUS_FILE_INFO);
			ap_status_set(AP_STATUS_OTA_RUNING);
		}
    }
	p_buf[1] = state;

    tuya_ble_ota_data_response.type = TUYA_BLE_OTA_FILE_INFO;
	tuya_ble_ota_data_response.data_len = 26;
	tuya_ble_ota_data_response.p_data = p_buf;
	print_hex("[otaTx]", tuya_ble_ota_data_response.p_data,tuya_ble_ota_data_response.data_len);
    tuya_ble_ota_response(&tuya_ble_ota_data_response);
}

void tuya_ota_offset_req(uint8_t*recv_data,uint32_t recv_len)
{
	tuy_ota_ctx_t *p = ota_ctx;
	ota_transmit_st ota_st;

    uint8_t p_buf[5];
    uint32_t *ptr = NULL;
	uint32_t offset;
	tuya_ble_ota_response_t tuya_ble_ota_data_response;
	
    if(tuya_ble_ota_status_get()!=TUYA_BLE_OTA_STATUS_FILE_INFO)
    {
        TUYA_APP_LOG_ERROR("current ota status is not TUYA_OTA_STATUS_FILE_INFO  and is : %d !",tuya_ble_ota_status_get());
        return;
    }

	if (!p)
    {
        TUYA_APP_LOG_ERROR("");
        return;
    }

    offset  = recv_data[1]<<24;
    offset += recv_data[2]<<16;
    offset += recv_data[3]<<8;
    offset += recv_data[4];	
	//memcpy(&offset, &recv_data[1], sizeof(offset));
	
    p_buf[0] = 0x00;

	if ((offset) && last_crc_length)
	{
		ptr = (&offset);
	}

	ota_st.fw_size = p->fw_size;
	ota_st.fw_ver = 0xFFFFFFFF;
	ota_st.tws_enable = 1;
	ota_st.unique_id = p->crc32;
	if (OTA_TWS_SUCCESS != ota_transmit_init(&p->ota_hdl, &ota_st, ptr))
	{
		tuya_ota_resource_release();
	}
	else
	{
	    p_buf[1] = offset>>24;
	    p_buf[2] = offset>>16;
	    p_buf[3] = offset>>8;
	    p_buf[4] = offset;	
		//memcpy(&p_buf[2], &offset, sizeof(u32_t));
		p->rx_length_already = offset;
		if (!offset)
		{
			last_rx_crc = 0;
			last_crc_length = 0;
		}

		TUYA_APP_LOG_INFO("offset %d rx_length_already %d.",
			offset, p->rx_length_already);
		tuya_ble_ota_status_set(TUYA_BLE_OTA_STATUS_FILE_OFFSET);
	}
    
    tuya_ble_ota_data_response.type = TUYA_BLE_OTA_FILE_OFFSET_REQ;
	tuya_ble_ota_data_response.data_len = 5;
	tuya_ble_ota_data_response.p_data = p_buf;
	print_hex("[otaTx]", tuya_ble_ota_data_response.p_data,tuya_ble_ota_data_response.data_len);
    tuya_ble_ota_response(&tuya_ble_ota_data_response);
}

void tuya_ota_data_req(uint8_t*recv_data,uint32_t recv_len)
{
	tuy_ota_ctx_t *p = ota_ctx;
    uint8_t p_buf[2];
    uint8_t state = 0;
    uint16_t len;
	u16_t crc16 = 0;
	u16_t crc_len = 0;
	tuya_ble_ota_response_t tuya_ble_ota_data_response;

    if((tuya_ble_ota_status_get()!=TUYA_BLE_OTA_STATUS_FILE_OFFSET) &&
		(tuya_ble_ota_status_get()!=TUYA_BLE_OTA_STATUS_FILE_DATA))
    {
        TUYA_APP_LOG_ERROR("current ota status is not TUYA_OTA_STATUS_FILE_OFFSET  or TUYA_OTA_STATUS_FILE_DATA and is : %d !",tuya_ble_ota_status_get());
        return;
    }

 	if (!p)
    {
        TUYA_APP_LOG_ERROR("");
        return;
    }
	
    state = 0;
    p->curr_fn = (recv_data[1]<<8)|recv_data[2];
    len = (recv_data[3]<<8)|recv_data[4];
	crc16 = (recv_data[5]<<8)|recv_data[6];

	//memcpy(&p->curr_fn, &recv_data[1], sizeof(u16_t));
	//memcpy(&len, &recv_data[3], sizeof(u16_t));
	//memcpy(&crc16, &recv_data[5], sizeof(u16_t));

	if ((0 == p->rx_length_already) && (0 == p->temp_data_len))
	{
		//p->head_check = 0;
		if (0 != memcmp(&recv_data[7], "AOTA", 4))
		{
			TUYA_APP_LOG_ERROR("");
			state = 8;
		}		
	}

	if (bt_manager_hfp_get_status() > 0)
	{
		TUYA_APP_LOG_ERROR("");
		state = 9;
	}

	if (0 == state)
	{
		TUYA_APP_LOG_INFO("curr_fn %d last_fn %d",p->curr_fn,p->last_fn);
		if ((p->curr_fn) && (p->curr_fn != (p->last_fn + 1)))
		{
			state = 1;
		}
	    else if (len > MAX_DFU_DATA_LEN)
	    {
	        TUYA_APP_LOG_ERROR("ota received package data length error : %d",len);
	        state = 5;
	    }
		else if ((p->temp_data_len + len) > (MAX_DFU_DATA_LEN))
		{
			TUYA_APP_LOG_ERROR("temp_data_len: %d len: %d.",p->temp_data_len, len);
			state = 5;
		}
	    else
	    {
			if (!p->data_buf)
			{
				p->data_buf = (u8_t *)tuya_ble_malloc(MAX_DFU_DATA_LEN);
			}
			
			memcpy(p->data_buf + p->temp_data_len, &recv_data[7], len);
			p->temp_data_len += len;
			p->last_fn = p->curr_fn;
			if ((p->temp_data_len == (MAX_DFU_DATA_LEN)) || 
				((p->rx_length_already + p->temp_data_len) == p->fw_size))
			{
				if (p->rx_length_already >= last_crc_length)
				{
					last_rx_crc = crc32_compute(p->data_buf, p->temp_data_len, &last_rx_crc);
					last_crc_length += p->temp_data_len;
				} 
				else if (p->rx_length_already+p->temp_data_len > last_crc_length)
				{
					crc_len = last_crc_length - p->rx_length_already;
					//crc_len = p->rx_length_already + p->temp_data_len - last_crc_length;
					last_rx_crc = crc32_compute(p->data_buf+crc_len, p->temp_data_len-crc_len, &last_rx_crc);
					last_crc_length += crc_len;
				}
				
				if (OTA_TWS_SUCCESS == ota_transmit_data_process(p->ota_hdl, p->data_buf, p->temp_data_len))
				{
					p->rx_length_already += p->temp_data_len;
					p->temp_data_len = 0;
				}
				else
				{
					state = 6;
				}
			}
	    }
	}

    p_buf[0] = 0x00;
    p_buf[1] = state;

    tuya_ble_ota_status_set(TUYA_BLE_OTA_STATUS_FILE_DATA);

    tuya_ble_ota_data_response.type = TUYA_BLE_OTA_DATA;
	tuya_ble_ota_data_response.data_len = 2;
	tuya_ble_ota_data_response.p_data = p_buf;
	print_hex("[otaTx]", tuya_ble_ota_data_response.p_data,tuya_ble_ota_data_response.data_len);
    tuya_ble_ota_response(&tuya_ble_ota_data_response);

    if(state)
    {
        TUYA_APP_LOG_ERROR("ota error so free!");
        tuya_ota_resource_release();
    }
}

void tuya_ota_end_req(uint8_t*recv_data,uint32_t recv_len)
{
    uint8_t p_buf[2];
	tuya_ble_ota_response_t tuya_ble_ota_data_response;
    tuy_ota_ctx_t *p = ota_ctx;
    u8_t ret;
	u8_t status = 0;

    if(tuya_ble_ota_status_get()==TUYA_BLE_OTA_STATUS_NONE)
    {
        TUYA_APP_LOG_ERROR("current ota status is TUYA_OTA_STATUS_NONE!");
        return;
    }

	if (p->rx_length_already != p->fw_size)
		status = 1;

	ret = ota_transmit_fw_check(p->ota_hdl);
    if (OTA_TWS_SUCCESS != ret)
		status = 2;
	
	p_buf[0] = 0;
	p_buf[1] = status;
    tuya_ble_ota_data_response.type = TUYA_BLE_OTA_END;
	tuya_ble_ota_data_response.data_len = 2;
	tuya_ble_ota_data_response.p_data = p_buf;
	print_hex("[otaTx]", tuya_ble_ota_data_response.p_data,tuya_ble_ota_data_response.data_len);
    tuya_ble_ota_response(&tuya_ble_ota_data_response);

	if (status)
    	tuya_ota_resource_release();

    return;
}

void tuya_ota_proc(uint16_t cmd,uint8_t*recv_data,uint32_t recv_len)
{
    switch(cmd)
    {
    case TUYA_BLE_OTA_REQ:
        tuya_ota_start_req(recv_data,recv_len);
        break;
    case TUYA_BLE_OTA_FILE_INFO:
        tuya_ota_file_info_req(recv_data,recv_len);
        break;
    case TUYA_BLE_OTA_FILE_OFFSET_REQ:
        tuya_ota_offset_req(recv_data,recv_len);
        break;
    case TUYA_BLE_OTA_DATA:
        tuya_ota_data_req(recv_data,recv_len);
        break;
    case TUYA_BLE_OTA_END:
        tuya_ota_end_req(recv_data,recv_len);
        break;
    default:
    	TUYA_APP_LOG_ERROR("tuya_ota_proc cmd err.");
        break;
    }

}

void tuya_ota_send_fail(void)
{
    uint8_t p_buf[2];
    uint8_t state = 0;
	tuya_ble_ota_response_t tuya_ble_ota_data_response;

	state = 10;
    p_buf[0] = 0x00;
    p_buf[1] = state;

    tuya_ble_ota_data_response.type = TUYA_BLE_OTA_DATA;
	tuya_ble_ota_data_response.data_len = 2;
	tuya_ble_ota_data_response.p_data = p_buf;
	print_hex("[slave otatx]", tuya_ble_ota_data_response.p_data,tuya_ble_ota_data_response.data_len);
    tuya_ble_ota_response(&tuya_ble_ota_data_response);
}

#endif

