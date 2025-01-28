#ifndef __CONFIG_ACT_LOG_FLASH_BUFFER_H
#define __CONFIG_ACT_LOG_FLASH_BUFFER_H

#include <kernel.h>
#include <sys/ring_buffer.h>
#include <logging/act_log.h>

#define ACTLOG_FLASH_ITEM_MAGIC   (0x5)
#define FLASH_WRITE_PAGE_SIZE (128)

typedef struct
{
    uint32_t head :4;
    uint32_t arg_num :4;
    uint32_t id :8;
    uint32_t line :16;
    uint32_t fmt :24;
    uint32_t func_name_enable :1;
    uint32_t timestamp_enable :1;
    uint32_t level :3;
    uint32_t log_type :3;
} flash_log_head_t;

typedef struct
{
    flash_log_head_t head;
    uint32_t data[MAX_NANO_ARG_NUM + 2];
} flash_log_item_t;

struct flash_buffer_ctx
{
    /* For use with flash read/write */
    const struct device *storage_dev;

    /* flash buffer base address */
    uint64_t base_addr;
    /* flash buffer total size, bytes unit */
    uint64_t total_size;
    /* flash buffer current write address, bytes unit */
    uint64_t log_start_addr;

    uint64_t log_end_addr;

    uint8_t init_flag :1;
    uint8_t erase_enable :1;
    uint8_t flush_enable :1;

    struct ring_buf *rbuf;
};

//flash buffer
struct flash_buffer_ctx *flash_buffer_init(int partition_id, int erase_enable, int flush_enable, struct ring_buf *rbuf);

int flash_buffer_sync(struct flash_buffer_ctx *ctx, struct flash_buffer_ctx *runtime_ctx);

int flash_buffer_write(struct flash_buffer_ctx *ctx, uint8_t *buf, uint32_t len);

int flash_buffer_get_data_size(struct flash_buffer_ctx *ctx);

int flash_buffer_read(struct flash_buffer_ctx *ctx, uint64_t addr, uint32_t *buf, uint32_t len);

int flash_buffer_clear(struct flash_buffer_ctx *ctx);

void flash_buffer_flush_enable(struct flash_buffer_ctx *ctx, int enable);

#endif
