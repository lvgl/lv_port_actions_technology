#if 1//def CFG_TUYA_BLE_APP_SUPPORT

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <app_defines.h>
#include <sys_manager.h>
#include <bt_manager.h>
#include <msg_manager.h>
#include <app_switch.h>
//#include <app_config.h>
#include <os_common_api.h>

#include <zephyr/types.h>
#include "tuya_ble_port.h"
#include "tuya_ble_type.h"
#include "tuya_ble_mem.h"
#include "aes.h"
#include "md5.h"
#include "hmac.h"
#include "tuya_ble_internal_config.h"
#include <stdarg.h>

static OS_MUTEX_DEFINE(g_tuya_app_mutex);

int32_t rand(void)
{
	uint32_t trng_low, trng_high;
	se_trng_init();

	se_trng_process(&trng_low, &trng_high);
	se_trng_deinit();
	return trng_low;
}
tuya_ble_status_t tuya_ble_gap_advertising_adv_data_update(uint8_t const* p_ad_data, uint8_t ad_len)
{
	u8_t *p_buf = tuya_ble_malloc(ad_len);

	memcpy(p_buf, p_ad_data, ad_len);
	
	//printf("%s %d\n",	__func__, ad_len);
	print_hex("tuya adv", p_buf, ad_len);
	//ble_hci_set_advertising_data(p_buf,ad_len);
	//bt_manager_ble_set_adv_data(p_buf,ad_len,NULL,0);

	tuya_ble_free((u8_t *)p_buf);
	
	return TUYA_BLE_SUCCESS;
}

tuya_ble_status_t tuya_ble_gap_advertising_scan_rsp_data_update(uint8_t const *p_sr_data, uint8_t sr_len)
{
	//printf("%s %d\n",	__func__, sr_len);
	print_hex("tuya SCAN", p_sr_data, sr_len);
	//bt_manager_ble_set_adv_data(NULL,0,p_sr_data,sr_len);
	//ble_hci_set_scan_response_data((void *));
	return TUYA_BLE_SUCCESS;
}

tuya_ble_status_t tuya_ble_gap_disconnect(void)
{
	printf("%s\n",	__func__);
	//ble_hci_control_disconnect();
	//if(bt_manager_ble_is_connected())
	//	bt_manager_ble_disconnect();

	return TUYA_BLE_SUCCESS;
}

tuya_ble_status_t tuya_ble_gap_addr_get(tuya_ble_gap_addr_t *p_addr)
{
	//bt_manager_configs_t* p = bt_manager_get_configs();
	//dump_memory();
	//show_task_stack_heap_info();
	printf("%s\n",	__func__);
	p_addr->addr_type = TUYA_BLE_ADDRESS_TYPE_PUBLIC;
	if (property_get(VFD_BLE_DEV_ADDR, p_addr->addr) < 0)
	{
		memset(p_addr->addr, 0, 6);
	}

	return TUYA_BLE_SUCCESS;
}

tuya_ble_status_t tuya_ble_gap_addr_set(tuya_ble_gap_addr_t *p_addr)
{
	u8_t dev_addr[6];

	printf("%s\n",	__func__);
	
	//memcpy(dev_addr, p->bt_dev_addr, sizeof(p->bt_dev_addr));
	//memcpy(p->bt_dev_addr, p_addr->addr, sizeof(p->bt_dev_addr));
	
	//ble_host_set_local_addr();

	//vdisk_write(VFD_BLE_DEV_ADDR, p->bt_dev_addr, sizeof(p->bt_dev_addr));
	//memcpy(p->bt_dev_addr, dev_addr, sizeof(p->bt_dev_addr));
	//bt_manager_tws_send_event_ex(TWS_EVENT_SYNC_BLE_MAC_ADDR, p_addr->addr, sizeof(p->bt_dev_addr),YES);

	return TUYA_BLE_SUCCESS;
}


tuya_ble_status_t tuya_ble_gatt_send_data(const uint8_t *p_data,uint16_t len)
{
	uint16_t data_len = len;

	printf("%s %d\n",	__func__, len);

	if(data_len > TUYA_BLE_DATA_MTU_MAX)
	{
		data_len = TUYA_BLE_DATA_MTU_MAX;
	}

	tuya_ble_service_send_data((void *)p_data, data_len);
	return TUYA_BLE_SUCCESS;
}
	
tuya_ble_status_t tuya_ble_common_uart_init(void)
{	 
	return TUYA_BLE_SUCCESS;
}

tuya_ble_status_t tuya_ble_common_uart_send_data(const uint8_t *p_data,uint16_t len)
{
	return TUYA_BLE_SUCCESS;
}

/**@brief Application time-out handler type. */
typedef void (*app_timer_timeout_handler_t)(void * p_context);

// app_timer_t add by zdx
#define TIMER_MAX_NUM				4

typedef struct {
	u32_t id;
	uint8_t is_avail;
	tuya_ble_timer_mode mode;
	uint32_t repeat_period; /**< Repeat period (0 if single shot mode). */
	app_timer_timeout_handler_t handler;	   /**< User handler. */
	volatile bool				active; 	   /**< Flag indicating that timer is active. */
	volatile uint32_t			end_val;	   /**< RTC counter value when timer expires. */
} tuya_ble_timer_item_t;

//static tuya_ble_timer_item_t m_timer_pool[TIMER_MAX_NUM];

//static tuya_ble_timer_item_t* acquire_timer(uint32_t timeout_value_ms)
//{
//	  return NULL;
//}

static int32_t release_timer(u32_t timer_id)
{
	return -1;
}

//static tuya_ble_timer_item_t* find_timer(u32_t timer_id)
//{
//	  return NULL;
//}

void tuya_ble_timer_show(tuya_ble_timer_item_t *timer)
{
}

void tuya_ble_timer_handler(void* param, u32_t timer_id)
{
}

tuya_ble_status_t tuya_ble_timer_create(void** p_timer_id,uint32_t timeout_value_ms, tuya_ble_timer_mode mode,tuya_ble_timer_handler_t timeout_handler)
{
	return TUYA_BLE_SUCCESS;
}


tuya_ble_status_t tuya_ble_timer_delete(void* timer_id)
{
		return TUYA_BLE_SUCCESS;
}

tuya_ble_status_t tuya_ble_timer_start(void* timer_id)
{
		return TUYA_BLE_ERR_INTERNAL;	
}

tuya_ble_status_t tuya_ble_timer_restart(void* timer_id,uint32_t timeout_value_ms)
{
		return TUYA_BLE_ERR_INTERNAL;
}


tuya_ble_status_t tuya_ble_timer_stop(void* timer_id)
{
	return TUYA_BLE_SUCCESS;
}

void tuya_ble_device_delay_ms(uint32_t ms)
{
	printf("%s\n",	__func__);
	//sys_sleep_msec(ms);
	os_sleep(ms);
}

tuya_ble_status_t tuya_ble_rand_generator(uint8_t* p_buf, uint8_t len)
{
	uint32_t cnt = len / 4;
	uint8_t  remain = len % 4;
	int32_t temp;
	uint32_t i = 0;

	printf("%s %d\n", __func__, len);

	for(i = 0; i < cnt; i++)
	{
		temp = rand();
		memcpy(p_buf,(uint8_t *)&temp,4);
		p_buf += 4;
	}
	temp = rand();
	memcpy(p_buf,(uint8_t *)&temp,remain);

	return TUYA_BLE_SUCCESS;
}

tuya_ble_status_t tuya_ble_device_reset(void)
{
	struct app_msg	msg = {0};
	msg.type = MSG_REBOOT;
	msg.cmd = REBOOT_REASON_NORMAL;
	send_async_msg("main", &msg);

	return TUYA_BLE_SUCCESS;
}

//static uint_t __CR_NESTED = 0;	

void tuya_ble_device_enter_critical(void)
{
	os_mutex_lock(&g_tuya_app_mutex, OS_FOREVER);
}

void tuya_ble_device_exit_critical(void)
{
	os_mutex_unlock(&g_tuya_app_mutex);
}

tuya_ble_status_t tuya_ble_rtc_get_timestamp(uint32_t *timestamp,int32_t *timezone)
{
	printf("%s\n",	__func__);

	*timestamp = 0;
	*timezone = 0;

	return TUYA_BLE_SUCCESS;
}

tuya_ble_status_t tuya_ble_rtc_set_timestamp(uint32_t timestamp,int32_t timezone)
{
	printf("%s\n",	__func__);

	return TUYA_BLE_SUCCESS;
}


char* tuya_ble_nv_addr_to_vfd(uint32_t addr)
{
	switch(addr)
	{
		case TUYA_BLE_AUTH_FLASH_ADDR:
			return VFD_TUYA_AUTH;
		case TUYA_BLE_AUTH_FLASH_BACKUP_ADDR:
			return VFD_TUYA_AUTH_BAK;
		case TUYA_BLE_SYS_FLASH_ADDR:
			return VFD_TUYA_SYS;
		case TUYA_BLE_SYS_FLASH_BACKUP_ADDR:
			return VFD_TUYA_SYS_BAK;
		case TUYA_NV_VOS_TOKEN_START_ADDR:
			return VFD_TUYA_VOS_TOKEN;
	}

	return NULL;
}

tuya_ble_status_t tuya_ble_nv_init(void)
{	 

	return TUYA_BLE_SUCCESS;
}

tuya_ble_status_t tuya_ble_nv_erase(uint32_t addr,uint32_t size)
{	 
	return TUYA_BLE_SUCCESS;
}


int tuya_tws_send_nv_info(u32_t event, void* data, int size, uint32_t addr)
{


	return 0;
}

tuya_ble_status_t tuya_ble_nv_write(u32_t addr,const uint8_t *p_data, uint32_t size)
{  
	tuya_ble_status_t result = TUYA_BLE_SUCCESS;

	//bt_manager_context_t*  bt_manager = bt_manager_get_context();

	printf("%s 0x%x %d\n",	__func__, addr, size);

	char *name = tuya_ble_nv_addr_to_vfd(addr);
	if (!name)
	{
		printf("%s: map to vfd failed, addr %x size %d\n",	__func__, addr, size);
		return TUYA_BLE_ERR_INVALID_PARAM;
	}
	printf("%s name %s\n",	__func__,name);

	if (property_set(name, p_data, size) != 0)
		result = TUYA_BLE_ERR_INTERNAL;

	if (BTSRV_TWS_MASTER == bt_manager_tws_get_dev_role())
	{
		//tuya_tws_send_nv_info(TWS_EVENT_TUYA_NV_SYNC, (void *)p_data, size, addr);
    }
	return result;
}

tuya_ble_status_t tuya_ble_nv_read(u32_t addr,uint8_t *p_data, uint32_t size)
{
	char *name = tuya_ble_nv_addr_to_vfd(addr);

	// printf("%s 0x%x %d\n",	__func__, addr, size);

	if (!name)
	{
		printf("%s: map to vfd failed, addr %x size %d\n",	__func__, addr, size);
		return TUYA_BLE_ERR_INVALID_PARAM;
	}
	printf("%s name %s\n",	__func__,name);
	
	if (property_get(name, p_data, size) < 0)
		return TUYA_BLE_ERR_INTERNAL;	


	if (BTSRV_TWS_MASTER == bt_manager_tws_get_dev_role())
	{
		//tuya_tws_send_nv_info(TWS_EVENT_TUYA_NV_SYNC, p_data, size, addr);
	}
	return TUYA_BLE_SUCCESS;
}

#if TUYA_BLE_USE_OS

// do nothing, app task create by system
bool tuya_ble_os_task_create(void **pp_handle, const char *p_name, void (*p_routine)(void *),void *p_param, uint16_t stack_size, uint16_t priority)
{
	return TUYA_BLE_SUCCESS;
}

// no use
bool tuya_ble_os_task_delete(void *p_handle)
{
	printf("%s\n",	__func__);
	return TUYA_BLE_SUCCESS;
}

// no use
bool tuya_ble_os_task_suspend(void *p_handle)
{
	printf("%s\n",	__func__);
	return TUYA_BLE_SUCCESS;
}

// no use
bool tuya_ble_os_task_resume(void *p_handle)
{
	printf("%s\n",	__func__);
	return TUYA_BLE_SUCCESS;
}

// do nothing, app task handle instead
bool tuya_ble_os_msg_queue_create(void **pp_handle, uint32_t msg_num, uint32_t msg_size)
{
	printf("%s\n",	__func__);
	return TUYA_BLE_SUCCESS;
}

// no use
bool tuya_ble_os_msg_queue_delete(void *p_handle)
{
	printf("%s\n",	__func__);
	return TUYA_BLE_SUCCESS;
}

// no use
bool tuya_ble_os_msg_queue_peek(void *p_handle, uint32_t *p_msg_num)
{
	printf("%s\n",	__func__);
	return TUYA_BLE_SUCCESS;
}

bool tuya_ble_os_msg_queue_send(void *p_handle, void *p_msg, uint32_t wait_ms)
{
	int msg_size = wait_ms;
	
	//tuya_ble_evt_param_t *evt = p_msg;
	
	//evt->hdr.event_handler = tuya_ble_gatt_send_data;
	//print_hex("tuya_ble_os_msg_queue_send:", evt->hdr.event, evt->hdr.event_handler);
	//SYS_LOG_INF("event %d event_handler 0x%x", evt->hdr.event, evt->hdr.event_handler);
	//print_hex("msg_data:", p_msg, msg_size);

	//if (tuya_ble_app_message_send_sync(p_msg, msg_size) == -1)
	if (tuya_ble_app_handle(p_msg, msg_size))
		return false;
	else
		return true;
}

// do nothing, app task handle instead
bool tuya_ble_os_msg_queue_recv(void *p_handle, void *p_msg, uint32_t wait_ms)
{
	printf("%s\n",	__func__);
	return TUYA_BLE_SUCCESS;
}
#endif

bool tuya_ble_aes128_ecb_encrypt(uint8_t *key,uint8_t *input,uint16_t input_len,uint8_t *output)
{
	uint16_t length;
	mbedtls_aes_context *aes_ctx = (mbedtls_aes_context *)tuya_ble_malloc(sizeof(mbedtls_aes_context));

	printf("%s %d %d\n",  __func__, *key, input_len);
	//print_hex("encrypt in:", input, input_len);

	if(input_len % 16)
	{
		tuya_ble_free((u8_t *)aes_ctx);
		return false;
	}

	length = input_len;

	mbedtls_aes_init(aes_ctx);

	mbedtls_aes_setkey_enc(aes_ctx, key, 128);

	while(length > 0)
	{
		mbedtls_aes_crypt_ecb(aes_ctx, MBEDTLS_AES_ENCRYPT, input, output );
		input  += 16;
		output += 16;
		length -= 16;
	}

	mbedtls_aes_free(aes_ctx);
	tuya_ble_free((u8_t *)aes_ctx);

	//print_hex("encrypt out:", output, input_len);

	return true;
}

bool tuya_ble_aes128_ecb_decrypt(uint8_t *key,uint8_t *input,uint16_t input_len,uint8_t *output)
{
	uint16_t length;
	mbedtls_aes_context *aes_ctx = (mbedtls_aes_context *)tuya_ble_malloc(sizeof(mbedtls_aes_context));

	printf("%s %d %d\n",  __func__, *key, input_len);
	//print_hex("decrypt in:", input, input_len);

	if(input_len % 16)
	{
		tuya_ble_free((u8_t *)aes_ctx);
		return false;
	}

	length = input_len;

	mbedtls_aes_init(aes_ctx);

	mbedtls_aes_setkey_dec(aes_ctx, key, 128);

	while(length > 0)
	{
		mbedtls_aes_crypt_ecb(aes_ctx, MBEDTLS_AES_DECRYPT, input, output );
		input  += 16;
		output += 16;
		length -= 16;
	}

	mbedtls_aes_free(aes_ctx);
	tuya_ble_free((u8_t *)aes_ctx);
	//print_hex("decrypt out:", output, input_len);

	return true;
}

bool tuya_ble_aes128_cbc_encrypt(uint8_t *key,uint8_t *iv,uint8_t *input,uint16_t input_len,uint8_t *output)
{
	mbedtls_aes_context *aes_ctx = (mbedtls_aes_context *)tuya_ble_malloc(sizeof(mbedtls_aes_context));

	printf("%s %d %d\n",  __func__, *key, input_len);
	//print_hex("aes128 in:", input, input_len);
	//print_hex("aes128 key:", key, input_len);
	//print_hex("aes128 iv:", iv, input_len);

	if(input_len % 16)
	{
		tuya_ble_free((u8_t *)aes_ctx);
		return false;
	}

	mbedtls_aes_init(aes_ctx);

	mbedtls_aes_setkey_enc(aes_ctx, key, 128);
	
	mbedtls_aes_crypt_cbc(aes_ctx,MBEDTLS_AES_ENCRYPT,input_len,iv,input,output);

	mbedtls_aes_free(aes_ctx);
	tuya_ble_free((u8_t *)aes_ctx);

	//print_hex("aes128 out:", output, input_len);

	return true;
}

bool tuya_ble_aes128_cbc_decrypt(uint8_t *key,uint8_t *iv,uint8_t *input,uint16_t input_len,uint8_t *output)
{
	mbedtls_aes_context *aes_ctx = (mbedtls_aes_context *)tuya_ble_malloc(sizeof(mbedtls_aes_context));

	printf("%s %d %d\n",  __func__, *key, input_len);
	//print_hex("decrypt in:", input, input_len);

	if(input_len%16)
	{
		tuya_ble_free((u8_t *)aes_ctx);
		return false;
	}

	mbedtls_aes_init(aes_ctx);

	mbedtls_aes_setkey_dec(aes_ctx, key, 128);
	
	mbedtls_aes_crypt_cbc(aes_ctx, MBEDTLS_AES_DECRYPT, input_len, iv, input,output);

	mbedtls_aes_free(aes_ctx);
	tuya_ble_free((u8_t *)aes_ctx);

	//print_hex("decrypt out:", output, input_len);

	return true;
}

// ok, checked
bool tuya_ble_md5_crypt(uint8_t *input,uint16_t input_len,uint8_t *output)
{
	printf("%s %d\n",  __func__, input_len);
	//print_hex("md5 in:", input, input_len);

	mbedtls_md5_context md5_ctx;
	mbedtls_md5_init(&md5_ctx);
	mbedtls_md5_starts(&md5_ctx);
	mbedtls_md5_update(&md5_ctx, input, input_len);
	mbedtls_md5_finish(&md5_ctx, output);
	mbedtls_md5_free(&md5_ctx);    

	//print_hex("md5 out:", output, 16);
	return true;
}

bool tuya_ble_hmac_sha1_crypt(const uint8_t *key, uint32_t key_len, const uint8_t *input, uint32_t input_len, uint8_t *output)
{	 
	printf("%s %d %d\n",  __func__, key_len, input_len);
	hmac_sha1_crypt(key, key_len, input, input_len, output);
	return true;
}

bool tuya_ble_hmac_sha256_crypt(const uint8_t *key, uint32_t key_len, const uint8_t *input, uint32_t input_len, uint8_t *output)
{
	printf("%s %d %d\n",  __func__, key_len, input_len);
	hmac_sha256_crypt(key, key_len, input, input_len, output);
	return true;
}
u16_t g_num = 0;

#if (TUYA_BLE_USE_PLATFORM_MEMORY_HEAP==1)
void *tuya_ble_port_malloc( uint32_t size )
{
	void *ptr = app_mem_malloc(size);
	g_num++;
	//SYS_LOG_ERR("ptr 0x%p size %d g_num %d.",ptr, size,g_num);
	
	return ptr;
}


void tuya_ble_port_free( void *pv )
{
	g_num--;
	//SYS_LOG_ERR("ptr 0x%p g_num %d.",pv,g_num);
	app_mem_free(pv);
}

#endif


#endif


