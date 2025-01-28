/*!
 * \file	  
 * \brief	  nativevoice BLE sample code
 * \details   
 * \author 
 * \date	  
 * \copyright Actions
 */

#include <app_defines.h>
#include <sys_manager.h>
#include <bt_manager.h>
#include <msg_manager.h>
#include <app_switch.h>
//#include <app_config.h>
#include <os_common_api.h>

#include <user_comm/ap_status.h>
//#include <user_comm/tinycrypt/gcm.h>
//#include <user_comm/tinycrypt/platform.h>
//#include <user_comm/aes_mw.h>
#include <user_comm/ap_record.h>

#include "./library/mbedtls/gcm.h"
#include "./library/mbedtls/platform.h"

#include "nv_protocol.h"
#include "../nv_sppble/nv_ble_stream_ctrl.h"



enum BLE_NV_VAREQUESTED_CMD
{
	NV_START_SESSION = 0x01,
	NV_STOP_SESSION = 0x02,
	NV_SESSION_STARTED = 0x03,
	NV_SESSION_ACTIVE = 0x04,
	NV_SESSION_STOPPED = 0x05,
	NV_SESSION_TURN = 0x06,
	NV_SESSION_A2DP_STOP = 0x07,
	NV_SESSION_TONE_PLAY = 0x08,
};


enum BLE_CHARACTERISTIC_PRESENTATION_FORMAT
{
	BLE_CP_FORMAT_BOOL = 0x01,
	BLE_CP_FORMAT_UINT2 = 0x02,
	
	BLE_CP_FORMAT_UINT8 = 0x04,

	BLE_CP_FORMAT_UINT32 = 0x08,
	BLE_CP_FORMAT_UINT48 = 0x09,

	BLE_CP_FORMAT_UINT128 = 0x0B,

	BLE_CP_FORMAT_NUM_MAX = 0x1B,
};

enum BLE_NV_AUTH_CMD
{
	NV_RANDOM_SEND = 0x00,
	NV_CIPHER_SEND = 0x01,
	NV_AUTH_PASS = 0x02,
	NV_AUTH_FAIL = 0x03,
};

typedef struct
{
	u8_t   uuid[16];
	u8_t   properties;
	u8_t   format;
	u8_t   size;
	u8_t   *value;

} __attribute__((packed)) ble_nv_characteristic_t;

const u8_t aesgcmKey[16] = 
{
	0x56,0xE4,0x16,0x9A,0x11,0xC5,0x9F,0x80,
	0x00,0x14,0x96,0x3C,0x65,0xD0,0x46,0x65
};

const u8_t aesgcmIv[12] = 
{
	0x2E,0x4C,0x78,0x32,0xF4,0x10,
	0x07,0x22,0xA7,0xFB,0x0D,0xC9
};
	
const u8_t test_random[16] = 
{
	0xfd,0xfd,0x96,0x3C,0x65,0xD0,0x16,0x9A,
	0x11,0xC5,0x9F,0x80,0x00,0x14,0x46,0x65
};

void nv_upload_speech_stop_request(u8_t status);

typedef struct
{
	loop_buffer_t  tx_record_buf;
	u8_t siri_start;
	u8_t multi_turn;
	u8_t running;
	u8_t status;
	u8_t  key[16];
	u8_t  random[16];
	u8_t  ver_random[16];
	u8_t  iv[12];
	u8_t  cipher[16];
	u8_t  tag[16];
	u8_t  ver_tag[16];

	u8_t  update_random[16];
	u8_t  update_tag[16];

} ble_nv_context_t;

ble_nv_context_t*  ble_nv_context;
nv_user_cb_st nv_user_cb = {0};


int ble_nv_write(u8_t num, u8_t* data, int len)
{
	ble_nv_context_t*  p = ble_nv_context;
	int  ret_val = 0;

	if (!p)
		return -1;
	
	SYS_LOG_INF("num 0x%x.",num);
	if (len<16)
		print_hex("write:",data,len);

	ret_val = nv_send_pkg_to_stream(data, len, num);

	return ret_val;
}

void nv_va_resource_release(void)
{
	if (AP_STATUS_RECORDING == ap_status_get())
	{
		record_stop_record();
		ap_status_set(AP_STATUS_AUTH);
		app_switch_add_app(APP_ID_BTMUSIC);
		app_switch(APP_ID_BTMUSIC, APP_SWITCH_NEXT, false);
	}
	else 
	{
		SYS_LOG_INF("voice stop already!");
	}
}

void nv_va_resource_alloc(void)
{
	ble_nv_context_t*  p = ble_nv_context;
	if (!p)
		return;

	record_stream_init_param user_param;

	if (AP_STATUS_AUTH != ap_status_get())
	{
		SYS_LOG_INF("restart!");
	}
	else 
	{
		if (bt_manager_hfp_get_status() > 0)
			return;

		/* start record after in push-to-talk*/
		user_param.stream_send_cb = nv_send_pkg_to_stream;
		user_param.release_cb = nv_record_resource_release;
		//app_switch_add_app(APP_ID_AP_RECORD);
		//app_switch(APP_ID_AP_RECORD, APP_SWITCH_NEXT, false);
		record_start_record(&user_param);
		ap_status_set(AP_STATUS_RECORDING);
	}
	return;
}

u8_t nv_aes_gcm_encrypt(void)
{
	ble_nv_context_t*  p = ble_nv_context;

	if (!p)
		return 1;

	mbedtls_gcm_context ctx;
	//unsigned char buf[64];
	//unsigned char tag_buf[16];
	int i, j, ret;
	mbedtls_cipher_id_t cipher = MBEDTLS_CIPHER_ID_AES;

	for( j = 0; j < 1; j++ )
	{
		int key_len = 8*sizeof(p->key);//128 + 64 * j;

		for( i = 0; i < 1; i++ )
		{
			mbedtls_gcm_init( &ctx );

			SYS_LOG_INF("AES-GCM-%3d #%d (%s): ",
								key_len, i, "enc" );

			ret = mbedtls_gcm_setkey( &ctx, cipher, p->key,
									  key_len );
			/*
			 * AES-192 is an optional feature that may be unavailable when
			 * there is an alternative underlying implementation i.e. when
			 * MBEDTLS_AES_ALT is defined.
			 */
			if( ret == MBEDTLS_ERR_PLATFORM_FEATURE_UNSUPPORTED && key_len == 192 )
			{
				SYS_LOG_INF( "skipped\n" );
				break;
			}
			else if( ret != 0 )
			{
				SYS_LOG_ERR("setkey fail, ret %d\n",ret);
				goto exit;
			}

			ret = mbedtls_gcm_crypt_and_tag( &ctx, MBEDTLS_GCM_ENCRYPT,
										sizeof(p->random),
										p->iv, sizeof(p->iv),
										NULL, 0,
										p->random, p->cipher, sizeof(p->tag), p->tag);
			if( ret != 0)
			{
				SYS_LOG_ERR("crypt fail, ret %d\n",ret);
				goto exit;
			}
#if 0
			if ( memcmp( buf, ct[j * 6 + i], pt_len[i] ) != 0 ||
				 memcmp( tag_buf, tag[j * 6 + i], 16 ) != 0 )
			{
				ret = 1;
				goto exit;
			}
#endif
			mbedtls_gcm_free( &ctx );

			SYS_LOG_INF( "passed\n" );

			mbedtls_gcm_init( &ctx );

			SYS_LOG_INF( "	AES-GCM-%3d #%d (%s): ",
								key_len, i, "dec" );

			ret = mbedtls_gcm_setkey( &ctx, cipher, p->key,
									  key_len );
			if( ret != 0 )
				goto exit;

			ret = mbedtls_gcm_crypt_and_tag( &ctx, MBEDTLS_GCM_DECRYPT,
										sizeof(p->random),
										p->iv, sizeof(p->iv),
										NULL, 0,
										p->cipher, p->ver_random, sizeof(p->tag), p->ver_tag);

			if( ret != 0 )
				goto exit;

			if ((memcmp(p->random, p->ver_random, sizeof(p->random)) != 0) ||
				(memcmp(p->tag, p->ver_tag, sizeof(p->ver_tag)) != 0))
			{
				ret = 1;
				goto exit;
			}

			mbedtls_gcm_free( &ctx );

			SYS_LOG_INF( "passed\n" );
#if 0
			mbedtls_gcm_init( &ctx );

			log_debug( "  AES-GCM-%3d #%d split (%s): ",
								key_len, i, "enc" );

			ret = mbedtls_gcm_setkey( &ctx, cipher, p->key,
									  key_len );
			if( ret != 0 )
				goto exit;

			ret = mbedtls_gcm_starts( &ctx, MBEDTLS_GCM_ENCRYPT,
									  p->iv, sizeof(p->iv),
									  NULL, 0);
			if( ret != 0 )
				goto exit;

			ret = mbedtls_gcm_update( &ctx, sizeof(p->random), p->random, p->update_random);
			if( ret != 0 )
				goto exit;

			ret = mbedtls_gcm_finish( &ctx, p->update_tag, sizeof(p->update_tag));
			if( ret != 0 )
				goto exit;

			mbedtls_gcm_free( &ctx );

			log_debug( "passed\n" );

			mbedtls_gcm_init( &ctx );

			log_debug( "  AES-GCM-%3d #%d split (%s): ",
								key_len, i, "dec" );

			ret = mbedtls_gcm_setkey( &ctx, cipher, p->key,
									  key_len );

			if( ret != 0 )
				goto exit;

			ret = mbedtls_gcm_starts( &ctx, MBEDTLS_GCM_DECRYPT,
							  p->iv, sizeof(p->iv),
									  NULL, 0);
			if( ret != 0 )
				goto exit;

			ret = mbedtls_gcm_update(&ctx, sizeof(p->random), p->update_random, p->random);
			if( ret != 0 )
				goto exit;

			ret = mbedtls_gcm_finish( &ctx, p->tag, sizeof(p->tag));
			if( ret != 0 )
				goto exit;

			if( memcmp(p->update_random, p->random, sizeof(p->random)) != 0 ||
				memcmp(p->update_tag, p->tag, sizeof(p->tag)) != 0 )
			{
				ret = 1;
				goto exit;
			}

			mbedtls_gcm_free( &ctx );

			log_debug( "passed\n" );
#endif
		}
	}

	print_hex("random:",p->random,sizeof(p->random));
	print_hex("cipher:",p->cipher,sizeof(p->cipher));
	print_hex("tag:",p->tag,sizeof(p->tag));


	ret = 0;

exit:
	if( ret != 0 )
	{
		SYS_LOG_INF( "failed\n" );
		mbedtls_gcm_free( &ctx );
	}

	return( ret );
}

void nv_auth_set(u8_t *data)
{
	ble_nv_context_t*  p = ble_nv_context;
	u8_t  send_buf[33];

	if (!p)
		return;

	if (NV_RANDOM_SEND == data[0])
	{
		memcpy(p->random, &data[1] ,sizeof(p->random));
		memcpy(p->iv, &data[1 + sizeof(p->random)] ,sizeof(p->iv));
	}
	
	nv_aes_gcm_encrypt();
	send_buf[0] = NV_CIPHER_SEND;
	memcpy(&send_buf[1], p->cipher, sizeof(p->cipher));
	memcpy(&send_buf[1+sizeof(p->cipher)], p->tag, sizeof(p->tag));
	ble_nv_write(BLE_NV_AUTH, send_buf, 1+sizeof(p->cipher)+sizeof(p->tag));
}

void ble_nv_action(u8_t num, u8_t* data, int len)
{
	u16_t va_identifiers;
	ble_nv_context_t*  p = ble_nv_context;

	switch (num)
	{		 
		case BLE_NV_REQ:
		{
			if (data && len)
				print_hex("VARequseted:",data,len);

			memcpy(&va_identifiers, &data[0], sizeof(u16_t));
			SYS_LOG_INF("va_identifiers 0x%x",va_identifiers);
			// e.g. 0x00280003
			if (/*(0x28 == va_identifiers) &&*/ (NV_SESSION_STARTED == data[3]))
			{
				p->multi_turn = 0;
				p->siri_start = 0;
				p->running = 1;
				nv_va_resource_alloc();
			}
			// e.g. 0x00000004
			if ((0 == va_identifiers) && 
				(NV_SESSION_ACTIVE == data[3]))
			{
				p->running = 0;
				nv_upload_speech_stop_request(1);
			}
			
			// e.g. 0x00000006
			if ((0 == va_identifiers) && 
				(NV_SESSION_TURN == data[3]))
			{
				p->multi_turn = 1;
				nv_va_resource_release();
			}

			// e.g. 0x00000007
			if ((0 == va_identifiers) && 
				(NV_SESSION_A2DP_STOP == data[3]) &&
				(1 == p->multi_turn))
			{
				p->multi_turn = 0;
				nv_va_resource_alloc();
			}

			// e.g. 0x00000005
			if ((0 == va_identifiers) && (NV_SESSION_STOPPED == data[3]))
			{
				SYS_LOG_INF("STOP VPA session!");
			}

			// e.g. 0x05000001
			if ((0x0005 == va_identifiers) && (NV_START_SESSION == data[3]))
			{
				nv_va_resource_release();
				// start hfp
				bt_manager_hfp_start_siri();
				p->siri_start = 1;
				//stop session e.g. 0x00000200
			}

			// e.g. 0x00000008
			if (NV_SESSION_TONE_PLAY == data[3])
			{
				//bt_ma_va_tone_set(TRUE);
				if (bt_manager_tws_get_dev_role() == BTSRV_TWS_MASTER)
				{
					//bt_manager_tws_send_event_sync(TWS_UI_EVENT, SYS_EVENT_ALEXA_TONE_PLAY);
				}
				else
				{
					//sys_event_notify(SYS_EVENT_ALEXA_TONE_PLAY);
				}
			}

			break;
		}
		
		case BLE_NV_LWO:
		{
			if (data && len)
				print_hex("local WWE enable:",data,len); 
			break;
		}

		case BLE_NV_CID:
		{
			if (data && len)
				print_hex("Client ID:",data,len);
			
			if (nv_user_cb.nv_clientid_cb)
				nv_user_cb.nv_clientid_cb(data, len);

			break;
		}

		case BLE_NV_AUTH:
		{
			if (data && len)
				print_hex("AUTH:",data,len);
			
			if (NV_AUTH_PASS == data[0])
			{
				SYS_LOG_INF("AUTH PASS");
				ap_status_set(AP_STATUS_AUTH);
				break;
			}
			
			if (NV_AUTH_FAIL == data[0])
			{
				SYS_LOG_ERR("AUTH fail!");
				break;
			}
			nv_auth_set(data);

			break;
		}

	}

	return;
}

void nv_va_stop_session_handler(void)
{
	ble_nv_context_t*  p = ble_nv_context;
	u8_t payload[4];
	// 1: Native Voice Default 3: Google 5: Siri
	u16_t va_identifiers = 0;
	if(!p) 
	{
		SYS_LOG_ERR("err:NV_context is null.");
		return;
	}

	memcpy(&payload[0], &va_identifiers, sizeof(u16_t));
	payload[2] = NV_STOP_SESSION; //Stop Session
	payload[3] = 0;

	ble_nv_write(BLE_NV_REQ, &payload[0], 4);
}

void nv_upload_speech_stop_request(u8_t status)
{
	ble_nv_context_t*  p = ble_nv_context;

	if(!p) 
		return;

	if (BT_STATUS_SIRI == bt_manager_hfp_get_status())
	{
		// stop hfp
		//bt_manager_hfp_stop_siri();
		//p->siri_start = 0;

	}
	if (0 == status)
		nv_va_stop_session_handler();
	
	nv_va_resource_release();
}

void nv_upload_speech_request(void)
{
	ble_nv_context_t*  p = ble_nv_context;
	u8_t payload[4];
	// 1: Native Voice Default 3: Google 5: Siri
	u16_t va_identifiers = 0;
	
	
	if(!p) 
	{
		SYS_LOG_INF("err:NV_context is null.");
		return;
	}

	if (1 == p->running)
	{
		nv_upload_speech_stop_request(0);
		return;
	}
	p->siri_start = 0;
	p->multi_turn = 0;

	memcpy(&payload[0], &va_identifiers, sizeof(u16_t));
	payload[2] = NV_START_SESSION; //Start Session
	payload[3] = 0;

	ble_nv_write(BLE_NV_REQ, &payload[0], 4);
}

void nv_va_stop_session(void)
{
	ble_nv_context_t*  p = ble_nv_context;

	
	if (!p) 
		return;

	if (0 == p->siri_start) 
		return;

	if (0 == p->multi_turn)
		nv_va_stop_session_handler();
	else
		nv_va_resource_alloc();

	p->siri_start = 0;
	p->multi_turn = 0;
}


u8_t ble_nv_status(void)
{
	ble_nv_context_t*  p = ble_nv_context;

	if (!p)
		return 0;

	return p->running;
}


void nv_recv_proc(u8_t *buf, u16_t len)
{
	void *attr;
	u16_t payload_len;
	enum BLE_NV_CHARACTERISTICS num;
	
	memcpy(&attr, buf, sizeof(u32_t));
	memcpy(&payload_len, buf+sizeof(u32_t), sizeof(u16_t));
	
	num = nv_uuid_num_get(attr);

	ble_nv_action(num, buf+sizeof(u32_t)+sizeof(u16_t),payload_len);

	return;
}


void nv_resource_alloc(void)
{
	if (!ble_nv_context)
	{
		ble_nv_context = app_mem_malloc(sizeof(ble_nv_context_t));
		memset(ble_nv_context, 0, sizeof(ble_nv_context_t));
		memcpy(ble_nv_context->key,aesgcmKey,sizeof(ble_nv_context->key));
		//memcpy(p->iv,aesgcmIv,sizeof(p->iv));
		//memcpy(p->random,test_random,sizeof(p->random));
		if (nv_user_cb.nv_key_cb)
			nv_user_cb.nv_key_cb(ble_nv_context->key, sizeof(ble_nv_context->key));
		print_hex("key:",ble_nv_context->key,sizeof(ble_nv_context->key));
	}
}

void nv_resource_release(void)
{
	if (ble_nv_context)
	{
		//nv_notify_voice_stop();
		app_mem_free(ble_nv_context);
		ble_nv_context = NULL;
	}
}

void nv_dev_sent_record_start_stop(u8_t cmd)
{
	if (!ble_nv_context)
		return;

	if ((BT_STATUS_SIRI != bt_manager_hfp_get_status()) && 
		(0 != bt_manager_hfp_get_status()))
	{
		SYS_LOG_WRN("");
		return;
	}

	if (ap_status_get() >= AP_STATUS_AUTH)
	{
		nv_upload_speech_request();
	}		

	return;
}

void nv_user_cb_init(nv_user_cb_st *p_cb_st)
{
	if (!p_cb_st)
	{
		SYS_LOG_ERR("");
		return;
	}
	
	memcpy(&nv_user_cb.nv_vaserviceversion_cb, p_cb_st, sizeof(nv_user_cb_st));
}

struct nv_did {
	u32_t VendorID;
	u32_t Model;
	u32_t Serial;
	u32_t Reserved;
} __packed;

static const struct nv_did did = {
	.VendorID = 0xe003,
	.Model = 0,
	.Serial = 0,
	.Reserved = 0,
};

static const u8_t nv_sv[4] = {0x5, 0, 0x5, 0};

u16_t nv_sv_get(u8_t *buf)
{
	memcpy(buf, &nv_sv, sizeof(nv_sv));
	
	if (nv_user_cb.nv_vaserviceversion_cb)
		nv_user_cb.nv_vaserviceversion_cb(&buf[0], sizeof(nv_sv));

	return (sizeof(nv_sv));
}

u16_t nv_did_get(u8_t *buf)
{
	memcpy(buf, &did, sizeof(struct nv_did));
	
	if (nv_user_cb.nv_deviceid_cb)
		nv_user_cb.nv_deviceid_cb(&buf[0], sizeof(struct nv_did));

	return (sizeof(struct nv_did));
}

u16_t nv_cid_set(u8_t *buf, u16_t len)
{
	if (nv_user_cb.nv_clientid_cb)
		nv_user_cb.nv_clientid_cb(buf, len);

	return len;
}

