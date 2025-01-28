#ifndef __ACT_LOG_INNER_H
#define __ACT_LOG_INNER_H

#include <os_common_api.h>
#include <logging/act_log.h>
#include <sys/ring_buffer.h>
#include <linker/linker-defs.h>
#include "act_log_flash_buffer.h"

#define ACTLOG_THREAD_STACK_SIZE  (2048)
typedef enum
{
    ACTLOG_MSG_LOG = 1,
    ACTLOG_MSG_HEXDUMP,
    ACTLOG_MSG_NANO_LOG,
    ACTLOG_MAX_MSG_TYPE,
} actlog_message_type_e;

typedef struct {
    /** FIFO uses first word itself, reserve space */
    //intptr_t _unused;
    uint8_t id;
    uint8_t level:4;
    uint8_t type:4;
    uint16_t line_number;
    uint32_t timestamp;
#ifdef CONFIG_ACTLOG_SHOW_FUNCTION
    const char *func_name;
#endif
    uint32_t arg_num;
}__packed log_message_head_t;

typedef struct
{
    uint8_t id;
    uint8_t level :4;
    uint8_t type :4;
    uint16_t line_number;
    uint32_t timestamp;
#ifdef CONFIG_ACTLOG_SHOW_FUNCTION
    const char *func_name;
#endif
    uint32_t str_len;
    char fmt_data[CONFIG_ACTLOG_FMT_DATA_SIZE + 1];
} normal_log_message_t;

typedef struct
{
    uint8_t id;
    uint8_t level :4;
    uint8_t type :4;
    uint16_t line_number;
    uint32_t timestamp;
#ifdef CONFIG_ACTLOG_SHOW_FUNCTION
    const char *func_name;
#endif
    char data[CONFIG_ACTLOG_FMT_DATA_SIZE];
    uint16_t data_len;
    uint8_t data_width;
    uint8_t line_width;
    uint32_t disp_addr;
} hexdump_log_message_t;

typedef struct
{
    uint8_t id;
    uint8_t level :4;
    uint8_t type :4;
    uint16_t line_number;
    uint32_t timestamp;
#ifdef CONFIG_ACTLOG_SHOW_FUNCTION
    const char *func_name;
#endif
    uint32_t arg_num;
    const char *fmt;
    uint32_t arg_value[MAX_NANO_ARG_NUM];
} nano_log_message_t;

typedef union
{
    normal_log_message_t normal;
    hexdump_log_message_t hex;
    nano_log_message_t nano;
} log_message_t;

typedef struct
{
    uint8_t id;
    uint8_t enable;
    uint8_t max_line_num;
    uint8_t line_num;
    uint16_t base_time;
    uint16_t limit_time;
} act_log_num_filter_t;

typedef struct
{
    uint8_t init_flag :1;
    uint8_t task_ready :1;
    uint8_t timestamp_enable :1;
    uint8_t level :3;
    uint8_t panic :1;
    uint8_t module_cnt;
    uint8_t output_mode;
    uint32_t thread_id;
    uint32_t drop_cnt;
    uint32_t irq_cnt;
    uint32_t err_cnt;
    struct ring_buf rbuf;
    uint8_t cache_buffer[4096];
    os_sem log_sem;
    uint8_t *module_level;
    act_log_num_filter_t filter;
} actlog_ctrl_t;

extern actlog_ctrl_t actlog_ctrl;

static inline uint32_t act_log_compress_const_data(uint32_t data_addr)
{
    if (data_addr >= (uint32_t) __rom_region_start && data_addr <= (uint32_t) __rom_region_end) {
        return (data_addr - (uint32_t) __rom_region_start);
    } else {
        return 0;
    }
}

static inline uint32_t act_log_decompress_const_data(uint32_t data_addr)
{
    if (data_addr != 0) {
        return (data_addr + (uint32_t) __rom_region_start);
    } else {
        return 0;
    }
}

static inline const char *actlog_source_name_get(uint8_t src_id)
{
    return src_id < log_sources_count() ? log_name_get(src_id) : NULL;
}

static inline int actlog_module_num_get(void)
{
    return log_sources_count();
}

static inline uint8_t actlog_compiled_level_get(uint8_t src_id)
{
    return log_compiled_level_get(src_id);
}

static inline void actlog_dynamic_level_set(actlog_ctrl_t * ctrl, uint8_t src_id, uint8_t level)
{
    uint8_t level_map = ctrl->module_level[src_id >> 1];

    if (src_id & 0x1) {
        level_map = (level_map & 0xf) | (level << 4);
    } else {
        level_map = (level_map & 0xf0) | level;
    }

    ctrl->module_level[src_id >> 1] = level_map;
}

static inline int actlog_dynamic_level_get(actlog_ctrl_t * ctrl, uint8_t src_id)
{
    uint8_t level_map = ctrl->module_level[src_id >> 1];

    if (src_id & 0x1) {
        return (level_map >> 4);
    } else {
        return (level_map & 0xf);
    }
}

// memory api
void *log_memory_alloc(uint32_t size);

void log_memory_free(void *ptr);

void log_memory_dump_info(uint32_t index);

//output function
int act_log_output_flow_init(void);

int act_log_output_flow_write(log_message_t *log_msg, uint8_t *data, uint32_t len);

int act_log_output_binary_init(void);

int act_log_output_binary_write(log_message_t *log_msg, uint8_t *data, uint32_t len);

int act_log_output_binary_get_used_size(int file_id);

int act_log_output_binary_traverse(int file_id, int (*traverse_cb)(uint8_t *data, uint32_t max_len), uint8_t *buf,
        uint32_t len);

int act_log_output_binary_flush(void);

int act_log_output_binary_clear(int file_id);

int act_log_output_textfile_init(void);

int act_log_output_textfile_write(log_message_t *log_msg, uint8_t *data, uint32_t len);

int act_log_output_textfile_get_used_size(int file_id);

int act_log_output_textfile_traverse(int file_id, int (*traverse_cb)(uint8_t *data, uint32_t max_len), uint8_t *buf,
        uint32_t len);

int act_log_output_textfile_flush(void);

int act_log_output_textfile_clear(int file_id);

//output backend
int act_log_backend_init(void);

int act_log_backend_register(uint8_t mode, actlog_backend_callback_t callback, void *user_data);

int act_log_backend_unregister(uint8_t mode);

int act_log_backend_output(log_message_t *log_msg, uint8_t *data, uint32_t len);

int act_log_backend_get_used_size(int file_id);

int act_log_backend_flush(void);

int act_log_backend_traverse(int file_id, int (*traverse_cb)(uint8_t *data, uint32_t max_len), uint8_t *buf,
        uint32_t len);

int act_log_backend_clear(int file_id);

//actlog
uint32_t process_log_linebuf(log_message_t *log_msg, char *log_buffer, uint32_t buffer_size);

int act_log_is_const_ptr(const char *fmt);

void log_test(void);

#if IS_ENABLED(CONFIG_SIM_FLASH_ACTS)
void check_save_log_to_flash(void);
#endif

int actlog_set_level_filter(const char *module_name, uint32_t dynamic_level);

int actlog_set_module_num_filter(const char *module_name, uint8_t enable, uint8_t max_log_num, uint16_t limit_time);

void actlog_dump_runtime_info(void);

void actlog_print_buffer(const uint8_t *addr, int width, int count, int linelen, unsigned long disp_addr);

size_t actlog_strnlen(const char *s, size_t maxlen);

#endif
