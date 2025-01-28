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
#include <user_comm/sys_comm.h>
#include <crc.h>

#include "ap_ota_private.h"

static const char *ota_strs[] = {
	"RESERVED",
	"BOOT",
	"SYS_PARAM",
	"SYSTEM",
	"DATA",
};

static u16_t xml_str_generate(char *buf, const char *tag, const char *str)
{
	u16_t wr = 0;

	wr += sprintf(buf+wr, ".   <");
	wr += sprintf(buf+wr, "%s", tag);
	wr += sprintf(buf+wr, ">");
	
	wr += sprintf(buf+wr, "%s", str);

	wr += sprintf(buf+wr, "</");
	wr += sprintf(buf+wr, "%s", tag);
	wr += sprintf(buf+wr, ">");

	return wr;
}

static u16_t xml_int_generate(char *buf, const char *tag, int value)
{
	u16_t wr = 0;
	char temp_buf[12];

	wr += sprintf(buf+wr, ".   <");
	wr += sprintf(buf+wr, "%s", tag);
	wr += sprintf(buf+wr, ">");

	memset(temp_buf, 0, sizeof(temp_buf));
	//SYS_LOG_INF("value %x",value);
	myitoa(value, temp_buf,16);
	//SYS_LOG_INF("temp_buf %s",temp_buf);
	wr += sprintf(buf+wr, "0x");
	wr += sprintf(buf+wr, "%s", temp_buf);

	wr += sprintf(buf+wr, "</");
	wr += sprintf(buf+wr, "%s", tag);
	wr += sprintf(buf+wr, ">");

	return wr;
}


u16_t ota_ap_head_splice(ota_rs_breakponit_t *breakpoint, struct ota_fw_head *head)
{
	memset(head, 0, sizeof(struct ota_fw_head));
	struct ota_upgrade_info *ota_info = ota_app_info_get();
	memcpy(&head->hdr, &ota_info->img->hdr, sizeof(struct ota_fw_hdr));
	memcpy(&head->dir, &ota_info->img->dir, sizeof(struct ota_fw_dir));

#if 0
	head->hdr.magic = OTA_FW_HDR_MAGIC;
	head->hdr.header_checksum = 0;
	head->hdr.header_version = 0x0100;
	head->hdr.header_size = sizeof(struct ota_fw_head);

	for (i = 0; i < OTA_BIN_VALID_NUM_MAX; i++)
	{
		if (breakpoint->entry[i].checksum)
			head->hdr.file_cnt ++;
	}
	head->hdr.file_cnt ++;
	head->hdr.flag = 0;
	head->hdr.dir_offset = 0x200;
	head->hdr.data_offset = sizeof(struct ota_fw_head);
	head->hdr.data_size = 0;
	head->hdr.data_checksum = 0;
	
	for (i = 1; i < head->hdr.file_cnt; i++)
	{
		strncpy(head->dir.entrys[i].filename, 
			breakpoint->entry[i].filename, sizeof(breakpoint->entry[i].filename));
		head->dir.entrys[i].offset = breakpoint->entry[i].offset;
		head->dir.entrys[i].length = breakpoint->entry[i].length;
		head->dir.entrys[i].checksum = breakpoint->entry[i].checksum;
	}
#endif
	return sizeof(struct ota_fw_head);
}

u16_t ota_ap_xml_splice(ota_rs_breakponit_t *breakpoint, char *xml_buf)
{
	u16_t wr = 0;
	int i;
	char *type_str;
	u8_t file_id;

	struct ota_upgrade_info *ota_info = ota_app_info_get();
	const struct fw_version *cur_ver = fw_version_get_current();

	wr += sprintf(xml_buf+wr, "<?xml ");

	wr += sprintf(xml_buf+wr, "<ota_firmware>");

	wr += sprintf(xml_buf+wr, ".   <firmware_version>");

	wr += xml_int_generate(xml_buf+wr, "version_code" , breakpoint->image_bk.new_version);
	wr += xml_str_generate(xml_buf+wr, "version_name" ,"1.00_2111xxxx");
	wr += xml_str_generate(xml_buf+wr, "board_name" ,cur_ver->board_name);
	wr += sprintf(xml_buf+wr, ".   </firmware_version>");

	wr += sprintf(xml_buf+wr, ".   <partitions>");

	if ((ota_info->img->hdr.file_cnt > 4) || (ota_info->img->hdr.file_cnt < 2))
		return 0;

	wr += xml_int_generate(xml_buf+wr, "partitionsNum" ,ota_info->img->hdr.file_cnt-1);
	for (i = 1; i < ota_info->img->hdr.file_cnt;  i++) {
		wr += sprintf(xml_buf+wr, ".   <partition>");
		if (0 == memcmp(ota_info->img->dir.entrys[i].filename, "mbrec.bin", 9))
		{
			type_str = (char *)ota_strs[1];
			file_id = 1;
		}
		else if (0 == memcmp(ota_info->img->dir.entrys[i].filename, "param.bin", 9))
		{
			type_str = (char *)ota_strs[2];
			file_id = 2;
		}
		else
		{
			type_str = (char *)ota_strs[3];
			file_id = 4;
		}
		//SYS_LOG_INF("checksum %x",ota_info->img->dir.entrys[i].checksum);
		//SYS_LOG_INF("length %x",ota_info->img->dir.entrys[i].length);
		wr += xml_str_generate(xml_buf+wr, "type" ,type_str);
		wr += xml_int_generate(xml_buf+wr, "file_id" ,file_id/*breakpoint->image_bk.file_state[i-1].file_id*/);
		wr += xml_str_generate(xml_buf+wr, "file_name" ,ota_info->img->dir.entrys[i].filename);
		wr += xml_int_generate(xml_buf+wr, "file_size" ,ota_info->img->dir.entrys[i].length);
		wr += xml_int_generate(xml_buf+wr, "checksum" ,ota_info->img->dir.entrys[i].checksum);

		wr += sprintf(xml_buf+wr, ".   </partition>");
	}

	wr += sprintf(xml_buf+wr, ".   </partitions>");

	wr += sprintf(xml_buf+wr, ".</ota_firmware>");


	return wr;
}

void ota_ap_head_perfect(struct ota_fw_head *head, char *xml_buf, u16_t xml_len)
{
	u32_t crc;

	crc = utils_crc32(0, xml_buf, xml_len);

	strncpy(head->dir.entrys[0].filename, "ota.xml", 7);
	head->dir.entrys[0].offset = sizeof(struct ota_fw_head);
	head->dir.entrys[0].length = xml_len;
	head->dir.entrys[0].checksum = crc;

	crc = utils_crc32(0, (const char *)head + 8, sizeof(struct ota_fw_head) - 8);
	head->hdr.header_checksum = crc;
}

u16_t ota_ap_head_xml_get(ota_rs_breakponit_t *breakpoint, u8_t *ptr)
{
	//u8_t *ptr = app_mem_malloc(sizeof(struct ota_fw_head) + 1536);

	struct ota_fw_head *head = (struct ota_fw_head *)ptr;
	u8_t *xml_buf = ptr + sizeof(struct ota_fw_head);
	u16_t xml_len;

	ota_ap_head_splice(breakpoint, head);
	xml_len = ota_ap_xml_splice(breakpoint, xml_buf);
	ota_ap_head_perfect(head, xml_buf, xml_len);

	return (sizeof(struct ota_fw_head) + xml_len);
}
