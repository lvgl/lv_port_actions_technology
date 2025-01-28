
#ifndef __AP_OTA_TRANSMIT_H__
#define __AP_OTA_TRANSMIT_H__

enum OTA_TWS_RTN_VAL
{
	OTA_TWS_SUCCESS 		  = 0x00,
	OTA_TWS_PARAM_INVALID	  = 0x01,
	OTA_TWS_MEMORY_ERROR	  = 0x02,
	OTA_TWS_PARSE_ERROR 	  = 0x03,
	OTA_TWS_VER_ERROR		  = 0x04,
	OTA_TWS_DEVICE_INFO_ERROR = 0x05,
	OTA_TWS_CONFIG_ERROR	  = 0x06,
	OTA_TWS_FAIL			  = 0x07,
	OTA_TWS_CONTINUE		  = 0x80,
};

typedef struct
{
	u32_t fw_size;
	u32_t fw_ver;
	u32_t unique_id;
	u32_t checksum;
	u8_t tws_enable;
} ota_transmit_st;

u8_t ota_transmit_init(void **phdl, ota_transmit_st *ota_st, u32_t *offset);
u8_t ota_transmit_deinit(void **hdl);
u8_t ota_transmit_fw_check(void *hdl);
u8_t ota_transmit_data_process(void *hdl, u8_t* payload, int payload_len);
u32_t ota_transmit_version_get(void);
void ota_transmit_resource_release(u8_t tws);
u8_t  ap_ota_tws_access_receive(u8_t* buf, u16_t len);

#endif /* __AP_OTA_TRANSMIT_H__ */

