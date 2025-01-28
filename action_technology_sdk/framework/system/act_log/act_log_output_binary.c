#include <errno.h>
#include <kernel.h>
#include <string.h>
#include <toolchain.h>
#include <sys/util.h>
#include <partition/partition.h>
#include <sys/ring_buffer.h>
#include <board_cfg.h>
#include <drivers/flash.h>
#include "act_log_inner.h"

#define CACHE_BUFFER_SIZE (CONFIG_ACTLOG_FLASH_CACHE_SIZE)

struct flash_log_ctx
{
    struct flash_buffer_ctx *normal_flash_buffer;

    struct flash_buffer_ctx *runtime_log_flash_buffer;

    struct flash_buffer_ctx *event_flash_buffer;
#if IS_ENABLED(CONFIG_SIM_FLASH_ACTS)
    struct flash_buffer_ctx *save_flash_buffer; // for sim flash(ram flash)  save, if panic
#endif
    struct ring_buf rbuf;
    uint8_t cache_buffer[CACHE_BUFFER_SIZE];
};

static struct flash_log_ctx flash_log;

static uint32_t act_log_output_binary_log_write(log_message_t *log_msg, uint8_t *data, uint32_t len);
uint32_t act_log_compress_log_write(log_message_t *log_msg, uint8_t *data, uint32_t len);
int act_log_output_binary_compress_data_traverse(struct flash_buffer_ctx *buffer_ctx,
        int (*traverse_cb)(uint8_t *data, uint32_t max_len), uint8_t *buf, uint32_t len);
static struct flash_buffer_ctx *flash_buffer_get(struct flash_log_ctx *ctx, int32_t file_id)
{
    if (file_id == ACTLOG_FILE_TYPE_LOG) {
        if (IS_ENABLED(CONFIG_ACTLOG_LOG_BUFFER)) {
            return ctx->normal_flash_buffer;
        }
    }

    if (file_id == ACTLOG_FILE_TYPE_RUNTIME_LOG) {
        if (IS_ENABLED(CONFIG_ACTLOG_RUNTIME_LOG_BUFFER)) {
            return ctx->runtime_log_flash_buffer;
        }
    }

    if (file_id == ACTLOG_FILE_TYPE_EVENT) {
        if (IS_ENABLED(CONFIG_ACTLOG_EVENT_BUFFER)) {
            return ctx->event_flash_buffer;
        }
    }
    if (file_id == ACTLOG_FILE_TYPE_LOG_SAVE) {
#if IS_ENABLED(CONFIG_SIM_FLASH_ACTS)
        return ctx->save_flash_buffer;
#else
        return ctx->normal_flash_buffer;
#endif
    }

    return NULL;
}

int act_log_output_binary_init(void)
{
    struct flash_log_ctx *ctx = &flash_log;

    ring_buf_init(&ctx->rbuf, CACHE_BUFFER_SIZE, ctx->cache_buffer);

    if (IS_ENABLED(CONFIG_ACTLOG_LOG_BUFFER)) {
#if IS_ENABLED(CONFIG_SIM_FLASH_ACTS)
        check_save_log_to_flash();
        ctx->save_flash_buffer = flash_buffer_init(PARTITION_FILE_ID_LOGBUF, true, true, NULL);
        ctx->normal_flash_buffer = flash_buffer_init(0, true, true, &ctx->rbuf);
#else
        ctx->normal_flash_buffer = flash_buffer_init(PARTITION_FILE_ID_LOGBUF, true, true, &ctx->rbuf);
#endif
        if (!ctx->normal_flash_buffer) {
            return -EIO;
        }
    }

    if (IS_ENABLED(CONFIG_ACTLOG_RUNTIME_LOG_BUFFER)) {
        ctx->runtime_log_flash_buffer = flash_buffer_init(PARTITION_FILE_ID_RUNTIME_LOGBUF, true, true, &ctx->rbuf);

        if (!ctx->runtime_log_flash_buffer) {
            return -EIO;
        }
    }

    if (IS_ENABLED(CONFIG_ACTLOG_EVENT_BUFFER)) {
        ctx->event_flash_buffer = flash_buffer_init(PARTITION_FILE_ID_EVTBUF, false, false, NULL);

        if (!ctx->event_flash_buffer) {
            return -EIO;
        }
    }

    return 0;
}

#if defined(CONFIG_ACTLOG_LOG_DATA_COMPRESS)
static int log_data_compress(log_message_t *log_msg, flash_log_item_t *item)
{
    int i, index, len = 0;

    if (log_msg->normal.type == ACTLOG_MSG_NANO_LOG) {
        memset(item, 0, sizeof(flash_log_item_t));
        item->head.head = ACTLOG_FLASH_ITEM_MAGIC;
        item->head.arg_num = log_msg->nano.arg_num;
        item->head.id = log_msg->nano.id;
        item->head.level = log_msg->nano.level;
        item->head.line = log_msg->nano.line_number;
        item->head.log_type = ACTLOG_MSG_NANO_LOG;

        item->head.fmt = act_log_compress_const_data((uint32_t)log_msg->nano.fmt);

        index = 0;

#ifdef CONFIG_ACTLOG_SHOW_FUNCTION
        //write func name
        if(log_msg->nano.func_name) {
            item->head.func_name_enable = 1;
            item->data[index++] = (uint32_t)log_msg->nano.func_name;
        }
#endif

#ifdef CONFIG_ACTLOG_SHOW_TIMESTAMP
        //write timestamp
        if(log_msg->nano.timestamp) {
            item->head.timestamp_enable = 1;
            item->data[index++] = log_msg->nano.timestamp;
        }
#endif

        for(i = 0; i < log_msg->nano.arg_num; i++) {
            item->data[i + index] = log_msg->nano.arg_value[i];
        }

        len = sizeof(flash_log_head_t) + (log_msg->nano.arg_num + index) * sizeof(uint32_t);
    }else if(log_msg->normal.type == ACTLOG_MSG_LOG){
		memset(item, 0, sizeof(flash_log_item_t));
		item->head.head = ACTLOG_FLASH_ITEM_MAGIC;
		item->head.arg_num = 1;
		item->head.id = log_msg->normal.id;
		item->head.level = log_msg->normal.level;
		item->head.line = log_msg->normal.line_number;
		item->head.log_type = ACTLOG_MSG_LOG;

		len = actlog_strnlen(log_msg->normal.fmt_data, (sizeof(flash_log_item_t) - sizeof(flash_log_head_t)));

		item->head.fmt = len;

		memcpy(&item->data[0], log_msg->normal.fmt_data, len);

		len = sizeof(flash_log_head_t) + len;

	}

    return len;

}

static void log_data_decompress(log_message_t *log_msg, flash_log_item_t *item)
{
    int i, index = 0;

	memset(log_msg, 0, sizeof(log_message_t));

	if(item->head.log_type == ACTLOG_MSG_NANO_LOG){
		log_msg->nano.id = item->head.id;
		log_msg->nano.type = ACTLOG_MSG_NANO_LOG;
		log_msg->nano.level = item->head.level;
		log_msg->nano.line_number = item->head.line;
		log_msg->nano.fmt = (const char *)act_log_decompress_const_data(item->head.fmt);
		log_msg->nano.arg_num = item->head.arg_num;
#ifdef CONFIG_ACTLOG_SHOW_FUNCTION
		if(item->head.func_name_enable) {
			log_msg->nano.func_name = (const char *)item->data[index++];
		}
#endif

#ifdef CONFIG_ACTLOG_SHOW_TIMESTAMP
		if(item->head.timestamp_enable) {
			log_msg->nano.timestamp = item->data[index++];
		}
#endif

		for(i = 0; i < item->head.arg_num; i++) {
			log_msg->nano.arg_value[i] = item->data[index + i];
		}
	}else if(item->head.log_type == ACTLOG_MSG_LOG){
		log_msg->normal.id = item->head.id;
		log_msg->normal.type = ACTLOG_MSG_LOG;
		log_msg->normal.level = item->head.level;
		log_msg->normal.line_number = item->head.line;
		log_msg->normal.str_len = item->head.fmt;
#ifdef CONFIG_ACTLOG_SHOW_FUNCTION
		log_msg->normal.func_name = NULL;
#endif
#ifdef CONFIG_ACTLOG_SHOW_TIMESTAMP
		log_msg->normal.timestamp = 0;
#endif
		memcpy(log_msg->normal.fmt_data, item->data, item->head.fmt);
	}
}

static int check_log_head_valid(flash_log_head_t *head)
{
    if(head->head != ACTLOG_FLASH_ITEM_MAGIC) {
        return false;
    }

    if(head->arg_num > MAX_NANO_ARG_NUM) {
        return false;
    }

    if(head->id >= actlog_ctrl.module_cnt) {
        return false;
    }

    if(head->log_type != ACTLOG_MSG_LOG && head->log_type != ACTLOG_MSG_NANO_LOG) {
        return false;
    }

	if(head->log_type == ACTLOG_MSG_NANO_LOG){
	    if(!act_log_is_const_ptr((const char *)act_log_decompress_const_data(head->fmt))) {
	        return false;
	    }
	}else{
		if(head->fmt > (sizeof(flash_log_item_t) - sizeof(flash_log_head_t))){
			return false;
		}
	}

    return true;
}

int act_log_output_binary_compress_data_traverse(struct flash_buffer_ctx *buffer_ctx, int (*traverse_cb)(uint8_t *data, uint32_t max_len), uint8_t *buf, uint32_t len)
{
    uint32_t read_len;
    uint32_t read_addr;
    uint32_t total_len;

    flash_log_item_t item;
    log_message_t log_msg;
    int line_size;

    read_addr = 0;
    total_len = 0;
    while(1) {
        read_len = flash_buffer_read(buffer_ctx, read_addr, (uint32_t *)&item, sizeof(flash_log_head_t));

        if(read_len == 0) {
			printk("flash end %d size %d\n", read_addr, flash_buffer_get_data_size(buffer_ctx));
            break;
        }

        if(!check_log_head_valid(&item.head)) {
            printk("check head err addr:0x%x\n", read_addr);
            actlog_print_buffer((const uint8_t *)&item, 1, sizeof(flash_log_head_t), 16, -1);
            read_addr += 1;
            continue;
        }

        //actlog_print_buffer((const uint8_t *)&item, 1, sizeof(flash_log_head_t), 16, -1);

        read_addr += read_len;

        //printk("item %d %d %d\n", item.head.arg_num, item.head.func_name_enable, item.head.timestamp_enable);

		if(item.head.log_type == ACTLOG_MSG_NANO_LOG){
			read_len = (item.head.arg_num + item.head.func_name_enable + item.head.timestamp_enable) << 2;
		}else{
			read_len = item.head.fmt;
		}

        if(read_len) {
            read_len = flash_buffer_read(buffer_ctx, read_addr, (uint32_t *)&item.data, read_len);

            //actlog_print_buffer((const uint8_t *)&item.data, 1, read_len, 16, -1);

            if(read_len == 0) {
                printk("flash end %d size %d\n", read_addr, flash_buffer_get_data_size(buffer_ctx));
                break;
            }
        }

        read_addr += read_len;

        log_data_decompress(&log_msg, &item);

        //printk("traverse addr:%x\n", read_addr);

        line_size = process_log_linebuf(&log_msg, buf, len);

        //printk("fmt:%s num %d value %d %x len %d\n", log_msg.nano.fmt, log_msg.nano.arg_num, log_msg.nano.arg_value[0], log_msg.nano.arg_value[1], line_size);

        if(traverse_cb) {
            traverse_cb(buf, line_size);
        }

        total_len += line_size;
    }

    return total_len;
}

uint32_t act_log_compress_log_write(log_message_t *log_msg, uint8_t *data, uint32_t len)
{
    flash_log_item_t item;
    len = log_data_compress(log_msg, &item);
    len = act_log_output_binary_log_write(log_msg, (uint8_t *)&item, len);
    return len;
}

#endif

static uint32_t act_log_output_binary_log_write(log_message_t *log_msg, uint8_t *data, uint32_t len)
{
    struct flash_log_ctx *ctx = &flash_log;

    struct flash_buffer_ctx *buffer_ctx = flash_buffer_get(ctx, ACTLOG_FILE_TYPE_LOG);

    uint32_t free_space = ring_buf_space_get(&ctx->rbuf);

    if (len > free_space) {
        /* all cache buffer write data, must flush buffer to flash */
		if (IS_ENABLED(CONFIG_ACTLOG_RUNTIME_LOG_BUFFER)) {
			struct flash_buffer_ctx *runtime_buffer_ctx = flash_buffer_get(ctx, ACTLOG_FILE_TYPE_RUNTIME_LOG);
			flash_buffer_sync(buffer_ctx, runtime_buffer_ctx);
		}else{
	        flash_buffer_sync(buffer_ctx, NULL);
		}
    }

    ring_buf_put(&ctx->rbuf, data, len);

    return len;
}

int act_log_output_binary_write(log_message_t *log_msg, uint8_t *data, uint32_t len)
{
    if (log_msg->normal.type == ACTLOG_MSG_NANO_LOG) {
        if (IS_ENABLED(CONFIG_ACTLOG_LOG_DATA_COMPRESS)) {
            len = act_log_compress_log_write(log_msg, data, len);
        } else {
            len = act_log_output_binary_log_write(log_msg, data, len);        }

    } else if(log_msg->normal.type == ACTLOG_MSG_LOG){
		if(!actlog_ctrl.panic){
			return 0;
		}

        if (IS_ENABLED(CONFIG_ACTLOG_LOG_DATA_COMPRESS)) {
            len = act_log_compress_log_write(log_msg, data, len);
        } else {
            len = act_log_output_binary_log_write(log_msg, data, len);
        }
    }

	return len;

}

int act_log_output_binary_get_used_size(int file_id)
{
    struct flash_log_ctx *ctx = &flash_log;

    struct flash_buffer_ctx *buffer_ctx = flash_buffer_get(ctx, file_id);

    if (ctx) {
        return flash_buffer_get_data_size(buffer_ctx);
    } else {
        return 0;
    }
}

int act_log_output_binary_normal_data_traverse(struct flash_buffer_ctx *buffer_ctx,
        int (*traverse_cb)(uint8_t *data, uint32_t max_len), uint8_t *buf, uint32_t len)
{
    int32_t used_size;
    uint32_t read_addr;

    used_size = flash_buffer_get_data_size(buffer_ctx);

    read_addr = 0;

    while (1) {
        if (used_size < len) {
            len = used_size;
        }

        len = flash_buffer_read(buffer_ctx, read_addr, (uint32_t *) buf, len);

        if (len == 0) {
            break;
        }

        if (traverse_cb) {
            traverse_cb(buf, len);
        }

        read_addr += len;
        used_size -= len;

        if (used_size == 0) {
            break;
        }
    }

    return read_addr;
}

int act_log_output_binary_traverse(int file_id, int (*traverse_cb)(uint8_t *data, uint32_t max_len), uint8_t *buf,
        uint32_t len)
{
    int traverse_len = 0;

    struct flash_log_ctx *ctx = &flash_log;

    struct flash_buffer_ctx *buffer_ctx = flash_buffer_get(ctx, file_id);

    if (!buf || !len || !buffer_ctx || ((uint32_t) buf % 4) || (len % 4)) {
        printk("traverse %p %x %p\n", buf, len, buffer_ctx);
        return 0;
    }

    if (file_id == ACTLOG_FILE_TYPE_LOG || file_id == ACTLOG_FILE_TYPE_LOG_SAVE || file_id == ACTLOG_FILE_TYPE_RUNTIME_LOG) {
        if (IS_ENABLED(CONFIG_ACTLOG_LOG_DATA_COMPRESS)) {
            traverse_len = act_log_output_binary_compress_data_traverse(buffer_ctx, traverse_cb, buf, len);
        } else {
            traverse_len = act_log_output_binary_normal_data_traverse(buffer_ctx, traverse_cb, buf, len);
        }
    } else {
        printk("file_id=%d not spport\n", file_id);
    }

    return traverse_len;
}

int act_log_output_binary_flush(void)
{
    struct flash_log_ctx *ctx = &flash_log;
    struct flash_buffer_ctx *buffer_ctx = flash_buffer_get(ctx, ACTLOG_FILE_TYPE_LOG);

	flash_buffer_flush_enable(buffer_ctx, true);

    /* all cache buffer write data, must flush buffer to flash */
	if (IS_ENABLED(CONFIG_ACTLOG_RUNTIME_LOG_BUFFER)) {
		struct flash_buffer_ctx *runtime_buffer_ctx = flash_buffer_get(ctx, ACTLOG_FILE_TYPE_RUNTIME_LOG);

		if(actlog_ctrl.panic){
			flash_buffer_flush_enable(runtime_buffer_ctx, false);
		}else{
			flash_buffer_flush_enable(runtime_buffer_ctx, true);
		}

		flash_buffer_sync(buffer_ctx, runtime_buffer_ctx);
	}else{
	    flash_buffer_sync(buffer_ctx, NULL);
	}

    return 0;
}

void act_log_runtime_flush_enable(int enable)
{
	if (IS_ENABLED(CONFIG_ACTLOG_RUNTIME_LOG_BUFFER)) {
		struct flash_log_ctx *ctx = &flash_log;
		struct flash_buffer_ctx *runtime_buffer_ctx = flash_buffer_get(ctx, ACTLOG_FILE_TYPE_RUNTIME_LOG);
		if(runtime_buffer_ctx != NULL){
			printk("runtime log flush en=%d\n", enable);
			flash_buffer_flush_enable(runtime_buffer_ctx, enable);
		}
	}
}

int act_log_output_binary_clear(int file_id)
{
    struct flash_log_ctx *ctx = &flash_log;
    struct flash_buffer_ctx *buffer_ctx = flash_buffer_get(ctx, file_id);

    if (buffer_ctx) {
        flash_buffer_clear(buffer_ctx);

        return 0;
    } else {
        return -EINVAL;
    }
}
