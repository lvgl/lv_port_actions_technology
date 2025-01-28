
#ifndef __OTA_MW_H__
#define __OTA_MW_H__

#include <stdint.h>

enum OTA_RTN_VAL
{
	OTA_MW_SUCCESS			 = 0x00,
	OTA_MW_PARAM_INVALID	 = 0x01,
	OTA_MW_MEMORY_ERROR 	 = 0x02,
	OTA_MW_PARSE_ERROR		 = 0x03,
	OTA_MW_VER_ERROR		 = 0x04,
	OTA_MW_DEVICE_INFO_ERROR = 0x05,
	OTA_MW_CONFIG_ERROR 	 = 0x06,
	OTA_MW_FAIL 			 = 0x07,
	OTA_MW_CONTINUE 		 = 0x80,
};

u8_t ota_resource_init(void **phdl, u32_t fw_size, u32_t fw_ver, void *param);
u8_t ota_resource_start(void *hdl, u32_t offset, void *ptr);
u8_t ota_resource_deinit(void **hdl, void *param);
u8_t ota_fw_check(void *hdl);
u8_t ota_data_process(void *hdl, u8_t* payload, int payload_len);
#endif /* __OTA_MW_H__ */

