#ifndef __ATT_PATTERN_HEADER_H
#define __ATT_PATTERN_HEADER_H

//ATT test item timeout value, unit:s
#define  ATT_ITEM_TEST_TIMEOUT      60

#define STUB_ATT_RW_TEMP_BUFFER		((u8_t *)0x60000)

#define STUB_ATT_RW_TEMP_BUFFER_LEN (256)

#define STUB_ATT_RETURN_DATA_BUFFER (STUB_ATT_RW_TEMP_BUFFER + STUB_ATT_RW_TEMP_BUFFER_LEN)

#define STUB_ATT_RETURN_DATA_LEN    (256)


typedef enum
{
    //common test id,0x01-0xbf
    TESTID_MODIFY_BTNAME            = 0x01,
	TESTID_MODIFY_BLENAME           = 0x02,
    TESTID_FM_TEST                  = 0x03,
    TESTID_GPIO_TEST                = 0x04,
    TESTID_MIC_CH_TEST              = 0x05,
    TESTID_FM_CH_TEST               = 0x06,
    TESTID_LINEIN_CH_TEST           = 0x07,
    TESTID_LINEIN_TEST              = 0x08,
    TESTID_SDCARD_TEST              = 0x0a,
    TESTID_UHOST_TEST               = 0x0b,
   
    TESTID_PREPARE_PRODUCT_TEST     = 0x0d,
    TESTID_PRODUCT_TEST             = 0x0e,

    TESTID_FLASHDUMP_TEST           = 0x0f,
    TESTID_BURN_IC                  = 0x10,
    TESTID_CHECK_IC                 = 0x11,
    TESTID_FLASHDOWNLOAD_TEST       = 0x12,
    TESTID_MEM_UPLOAD_TEST          = 0x13,
    TESTID_MEM_DOWNLOAD_TEST        = 0x14,

    TESTID_READ_BTNAME              = 0x16,
    TESTID_READ_BLENAME             = 0x17,
    TESTID_FTMODE                   = 0x18,
    TESTID_BQBMODE                  = 0x19,
    TESTID_MP_CFO                   = 0x1a,
    TESTID_MP_CFO_READ              = 0x1b,
    TESTID_PWR_TEST                 = 0x1c,
    TESTID_READ_FW_VERSION          = 0x20,   
    TESTID_FLASHTEST                = 0x21,
    TESTID_LRADC_TEST               = 0x22,
    TESTID_UUID_READ         		= 0x24,

    TESTID_LEFT_MIC_VERIFY          = 0x30,
    TESTID_RIGHT_MIC_VERIFY         = 0x31,

    // special ID, MUST not modify
    TESTID_MODIFY_BTADDR            = 0xc0,
    TESTID_BT_TEST                  = 0xc1,
    TESTID_READ_BTADDR              = 0xc4,
    //donot modify this testcase id
    TESTID_SERIAL_NUMBER_WRITE      = 0x0c,
    TESTID_PRODUCT_INFO_WRITE       = 0xc8,
    TESTID_SERIAL_NUMBER_READ       = 0xc9,

    //the dut needs to wait for the PC to reply to a valid TESTID
    TESTID_ID_WAIT                  = 0xfffe,

    //the dut can end testcases
    TESTID_ID_QUIT                  = 0xffff,
}ATT_TEST_ID_e;


#define BT_DEV_ADDR_LEN             (6)
#define MAC_STR_LEN		            (12+1)	/* 12(len) + 1(NULL) */

#define CFG_MAX_BT_DEV_NAME_LEN     (32)
#define BT_NAME_STR_LEN		        (CFG_MAX_BT_DEV_NAME_LEN+1)	/* 32(len) + 1(NULL) */

#define CFG_MAX_SN_NAME_LEN         (32)
#define SN_STR_LEN                  (32+1)

typedef enum
{
    TEST_MODE_CARD = 0,
    TEST_MODE_USB,
    TEST_MODE_UART,
    TEST_MODE_EXTEND
}test_mode_e;


typedef enum
{
    TEST_PASS = 0,
    TEST_FAIL,
    TEST_BT_FAIL,
    TEST_MIC_FAIL,
    TEST_FW_ERROR,
    TEST_SN_ERROR,
}test_result_e;


typedef enum
{
    //DUT直接和PC连接
    DUT_CONNECT_MODE_DIRECT = 0,
    //DUT通过UDA转接板和PC连接
    DUT_CONNECT_MODE_UDA,
}dut_connect_state_e;


typedef enum
{
    DUT_STATE_IDLE = 0,
    DUT_STATE_BUSY,
    DUT_STATE_BT_CONNECTING,
    DUT_STATE_BT_PLAYING,
}ATT_DUT_STATE_e;


typedef struct
{
    u16_t offsetl;
    u16_t offseth;
    u16_t lengthl;
    u16_t lengthh;
}att_fread_arg_t;





typedef struct
{
    u8_t magic[8];
    u8_t sdk_version[4];
    u8_t reserved[20];
    atf_dir_t atf_dir[15];
} atf_head_t;


typedef test_result_e (*test_func_t)(void *arg, u32_t arg_len);

typedef s32_t (*read_arg_func_t)(u16_t *line_buffer, u8_t *arg_buffer, u32_t arg_len);

typedef struct
{
    u32_t test_id;
    test_func_t test_func;
}att_task_stru_t;


typedef struct
{
    u8_t bt_transmitter_addr[6];
    u8_t bt_test_mode;
}btplay_test_arg_t;


typedef struct
{
    u32_t  test_result;
    u8_t   return_arg[0];
}return_result_t;


typedef struct
{
    u32_t id;                  ///< ATT test ID, \ref in ATT_TEST_ID_e
    u32_t para_length;         ///< test parameter length, byte unit
}att_test_item_para;


// string coding format
typedef enum
{
    STRING_CODING_UTF_8 = 0,    ///< utf-8 coding
    STRING_CODING_UTF_16,       ///< utf-16 coding
}att_string_coding_e;


// ATT测试项参数
typedef struct
{
    u32_t  para_len;           ///< 参数长度
    u8_t   para[0];            ///< 参数数据, word size对齐
}test_para_t;

typedef struct
{
    u32_t  para_num;           ///< 全部参数数目
    test_para_t para_item[0];
}att_test_para_t;

// output arguments structure
typedef struct
{
    u32_t  arg_length;         ///< 输出参数长度
    u8_t*  arg;                ///< 输出参数buffer地址
}output_arg_t;


struct att_env_var {
	u8_t test_mode;
	u8_t  *arg_buffer;
    u16_t test_id;
    u16_t arg_len;
	att_interface_api_t *api;
	att_interface_dev_t *dev_tbl;
};

extern s32_t bt_log;

extern struct att_env_var *self;

s32_t act_test_read_arg(att_test_para_t *input_arg, output_arg_t *output_arg, u32_t output_arg_num);

int act_test_normal_report(u32_t test_id, test_result_e result);

int act_test_report_result(u32_t test_id, u8_t *result_data, u32_t result_len);

int act_log_to_pc(u8_t *str, u32_t len);

void wait_until_stub_idle(void);

int act_test_set_bt_name(char *name_str, uint32_t name_len);

int act_test_set_bt_blename(char *name_str, uint32_t name_len);

int act_test_set_bt_addr(uint8_t *mac_value, uint32_t mac_len);

int act_test_set_bt_bqbflag(uint8_t flag);

int act_test_get_bt_name(char *name_str, uint32_t name_len);

int act_test_get_bt_blename(char *name_str, uint32_t name_len);

int act_test_get_bt_addr(uint8_t *mac_value, uint32_t mac_len);

int act_test_set_sn_info(char *sn_str, uint32_t sn_str_len);

int act_test_get_sn_info(char *sn_str, uint32_t sn_str_len);

int att_buf_printf(const char * format, ...);

#endif
