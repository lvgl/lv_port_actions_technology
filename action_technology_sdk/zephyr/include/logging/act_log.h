#ifndef __ACT_LOG_H
#define __ACT_LOG_H

#define MAX_MODULE_NAME_LEN (16)
#define MAX_NANO_ARG_NUM        (7)
#define INVALID_NANO_ARG_NUM    (15)

extern const char * const nano_args;
#define FUN_ARG_0 (&nano_args[0])
#define FUN_ARG_1 (&nano_args[1])
#define FUN_ARG_2 (&nano_args[2])
#define FUN_ARG_3 (&nano_args[3])
#define FUN_ARG_4 (&nano_args[4])
#define FUN_ARG_5 (&nano_args[5])
#define FUN_ARG_6 (&nano_args[6])
#define FUN_ARG_7 (&nano_args[7])
#define FUN_ARG_8 (&nano_args[8])
#define FUN_ARG_9 (&nano_args[9])
#define FUN_ARG_10 (&nano_args[10])
#define FUN_ARG_11 (&nano_args[11])
#define FUN_ARG_12 (&nano_args[12])
#define FUN_ARG_I (&nano_args[13])
#define FUN_ARG_EX(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, N, ...) N
#define FUN_ARG_EX_() FUN_ARG_12, FUN_ARG_11, FUN_ARG_10, FUN_ARG_9, \
						FUN_ARG_8, FUN_ARG_7, FUN_ARG_6, FUN_ARG_5, \
						FUN_ARG_4, FUN_ARG_3, FUN_ARG_2, FUN_ARG_1, FUN_ARG_0
#define ARGS_CHECK(N) (true ? N : FUN_ARG_I)
#define FUN_ARG_NUM_(...) FUN_ARG_EX(__VA_ARGS__)
#define FUN_ARG_NUM(...) ARGS_CHECK(FUN_ARG_NUM_(FUN_ARG_I, ##__VA_ARGS__, FUN_ARG_EX_()))
#define FUN_ARG_NUM_INTEGER(a) (((a) >= 0 && (a) <= MAX_NANO_ARG_NUM) ? (a) : INVALID_NANO_ARG_NUM)

#define  ACTLOG_LEVEL_NONE    0
#define  ACTLOG_LEVEL_ERROR   1
#define  ACTLOG_LEVEL_WARN    2
#define  ACTLOG_LEVEL_INFO    3
#define  ACTLOG_LEVEL_DEBUG   4
#define  ACTLOG_LEVEL_MAX     5

#define  ACTLOG_FILE_TYPE_LOG       	0
#define  ACTLOG_FILE_TYPE_RUNTIME_LOG   1
#define  ACTLOG_FILE_TYPE_EVENT     	2
#define  ACTLOG_FILE_TYPE_LOG_SAVE  	3


typedef enum{
    ACTLOG_OUTPUT_MODE_DIRECT   = (1 << 0),
    ACTLOG_OUTPUT_MODE_FLOW     = (1 << 1),
    ACTLOG_OUTPUT_MODE_BINARY   = (1 << 2),
    ACTLOG_OUTPUT_MODE_TEXT     = (1 << 3),
    ACTLOG_OUTPUT_MODE_USER     = (1 << 4),
}act_log_output_mode_e;

#define ACTLOG_PACK(id, level, line, nums) \
    ((((line) & 0xffff) << 16) \
    | (((id) & 0xff) << 8) \
    | ((level & 0x7) << 4) \
    | (nums & 0xf))


typedef union{
    uint32_t data;
    struct{
        uint8_t arg_num: 4;
        uint8_t level: 3;
        uint8_t reserved : 1;
        uint8_t  id;
        uint16_t line;
    }bit_data;
}act_log_pack_data;

typedef void (* actlog_backend_callback_t)(const uint8_t *data, uint32_t len, void *user_data);

int act_log_init(void);

/*for bt transfer*/
#define LOG_TYPE_COREDUMP   0
#define LOG_TYPE_SYSLOG		1
#define LOG_TYPE_RUN_LOG	2
#define LOG_TYPE_RAMDUMP	3

#ifdef CONFIG_ACTLOG
extern int actlog_syslog_transfer(int log_type, int (*traverse_cb)(uint8_t *data, uint32_t max_len));
#endif
#ifdef CONFIG_DEBUG_COREDUMP_BACKEND_FLASH_DEV
int coredump_transfer(int (*traverse_cb)(uint8_t *data, uint32_t max_len));
#endif
#ifdef CONFIG_DEBUG_RAMDUMP
int ramdump_transfer(int (*traverse_cb)(uint8_t *data, uint32_t max_len));
#endif
static inline int actlog_bt_transfer(int log_type, int (*traverse_cb)(uint8_t *data, uint32_t max_len))
{
#ifdef CONFIG_ACTLOG
	if(log_type == LOG_TYPE_SYSLOG || log_type == LOG_TYPE_RUN_LOG)
		return actlog_syslog_transfer(log_type, traverse_cb);
#endif
#ifdef CONFIG_DEBUG_COREDUMP_BACKEND_FLASH_DEV
if(log_type == LOG_TYPE_COREDUMP)
	return coredump_transfer(traverse_cb);
#endif
#ifdef CONFIG_DEBUG_RAMDUMP
if(log_type == LOG_TYPE_RAMDUMP)
	return ramdump_transfer(traverse_cb);
#endif

	return 0;
}


/**
 * @brief register output backend
 *
 * @param mode backend output mode, see @act_log_output_mode_e
 * @param callback callback function for output backend
 * @param user_data user-defined data
 *
 * @retval 0 on success else negative code.
 */
int act_log_register_output_backend(uint8_t mode, actlog_backend_callback_t callback, void *user_data);

/**
 * @brief unregister output backend
 *
 * @param mode backend output mode, see @act_log_output_mode_e
 *
 * @retval 0 on success else negative code.
 */
int act_log_unregister_output_backend(uint8_t mode);

/* The log process thread has the K_LOWEST_APPLICATION_THREAD_PRIO, adjust it
 * to a higher priority to increase the chances of being scheduled to handle
 * log message as soon as possible
 */
void actlog_increase_log_thread_priority(void);


/* The log process thread has the K_LOWEST_APPLICATION_THREAD_PRIO, recovery it
 * from a higher priority to normal priority 
 */
void actlog_decrease_log_thread_priority(void);

#ifdef CONFIG_ACTLOG_SHOW_FUNCTION
#define ACTLOG_FUNCTION __FUNCTION__
#else
#define ACTLOG_FUNCTION NULL
#endif

#ifdef CONFIG_ACTLOG_SHOW_FUNCTION
void actlog_printk(uint32_t pack_data, const char *func, const char *fmt, ...);

void actlog_printk_nano(uint32_t pack_data, const char *func, const char *fmt, ...);

#define NORMAL_PRINTK(pack_data, fmt, ...) actlog_printk(pack_data, ACTLOG_FUNCTION, fmt, ##__VA_ARGS__)
#define NANO_PRINTK(pack_data, fmt, ...) actlog_printk_nano(pack_data, ACTLOG_FUNCTION, fmt, ##__VA_ARGS__)

#else
void actlog_printk(uint32_t pack_data, const char *fmt, ...);

void actlog_printk_nano(uint32_t pack_data, const char *fmt, ...);

#define NORMAL_PRINTK(pack_data, fmt, ...) actlog_printk(pack_data, fmt, ##__VA_ARGS__)
#define NANO_PRINTK(pack_data, fmt, ...) actlog_printk_nano(pack_data, fmt,  ##__VA_ARGS__)

#endif

#ifndef CONFIG_ACTLOG_USE_NANOLOG
#define ACT_LOG_PRINT(pack_data, fmt, ...) NORMAL_PRINTK(pack_data, fmt, ##__VA_ARGS__)
#else
#define ACT_LOG_PRINT(pack_data, fmt, ...) NANO_PRINTK(pack_data, fmt, ##__VA_ARGS__)
#endif

#define ACTLOG_PACK_NANO_DATA(id, level, fmt, ...) ACTLOG_PACK(id, level, __LINE__, (FUN_ARG_NUM_INTEGER(FUN_ARG_NUM(__VA_ARGS__) - nano_args)))
#define ACTLOG_ID_GET()  LOG_CONST_ID_GET(__log_current_const_data)

#ifdef CONFIG_ACTLOG_COMPILE_MODUL_LEVEL
#define ACTLOG_CONST_LEVEL_CHECK(_level)	((_level <= __log_level) &&	(_level <= CONFIG_ACTLOG_COMPILE_LEVEL))
#else
#define ACTLOG_CONST_LEVEL_CHECK(_level)	((_level <= CONFIG_ACTLOG_COMPILE_LEVEL))

#endif

#define ACT_LOG(_level, fmt, ...) do { \
			if (!ACTLOG_CONST_LEVEL_CHECK(_level)) { \
				break; \
			} \
			ACT_LOG_PRINT(ACTLOG_PACK_NANO_DATA(ACTLOG_ID_GET(), _level, fmt, __VA_ARGS__), fmt, ##__VA_ARGS__); \
		} while (false)

#endif
