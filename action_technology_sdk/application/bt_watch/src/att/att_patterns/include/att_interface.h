#ifndef __ATT_INTERFACE_H
#define __ATT_INTERFACE_H

typedef struct
{
    uint8 filename[12];
    uint32 load_addr;
    uint32 offset;
    uint32 length;
    uint32 run_addr;
    uint32 checksum;
} atf_dir_t;


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

} att_interface_api_t;

typedef struct
{
    struct device *gpio_dev;
} att_interface_dev_t;


#define REBOOT_TYPE_NORMAL		0x0
#define REBOOT_TYPE_GOTO_ADFU		0x1000
#define REBOOT_TYPE_GOTO_BTSYS		0x1100
#define REBOOT_TYPE_GOTO_WIFISYS	0x1200
#define REBOOT_TYPE_GOTO_SYSTEM		0x1200
#define REBOOT_TYPE_GOTO_RECOVERY	0x1300
void sys_pm_reboot(int reboot_type);

//new

int read_atf_sub_file(u8_t *dst_addr, u32_t dst_buffer_len, const u8_t *sub_file_name, s32_t offset, s32_t read_len, atf_dir_t *sub_atf_dir);


#define app_mem_malloc malloc

#define app_mem_free   free

#endif
