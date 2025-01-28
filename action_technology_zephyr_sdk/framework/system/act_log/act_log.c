#include <init.h>
#include "act_log_inner.h"
#include <linker/linker-defs.h>
#include <debug/actions_exception.h>
#include <drivers/flash.h>
#include <board_cfg.h>

actlog_ctrl_t actlog_ctrl;

static OS_THREAD_STACK_DEFINE(actlog_thread_stack, ACTLOG_THREAD_STACK_SIZE);

static uint8_t log_line_buf[CONFIG_ACTLOG_LINEBUF_SIZE];

const char * const nano_args = "0123456789abcI";

static const char act_loglevel_info[] =
{
    'N',    //NONE
    'E',    //Error
    'W',    //Warn
    'I',    //Info
    'D',    //Debug
};



static int actlog_module_id_get(const char *name)
{
	uint32_t modules_cnt = actlog_module_num_get();
	const char *tmp_name;
	uint32_t i;

	for (i = 0U; i < modules_cnt; i++) {
		tmp_name = actlog_source_name_get(i);

		if (tmp_name && strncmp(tmp_name, name, 64) == 0) {
			return i;
		}
	}
	return -1;
}

static void act_log_module_level_init(actlog_ctrl_t *ctrl)
{
	uint32_t i;
	uint8_t compile_level;

	uint32_t modules_cnt = actlog_module_num_get();

	uint32_t level_bytes = ROUND_UP(modules_cnt, 2) >> 1;

	ctrl->module_level = (uint8_t *)mem_malloc(level_bytes);

	if(!ctrl->module_level){
		return;
	}

	memset(ctrl->module_level, 0, level_bytes);

	for (i = 0; i < modules_cnt; i++) {
		compile_level = actlog_compiled_level_get(i);
		actlog_dynamic_level_set(ctrl, i, compile_level);
	}

}

static int act_log_get_output_mode(void)
{
    return actlog_ctrl.output_mode;
}

static uint32_t log_hex_dump_data(const char *data, int length, char *log_buffer, uint32_t buffer_size)
{
    int index = 0;
    uint32_t curr_index;

    curr_index = 0;
    for (index = 0; index < length; index++) {
        curr_index += snprintk(&log_buffer[curr_index], (buffer_size - curr_index), "%02X", (int)(data[index]));
        if ((index + 1) % 16 == 0) {
            curr_index += snprintk(&log_buffer[curr_index], (buffer_size - curr_index), "\n");
            continue;
        }
        if (index + 1 != length) {
            curr_index += snprintk(&log_buffer[curr_index], (buffer_size - curr_index), " ");
        }
    }
    if (0 != index && 0 != index % 16) {
        curr_index += snprintk(&log_buffer[curr_index], (buffer_size - curr_index), "\n");
    }

    return curr_index;
}

static uint32_t  log_nano_dump_data(const char *fmt, int arg_num, uint32_t *arg_value, char *log_buffer, uint32_t buffer_size)
{
    int32_t index = 0;

    switch (arg_num) {
        case 0:
            index = strlen(fmt);
            if (index > buffer_size) {
                index = buffer_size;
            }
            strncpy(log_buffer, fmt, index);
            break;
        case 1:
            index = snprintk(log_buffer, buffer_size, fmt, arg_value[0]);
            break;
        case 2:
            index = snprintk(log_buffer, buffer_size, fmt, arg_value[0], arg_value[1]);
            break;
        case 3:
            index = snprintk(log_buffer, buffer_size, fmt, arg_value[0], arg_value[1], arg_value[2]);
            break;
        case 4:
            index = snprintk(log_buffer, buffer_size, fmt, arg_value[0], arg_value[1], arg_value[2], arg_value[3]);
            break;
        case 5:
            index = snprintk(log_buffer, buffer_size, fmt, arg_value[0], arg_value[1], arg_value[2], arg_value[3], arg_value[4]);
            break;
        case 6:
            index = snprintk(log_buffer, buffer_size, fmt, arg_value[0], arg_value[1], arg_value[2], arg_value[3], arg_value[4], \
                arg_value[5]);
        case 7:
            index = snprintk(log_buffer, buffer_size, fmt, arg_value[0], arg_value[1], arg_value[2], arg_value[3], arg_value[4], \
                arg_value[5], arg_value[6]);
        case 8:
            index = snprintk(log_buffer, buffer_size, fmt, arg_value[0], arg_value[1], arg_value[2], arg_value[3], arg_value[4], \
                arg_value[5], arg_value[6], arg_value[7]);
        case 9:
            index = snprintk(log_buffer, buffer_size, fmt, arg_value[0], arg_value[1], arg_value[2], arg_value[3], arg_value[4], \
                arg_value[5], arg_value[6], arg_value[7], arg_value[8]);
        case 10:
            index = snprintk(log_buffer, buffer_size, fmt, arg_value[0], arg_value[1], arg_value[2], arg_value[3], arg_value[4], \
                arg_value[5], arg_value[6], arg_value[7], arg_value[8], arg_value[9]);

            break;
        default:
            break;
    }

    if (index < 0) {
        index = 0;
    }

    if (index >= (buffer_size - 3)) {
		log_buffer[buffer_size - 3] = '\r';
        log_buffer[buffer_size - 2] = '\n';
        log_buffer[buffer_size - 1] = '\0';
    } else {
		log_buffer[index++] = '\r';
        log_buffer[index++] = '\n';
        log_buffer[index++] = '\0';
    }

    return index;
}

uint32_t process_log_linebuf(log_message_t *log_msg, char *log_buffer, uint32_t buffer_size)
{
    uint32_t curr_index;

    curr_index = 0;

#ifdef CONFIG_ACTLOG_SHOW_FUNCTION
    if(IS_ENABLED(CONFIG_ACTLOG_SHOW_TIMESTAMP)){
        curr_index += snprintk(log_buffer, buffer_size,
                                "[%u][%s][%c][%s][%d]: ",
                                log_msg->normal.timestamp,
                                actlog_source_name_get(log_msg->normal.id),
                                act_loglevel_info[log_msg->normal.level],
                                log_msg->normal.func_name,
                                log_msg->normal.line_number);
    }else{
        curr_index += snprintk(log_buffer, buffer_size,
                                "[%s][%c][%s][%d]: ",
                                actlog_source_name_get(log_msg->normal.id),
                                act_loglevel_info[log_msg->normal.level],
                                log_msg->normal.func_name,
                                log_msg->normal.line_number);
    }
#else
    if(IS_ENABLED(CONFIG_ACTLOG_SHOW_TIMESTAMP)){
        curr_index += snprintk(log_buffer, buffer_size,
                                "[%u][%s][%c][%d]: ",
                                log_msg->normal.timestamp,
                                actlog_source_name_get(log_msg->normal.id),
                                act_loglevel_info[log_msg->normal.level],
                                log_msg->normal.line_number);
    }else{
        curr_index += snprintk(log_buffer, buffer_size,
                                "[%s][%c][%d]: ",
                                actlog_source_name_get(log_msg->normal.id),
                                act_loglevel_info[log_msg->normal.level],
                                log_msg->normal.line_number);
    }
#endif

    if(log_msg->normal.type == ACTLOG_MSG_LOG){
        curr_index += snprintk(&log_buffer[curr_index], (buffer_size - curr_index),
                               "%s\n", log_msg->normal.fmt_data);
    }else if(log_msg->normal.type == ACTLOG_MSG_HEXDUMP){
        curr_index += log_hex_dump_data(log_msg->hex.data, log_msg->hex.data_len, &log_buffer[curr_index], (buffer_size - curr_index));
    }else if(log_msg->normal.type == ACTLOG_MSG_NANO_LOG){
        curr_index += log_nano_dump_data(log_msg->nano.fmt, log_msg->nano.arg_num, log_msg->nano.arg_value, \
            &log_buffer[curr_index], (buffer_size - curr_index));
    }

    return curr_index;
}

static void save_log_message(log_message_t *log_msg)
{
    uint32_t log_line_cnt;

    log_line_cnt = process_log_linebuf(log_msg, log_line_buf, sizeof(log_line_buf));

    act_log_backend_output(log_msg, log_line_buf, log_line_cnt);
}

static void process_log_message(log_message_t *log_msg)
{
    save_log_message(log_msg);
}


void actlog_log_handler(actlog_ctrl_t *ctrl)
{
    int len = 0;
    uint32_t irq_flag;
	uint32_t buf_size;

    log_message_t log_msg;

#ifdef CONFIG_ACTLOG_PRINT_DROP_COUNT
    uint32_t drop_cnt;

    irq_flag = irq_lock();
    drop_cnt = ctrl->drop_cnt;
    if (ctrl->drop_cnt) {
        ctrl->drop_cnt = 0;
    }
    irq_unlock(irq_flag);

    if (IS_ENABLED(CONFIG_ACTLOG_PRINT_DROP_COUNT)){
        if (drop_cnt){
            printk("\t\t>>>log drop:%u\r\n\n", drop_cnt);
        }
    }
#endif
    while (ring_buf_size_get(&ctrl->rbuf) >= sizeof(log_message_head_t)) {
		buf_size = ring_buf_size_get(&ctrl->rbuf);
        if (ring_buf_get(&ctrl->rbuf, (uint8_t *)&log_msg, sizeof(log_message_head_t)) != sizeof(log_message_head_t)) {
			printk("read head data err\n");
			continue;
        }

        if (log_msg.nano.type >= ACTLOG_MAX_MSG_TYPE || log_msg.nano.level == 0) {
            printk("read head err:%x %x %x %x\n", log_msg.nano.type, log_msg.nano.level, ring_buf_size_get(&ctrl->rbuf), buf_size);
            irq_flag = irq_lock();
            ring_buf_reset(&ctrl->rbuf);
            irq_unlock(irq_flag);
			ctrl->err_cnt++;
            break;
        }

        if (log_msg.nano.type == ACTLOG_MSG_NANO_LOG){
            len = (log_msg.nano.arg_num + 1) << 2;

            if (ring_buf_get(&ctrl->rbuf, (uint8_t *)&log_msg.nano.fmt, len) != len) {
				printk("read data err:%x %x\n", log_msg.nano.arg_num, len);
                continue;
            }
        }else if(log_msg.normal.type == ACTLOG_MSG_LOG){
            len = log_msg.normal.str_len;

            if (ring_buf_get(&ctrl->rbuf, (uint8_t *)&log_msg.normal.fmt_data, len) != len) {
				printk("read data err: %x\n", len);
                continue;
            }
		}else{
			continue;
		}

        process_log_message(&log_msg);
    }
}

static void actlog_task_entry(void *p1, void *p2, void *p3)
{
    actlog_ctrl.task_ready = true;

    while(1){
		os_sem_take(&actlog_ctrl.log_sem, OS_FOREVER);
		actlog_log_handler(&actlog_ctrl);
    }
}

void actlog_exception_init_cb(void)
{
	if(actlog_ctrl.panic){
		return;
	}
#ifdef CONFIG_ACTIONS_PRINTK_DMA
	printk_dma_switch(0);
#endif

    if (IS_ENABLED(CONFIG_ACTLOG_OUTPUT_BINARY)) {
        if((act_log_get_output_mode() & ACTLOG_OUTPUT_MODE_BINARY) == 0){
            act_log_output_binary_init();
        }

        actlog_ctrl.output_mode = ACTLOG_OUTPUT_MODE_FLOW | ACTLOG_OUTPUT_MODE_BINARY;
    }

	printk("%s ringbuf size %d\n", __FUNCTION__, ring_buf_size_get(&actlog_ctrl.rbuf));

	actlog_ctrl.panic = true;

	actlog_log_handler(&actlog_ctrl);

	//flush flash ringbuf data to flash
	act_log_backend_flush();
}

int cmd_print_actlog(int file_id);

#if IS_ENABLED(CONFIG_SIM_FLASH_ACTS)
static void save_log_to_sram(void)
{
	struct device *sim_dev;
	char *sram_buf = __sim_flash_ram_start;

	sim_dev = (struct device *)device_get_binding(CONFIG_SIM_FLASH_NAME);
	if (!sim_dev) {
		printk("log save err\n");
		return;
	}

	printk("system panic! save simflash log 0x%p,0x%x\n", sram_buf, CONFIG_SIM_FLASH_SIZE);
	flash_read(sim_dev, 0, sram_buf, CONFIG_SIM_FLASH_SIZE);
}
#endif

void actlog_exception_run_cb(void)
{
	printk("%s ringbuf size %d\n", __FUNCTION__, ring_buf_size_get(&actlog_ctrl.rbuf));

	actlog_log_handler(&actlog_ctrl);

	//flush flash ringbuf data to flash
	act_log_backend_flush();

	//extra data only print by flow, not save in flash
	actlog_ctrl.output_mode = ACTLOG_OUTPUT_MODE_FLOW;

	//cmd_print_actlog(ACTLOG_FILE_TYPE_LOG);

#if IS_ENABLED(CONFIG_SIM_FLASH_ACTS)
	save_log_to_sram();
#endif
}
int act_log_init(void)
{
    actlog_ctrl_t *ctrl = (actlog_ctrl_t *)&actlog_ctrl;
	actions_exception_callback_routine_t actlog_exc_cb;

    //only init once
    if(ctrl->init_flag){
        return 0;
    }

	os_sem_init(&ctrl->log_sem, 0, 1);

	ring_buf_init(&ctrl->rbuf, 4096, ctrl->cache_buffer);

	ctrl->module_cnt = actlog_module_num_get();

    ctrl->task_ready = false;

    ctrl->output_mode = 0;

    ctrl->level = ACTLOG_LEVEL_INFO;

    ctrl->thread_id = os_thread_create((char *)actlog_thread_stack,
											ACTLOG_THREAD_STACK_SIZE,
											actlog_task_entry,
											NULL, NULL, NULL,
											K_LOWEST_APPLICATION_THREAD_PRIO, 0, OS_NO_WAIT);

	os_thread_name_set((struct k_thread *)ctrl->thread_id, "actlog");

    act_log_backend_init();

	printk("log used size %d\n", act_log_backend_get_used_size(0));

	act_log_module_level_init(ctrl);

    ctrl->init_flag = true;

	actlog_exc_cb.init_cb = actlog_exception_init_cb;

	actlog_exc_cb.run_cb = actlog_exception_run_cb;

	exception_register_callbacks(&actlog_exc_cb);

    return 0;
}

int act_log_register_output_backend(uint8_t mode, actlog_backend_callback_t callback, void *user_data)
{
    int ret;

    if (callback == NULL) {
        return -EINVAL;
    }

    if (actlog_ctrl.output_mode & mode) {
        return -EALREADY;
    }

    ret = act_log_backend_register(mode, callback, user_data);
    if (ret == 0) {
        actlog_ctrl.output_mode |= mode;
    }

    return ret;
}

int act_log_unregister_output_backend(uint8_t mode)
{
    if (actlog_ctrl.output_mode & mode) {
        act_log_backend_unregister(mode);
        actlog_ctrl.output_mode &= ~mode;
        return 0;
    }

    return -EINVAL;
}

uint32_t act_log_check_filter(uint8_t module_id, uint8_t level)
{
    uint8_t log_num;
    uint32_t cur_time;

	act_log_num_filter_t *filter;

	if (!actlog_ctrl.init_flag){
		return false;
	}

    if ((level > actlog_ctrl.level) || (module_id >= actlog_ctrl.module_cnt)) {
        return true;
    }

    if (level > actlog_dynamic_level_get(&actlog_ctrl, module_id)){
        return true;
    }

    filter = &actlog_ctrl.filter;

    if (!filter->enable || filter->id != module_id){
        return false;
    }

    if (filter->base_time == 0) {
        filter->base_time  = os_uptime_get_32();
    }

    cur_time = os_uptime_get_32();
    filter->line_num++;
    log_num = filter->line_num;

    if ((cur_time < filter->base_time) || ((cur_time - filter->base_time) >= filter->limit_time)) {
        filter->base_time = cur_time;
        filter->line_num = 0;
    } else {
        if (log_num > filter->max_line_num) {
            return true;
        }
    }

    return false;
}



int actlog_set_level_filter(const char *module_name, uint32_t dynamic_level)
{
    int  id;

	if (dynamic_level >= ACTLOG_LEVEL_MAX){
		printk("invalid level %d\n", dynamic_level);
		return -EINVAL;
	}

	printk("set module %s level %d\n", module_name, dynamic_level);

    if (strcmp(module_name, "all") == 0) {

        actlog_ctrl.level = dynamic_level;

    } else {

        id = actlog_module_id_get(module_name);

        /* no match tag name found */
        if (id < 0) {
            return -1;
        }

		actlog_dynamic_level_set(&actlog_ctrl, id, dynamic_level);
    }

    return 0;
}

int actlog_set_module_num_filter(const char *module_name, uint8_t enable, uint8_t max_log_num, uint16_t limit_time)
{
    int  id;
	act_log_num_filter_t *filter;

	id = actlog_module_id_get(module_name);

	/* no match tag name found */
	if (id < 0) {
		return -1;
	}

    filter = &actlog_ctrl.filter;

	filter->id = id;
	filter->enable = enable;
	filter->limit_time = limit_time;
	filter->max_line_num = max_log_num;

	return 0;
}


int act_log_is_const_ptr(const char *fmt)
{
	return ((fmt >= (const char *)__rom_region_start) &&
		(fmt < (const char *)__rom_region_end));
}

//parse:%x,%X,%d,%u,%i,%c,%p,%s
uint32_t act_log_check_fmt_string_is_valid(va_list args, const char *fmt, uint8_t *arg_num)
{
    int i, j, parse_end;
    const char *ptr;
	int parse_arg_num = 0;

    if(!arg_num){
        return true;
    }

    for (i = 0; fmt[i] != '\0'; i++) {
        if (fmt[i] != '%') {
            continue;
        }

        if (fmt[i + 1] == '%') {
            continue;
        }

		parse_arg_num++;

        j = i;
        parse_end = false;
        while(fmt[j] != '\0' && !parse_end){
            switch(fmt[j]){
                case 's':
                if ((fmt[j - 1] == '%' || (fmt[j - 1] >= '0' && fmt[j - 1] <= '9'))){
                    ptr = (const char *)va_arg(args, uint32_t);
                    if(!act_log_is_const_ptr(ptr)){
                        return false;
                    }
                    parse_end = true;
                }
                break;

                case 'x':
                case 'd':
                case 'p':
                case 'u':
                case 'c':
                case 'i':
                case 'X':
                if ((fmt[j - 1] == '%' || (fmt[j - 1] >= '0' && fmt[j - 1] <= '9'))){
                    ptr = (const char *)va_arg(args, uint32_t);
                    parse_end = true;
                }
                break;

                default:
                break;
            }
            j++;
        }
    }

	if(parse_arg_num == 0){
		*arg_num = 0;
	}

    return true;
}

void act_log_put_data(void *data, uint32_t len)
{
	int irq_flag;
    actlog_ctrl_t *ctrl = &actlog_ctrl;

	irq_flag = irq_lock();

    uint32_t free_space = ring_buf_space_get(&ctrl->rbuf);

    if (free_space >= len) {
        ring_buf_put(&ctrl->rbuf, data, len);
    }else{
        ctrl->drop_cnt++;
    }

    if (k_is_in_isr()){
        ctrl->irq_cnt++;
    }

	irq_unlock(irq_flag);

    if(ctrl->panic){
        actlog_log_handler(ctrl);
    }else{
        os_sem_give(&ctrl->log_sem);
    }

    return;
}


//static char nano_tips[] = "args num err";

static void act_nano_log(uint8_t module_id, uint8_t level, const char *func, uint16_t line, uint8_t arg_num, const char *fmt, va_list args)
{
    uint8_t i;

    nano_log_message_t nano;

    memset(&nano, 0, sizeof(nano_log_message_t));

    nano.type = ACTLOG_MSG_NANO_LOG;
    nano.id = module_id;
    nano.level = level;
    nano.line_number = line;

#ifdef CONFIG_ACTLOG_SHOW_FUNCTION
    nano.func_name = func;
#endif

    if(IS_ENABLED(CONFIG_ACTLOG_SHOW_TIMESTAMP)){
        nano.timestamp = os_uptime_get_32();
    }
    nano.fmt = fmt;
    nano.arg_num = arg_num;

    for(i = 0; i < arg_num; i++){
        nano.arg_value[i] = va_arg(args, uint32_t);
    }

	//printk("nano %d fmt %s arg %d %x %x\n", line, fmt, arg_num, nano.arg_value[0], nano.arg_value[1]);

    //k_fifo_put(&actlog_ctrl.log_input_fifo, (void *)log_msg);
    act_log_put_data(&nano, sizeof(log_message_head_t) + ((arg_num + 1) * sizeof(uint32_t)));
}

static void act_normal_log(uint8_t module_id, uint8_t level, const char *func, uint16_t line,  const char *fmt, va_list args)
{
	int strlen;
	int buf_size;

	normal_log_message_t normal;

	memset(&normal, 0, sizeof(normal));

    normal.type = ACTLOG_MSG_LOG;
    normal.id = module_id;
    normal.level = level;
    normal.line_number = line;

#ifdef CONFIG_ACTLOG_SHOW_FUNCTION
    normal.func_name = func;
#endif
    if(IS_ENABLED(CONFIG_ACTLOG_SHOW_TIMESTAMP)){
        normal.timestamp = os_uptime_get_32();
    }

    strlen = vsnprintk(normal.fmt_data, CONFIG_ACTLOG_FMT_DATA_SIZE, fmt, args);

	buf_size = sizeof(normal.fmt_data);
    if (strlen >= (buf_size - 3)) {
		normal.fmt_data[buf_size - 3] = '\r';
        normal.fmt_data[buf_size - 2] = '\n';
        normal.fmt_data[buf_size - 1] = '\0';
		strlen = buf_size;
    } else {
		normal.fmt_data[strlen++] = '\r';
        normal.fmt_data[strlen++] = '\n';
        normal.fmt_data[strlen++] = '\0';
    }

	normal.str_len = strlen;

    act_log_put_data(&normal, sizeof(log_message_head_t) + strlen);
}

#ifdef CONFIG_ACTLOG_SHOW_FUNCTION
void actlog_printk(uint32_t pack_data, const char *func,  const char *fmt, ...)
#else
void actlog_printk(uint32_t pack_data, const char *fmt, ...)
#endif
{
    act_log_pack_data log_data;
    log_data.data = pack_data;

    va_list args;
	va_start(args, fmt);
    if(act_log_check_filter(log_data.bit_data.id, log_data.bit_data.level)){
        return;
    }

	if(!actlog_ctrl.task_ready){
		printk("[%s][%c][%d*]:", actlog_source_name_get(log_data.bit_data.id),  act_loglevel_info[log_data.bit_data.level], log_data.bit_data.line);
		vprintk(fmt, args);
		printk("\r\n");
        return;
	}

#ifdef CONFIG_ACTLOG_SHOW_FUNCTION
    act_normal_log(log_data.bit_data.id, log_data.bit_data.level, func, log_data.bit_data.line, fmt, args);
#else
    act_normal_log(log_data.bit_data.id, log_data.bit_data.level, NULL, log_data.bit_data.line, fmt, args);
#endif
    va_end(args);
}

#ifdef CONFIG_ACTLOG_SHOW_FUNCTION
void actlog_printk_nano(uint32_t pack_data, const char *func, const char *fmt, ...)
#else
void actlog_printk_nano(uint32_t pack_data, const char *fmt, ...)
#endif
{
    act_log_pack_data log_data;
    log_data.data = pack_data;
	uint8_t arg_num;

    va_list args;
    va_start(args, fmt);

    if(act_log_check_filter(log_data.bit_data.id, log_data.bit_data.level)){
		//printk("filter print:%d %d\n", log_data.bit_data.id, log_data.bit_data.line);
		//vprintk(fmt, args);
		//printk("\r\n");
        return;
    }

	if(!actlog_ctrl.task_ready){
		printk("[%s][%c][%d*]:", actlog_source_name_get(log_data.bit_data.id),  act_loglevel_info[log_data.bit_data.level], log_data.bit_data.line);
		vprintk(fmt, args);
		printk("\r\n");
        return;
	}

	arg_num = log_data.bit_data.arg_num;

    if (arg_num > MAX_NANO_ARG_NUM || !act_log_check_fmt_string_is_valid(args, fmt, &arg_num)){
#ifdef CONFIG_ACTLOG_SHOW_FUNCTION
		act_normal_log(log_data.bit_data.id, log_data.bit_data.level, func, log_data.bit_data.line, fmt, args);
#else
		act_normal_log(log_data.bit_data.id, log_data.bit_data.level, NULL, log_data.bit_data.line, fmt, args);
#endif
		return;
    }

#ifdef CONFIG_ACTLOG_SHOW_FUNCTION
    act_nano_log(log_data.bit_data.id, log_data.bit_data.level, func, log_data.bit_data.line, arg_num, fmt, args);
#else
    act_nano_log(log_data.bit_data.id, log_data.bit_data.level, NULL, log_data.bit_data.line, arg_num, fmt, args);
#endif
    va_end(args);
}

void actlog_dump_runtime_info(void)
{
	uint32_t i;

    actlog_ctrl_t *ctrl = (actlog_ctrl_t *)&actlog_ctrl;

	printk("module cnt %d level %d\n", ctrl->module_cnt, ctrl->level);
	printk("drop cnt %d err cnt %d\n", ctrl->drop_cnt, ctrl->err_cnt);
	printk("rbuf size %d\n", ring_buf_size_get(&ctrl->rbuf));
	printk("out mode %d\n", ctrl->output_mode);

	printk("filter %s enable:%d log num %d limit time %d\n", actlog_source_name_get(ctrl->filter.id),\
		ctrl->filter.enable, ctrl->filter.max_line_num, ctrl->filter.limit_time);

	for(i = 0; i < ctrl->module_cnt; i++){
		if (actlog_dynamic_level_get(&actlog_ctrl, i) < 5) {
			printk("%s level:%c\n", actlog_source_name_get(i), act_loglevel_info[actlog_dynamic_level_get(&actlog_ctrl, i)]);
		}
	}

}



typedef int (*bt_api_transfer_cb)(uint8_t *data, uint32_t max_len);
bt_api_transfer_cb g_bt_cb = NULL;
static int actlog_bt_callback(uint8_t *data, uint32_t len)
{
    uint32_t read_len = 0;
    uint32_t str_len;
    while(read_len < len){
        if(*data){
            str_len = actlog_strnlen(data, len - read_len);
            //flow output,do not use printk
            if(g_bt_cb)
            	g_bt_cb(data, str_len);
            read_len += str_len;
            data += str_len;
        }else{
            data++;
            read_len++;
        }
    }
    return 0;
}

int actlog_syslog_transfer(int log_type, int (*traverse_cb)(uint8_t *data, uint32_t max_len))
{
	int traverse_len;
	int file_id;
    /* Print buffer */
    char print_buf[CONFIG_ACTLOG_LINEBUF_SIZE];
	if(traverse_cb == NULL)
		return 0;
	if(log_type == LOG_TYPE_SYSLOG)
		file_id = ACTLOG_FILE_TYPE_LOG_SAVE;
	else
		file_id = ACTLOG_FILE_TYPE_RUNTIME_LOG;

	//flush flash ringbuf data to flash
	act_log_backend_flush();

	g_bt_cb = traverse_cb;
    traverse_len = act_log_backend_traverse(file_id, actlog_bt_callback, print_buf, sizeof(print_buf));
	g_bt_cb = NULL;
    return traverse_len;
}

/* The log process thread has the K_LOWEST_APPLICATION_THREAD_PRIO, adjust it
 * to a higher priority to increase the chances of being scheduled to handle
 * log message as soon as possible
 */
void actlog_increase_log_thread_priority(void)
{
    actlog_ctrl_t *ctrl = (actlog_ctrl_t *)&actlog_ctrl;
	k_thread_priority_set((k_tid_t)ctrl->thread_id, 4);
}

void actlog_decrease_log_thread_priority(void)
{
    actlog_ctrl_t *ctrl = (actlog_ctrl_t *)&actlog_ctrl;
	k_thread_priority_set((k_tid_t)ctrl->thread_id, K_LOWEST_APPLICATION_THREAD_PRIO);
}
