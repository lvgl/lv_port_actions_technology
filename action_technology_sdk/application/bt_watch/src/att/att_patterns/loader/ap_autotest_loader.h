#ifndef __AP_AUTOTEST_LOADER_H
#define __AP_AUTOTEST_LOADER_H

#define ATF_MAX_SUB_FILENAME_LENGTH  12

typedef struct
{
    uint8  test_state;
    uint8 reserved;
    uint16 test_id;
    uint16 arg_len;
    u32_t run_addr;
    char last_pt_name[13]; //8+1+3+'\0'
}autotest_test_info_t;

struct att_code_info {
    unsigned char testid;
    unsigned char type;
    unsigned char need_drv;
    const char    pt_name[13]; //8+1+3+'\0'
};

struct att_drv_code_info {
    const char    pt_name[13]; //8+1+3+'\0'
    const char    drv_name[13]; //8+1+3+'\0'
};

extern autotest_test_info_t g_test_info;
extern struct att_env_var g_att_env_var;

extern void test_dispatch(void);

#endif
