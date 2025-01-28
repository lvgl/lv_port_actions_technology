#ifndef __TOOL_ATT_H
#define __TOOL_ATT_H

#define ATF_MAX_SUB_FILENAME_LENGTH  12

// string coding format
typedef enum
{
    STRING_CODING_UTF_8 = 0,    ///< utf-8 coding
    STRING_CODING_UTF_16,       ///< utf-16 coding
}att_string_coding_e;

typedef struct
{
    uint8_t filename[12];
    uint32_t load_addr;
    uint32_t offset;
    uint32_t length;
    uint32_t run_addr;
    uint32_t checksum;
}atf_dir_t;

//atf file read data request packet
typedef struct
{
    uint32_t offset;
    uint32_t readLen;
} __packed atf_file_read_t;

//atf file read data reply packet
typedef struct
{
    uint8_t payload[0];
} __packed atf_file_read_ack_t;

typedef struct
{
    uint32_t version;
    uint8_t compiler_type;
    uint8_t reserved[3];
	int (*property_get)(const char *key, char *value, int value_len);
	int (*property_set)(const char *key, char *value, int value_len);
	int (*property_set_factory)(const char *key, char *value, int value_len);

    int (*stub_write_packet)(uint16_t opcode, uint32_t op_para, uint8_t *data_buffer, uint32_t data_len);
    int (*stub_read_packet)(uint16_t opcode, uint32_t op_para, uint8_t *data_buffer, uint32_t data_len);
    int (*stub_status_inquiry)(void);

	int (*read_atf_sub_file)(u8_t *dst_addr, u32_t dst_buffer_len, const u8_t *sub_file_name, \
		s32_t offset, s32_t read_len, atf_dir_t *sub_atf_dir);

    void *(*malloc)(size_t size);
    void (*free)(void *ptr);
    void (*k_sleep)(s32_t duration);
	u32_t (*k_uptime_get)(void);
	void (*udelay)(u32_t val);
    void (*vprintk)(const char *fmt, va_list args);
}att_interface_api_t;

enum
{
    DB_DATA_BT_ADDR,
    DB_DATA_BT_NAME,
    DB_DATA_BT_BLE_NAME,
    DB_DATA_BT_PLIST,
    DB_DATA_BT_RF_BQB_FLAG,
};

#endif
