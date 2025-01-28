#include <errno.h>
#include <kernel.h>
#include <string.h>
#include <toolchain.h>
#include <sys/util.h>
#include <partition/partition.h>
#include <sys/ring_buffer.h>
#include <board_cfg.h>
#include <soc.h>
#include <drivers/flash.h>
#include <linker/linker-defs.h>
#include "act_log_inner.h"
#include "easyflash/easyflash.h"

#define FLASH_WRITE_MIN_SIZE  (4)

#if IS_ENABLED(CONFIG_ACTLOG_RUNTIME_LOG_BUFFER)
#define RUNTIME_LOG_BUFFER_NUM (1)
#else
#define RUNTIME_LOG_BUFFER_NUM (0)
#endif

#if IS_ENABLED(CONFIG_ACTLOG_EVENT_BUFFER)
#define EVENT_BUFFER_NUM (1)
#else
#define EVENT_BUFFER_NUM (0)
#endif

#if IS_ENABLED(CONFIG_SIM_FLASH_ACTS)
#define LOG_BUFFER_NUM (2)
#else
#define LOG_BUFFER_NUM (1)
#endif

#define CONFIG_FLASH_BUFFER_NUM (LOG_BUFFER_NUM + RUNTIME_LOG_BUFFER_NUM + EVENT_BUFFER_NUM)

static struct flash_buffer_ctx flash_buffer[CONFIG_FLASH_BUFFER_NUM];

static uint8_t write_temp_buffer[FLASH_WRITE_PAGE_SIZE] __aligned(4);

/**
 * @brief Open the coredump storage.
 *
 * @return device: storage device
 */
static struct device *flash_buffer_storage_init(int partition_id)
{
    struct device *dev = NULL;
    const struct partition_entry *part;

    if (partition_id) {
        part = partition_get_part(partition_id);
        if (!part) {
            return NULL;
        }

		if(part->storage_id == STORAGE_ID_NOR){
	        dev = (struct device *) device_get_binding(CONFIG_ACTLOG_STORAGE_NAME);
	        if (!dev) {
	            printk("init flash buffer failed %s\n", CONFIG_ACTLOG_STORAGE_NAME);
	            return NULL;
	        }
		}else if(part->storage_id == STORAGE_ID_NAND
		         || part->storage_id == STORAGE_ID_BOOTNAND
		         || part->storage_id == STORAGE_ID_SD){
#ifdef CONFIG_BLOCK_DEV_FLASH_ACTS
			dev = (struct device *) device_get_binding(CONFIG_BLOCK_DEV_FLASH_NAME);
			if (!dev) {
				printk("init flash buffer failed %s\n", CONFIG_BLOCK_DEV_FLASH_NAME);
				return NULL;
			}
#endif
		}
    } else {
#if IS_ENABLED(CONFIG_SIM_FLASH_ACTS)
        dev = (struct device *)device_get_binding(CONFIG_SIM_FLASH_NAME);
        if (!dev) {
            printk("init flash buffer failed %s\n", CONFIG_SIM_FLASH_NAME);
            return NULL;
        }
#else
        return NULL;
#endif
    }
    return dev;
}
#if IS_ENABLED(CONFIG_SIM_FLASH_ACTS)
void check_save_log_to_flash(void)
{
    unsigned int val = 0;
    struct device *f_dev;
    char *sram_buf = __sim_flash_ram_start;
    const struct partition_entry *part;

    soc_pstore_get(SOC_PSTORE_TAG_SYS_PANIC, &val);
    if(val) {
        f_dev = (struct device *)device_get_binding(CONFIG_ACTLOG_STORAGE_NAME);
        part = partition_get_part(PARTITION_FILE_ID_LOGBUF);
        if (!part || !f_dev) {
            printk("log save:%p,%p err\n", part, f_dev);
            return;
        }
        if(part->size < CONFIG_SIM_FLASH_SIZE) {
            printk("log part 0x%x < 0x%x\n", part->size, CONFIG_SIM_FLASH_SIZE);
            return;
        }
        printk("system panic! save sram log 0x%p,0x%x \n", sram_buf, CONFIG_SIM_FLASH_SIZE);
        flash_erase(f_dev, part->offset, part->size);
        flash_write(f_dev, part->offset, sram_buf, CONFIG_SIM_FLASH_SIZE);
        soc_pstore_set(SOC_PSTORE_TAG_SYS_PANIC, 0);
    } else {
        printk("log not save\n");
    }
}

#endif

struct flash_buffer_ctx *flash_buffer_init(int partition_id, int erase_enable, int flush_enable, struct ring_buf *rbuf)
{
    struct device *storage_dev;
    const struct partition_entry *part;
    int i;
    struct flash_buffer_ctx *ctx;
    int flag;

    flag = irq_lock();

    for (i = 0; i < CONFIG_FLASH_BUFFER_NUM; i++) {
        if (!flash_buffer[i].init_flag) {
            flash_buffer[i].init_flag = true;
            break;
        }
    }

    irq_unlock(flag);

    if (i == CONFIG_FLASH_BUFFER_NUM) {
        return NULL;
    }

    ctx = &flash_buffer[i];

    ctx->flush_enable = flush_enable;
    ctx->rbuf = rbuf;

    if (ctx->flush_enable) {
        storage_dev = flash_buffer_storage_init(partition_id);
        if (!storage_dev) {
            return NULL;
        }
        if (partition_id != 0) {
            part = partition_get_part(partition_id);
            if (!part) {
                return NULL;
            }
            ctx->base_addr = part->offset;
            ctx->total_size = part->size;
            ctx->storage_dev = storage_dev;
            ctx->erase_enable = erase_enable;
        } else {
#if IS_ENABLED(CONFIG_SIM_FLASH_ACTS)
            ctx->base_addr = 0;
            ctx->total_size = CONFIG_SIM_FLASH_SIZE;
            ctx->storage_dev = storage_dev;
            ctx->erase_enable = erase_enable;
#else
            return NULL;
#endif
        }

        printk("actlog stoarge %p base %llx size %lld\n", storage_dev, ctx->base_addr, ctx->total_size);

        ef_log_init(ctx);
    }

    return (void *) ctx;
}

void flash_buffer_flush_enable(struct flash_buffer_ctx *ctx, int enable)
{
    ctx->flush_enable = enable;
}

int flash_buffer_sync(struct flash_buffer_ctx *ctx, struct flash_buffer_ctx *runtime_ctx)
{
    uint32_t write_size, total_len;
	struct flash_buffer_ctx *cur_ctx;

	if(ctx == NULL)
		cur_ctx = runtime_ctx;
	else
		cur_ctx = ctx;
	if(cur_ctx == NULL)
		return 0;

    total_len = 0;
    while (1) {
        write_size = ring_buf_get(cur_ctx->rbuf, (uint8_t *) write_temp_buffer, FLASH_WRITE_PAGE_SIZE);

        if (write_size == 0) {
            break;
        }

		//memset unaligned data to '\0'
		if (write_size < FLASH_WRITE_PAGE_SIZE) {
			memset(&write_temp_buffer[write_size], 0, FLASH_WRITE_PAGE_SIZE - write_size);
		}

		write_size = ROUND_UP(write_size, FLASH_WRITE_MIN_SIZE);

        if ((ctx!= NULL) && ctx->flush_enable) {

            ef_log_write(ctx, (const uint32_t *)write_temp_buffer, write_size);
        }

		if((runtime_ctx!= NULL) && runtime_ctx->flush_enable){
			ef_log_write(runtime_ctx, (const uint32_t *)write_temp_buffer, write_size);
		}

		total_len += write_size;

    }

    return total_len;
}

int flash_buffer_write(struct flash_buffer_ctx *ctx, uint8_t *buf, uint32_t len)
{
    int ret_val;
    int align_len;
    char write_allign_char[4] = { 0 };

    if (!buf || !len) {
        return 0;
    }

    align_len = len / 4 * 4;

    /* write log to flash */
    ret_val = ef_log_write(ctx, (uint32_t *) buf, align_len);

    /* write last word alignment data */
    if ((ret_val == 0) && (align_len != len)) {
        memcpy(write_allign_char, buf + align_len, len - align_len);
        ef_log_write(ctx, (uint32_t *) write_allign_char, 4);
    }

    return len;
}

int flash_buffer_get_data_size(struct flash_buffer_ctx *ctx)
{
    return ef_log_get_used_size(ctx);
}

//elog is 4bytes allign, support bytes read
int flash_buffer_read(struct flash_buffer_ctx *ctx, uint64_t addr, uint32_t *buf, uint32_t len)
{
    int ret_val;
    int copy_len;
    int align_addr;
    char read_allign_buf[4];

    uint8_t *pdata = (uint8_t *) buf;

    if (!buf || !len) {
        return 0;
    }

    //printk("read addr %x size %x\n", addr, len);

    align_addr = addr / 4 * 4;

    if (align_addr != addr) {
        copy_len = 4;
        ret_val = ef_log_read(ctx, align_addr, (uint32_t *) read_allign_buf, &copy_len);
        if (ret_val != 0) {
            return 0;
        }
        if (len > (4 - (addr - align_addr))) {
            copy_len = 4 - (addr - align_addr);
        } else {
            copy_len = len;
        }
        memcpy(pdata, read_allign_buf + (addr - align_addr), copy_len);

        addr += copy_len;

        pdata += copy_len;

        len -= copy_len;
    }

    copy_len = len / 4 * 4;

    if (copy_len) {
        ret_val = ef_log_read(ctx, addr, (uint32_t *) pdata, &copy_len);

        if (ret_val != 0) {
            return (pdata - (uint8_t *) buf);
        }

        addr += copy_len;

        pdata += copy_len;

        len -= copy_len;
    }

    if (len) {
        copy_len = 4;
        ret_val = ef_log_read(ctx, addr, (uint32_t *) read_allign_buf, &copy_len);
        if (ret_val != 0) {
            return (pdata - (uint8_t *) buf);
        }

        memcpy(pdata, read_allign_buf, len);

        pdata += len;
    }

    return (pdata - (uint8_t *) buf);
}

int flash_buffer_clear(struct flash_buffer_ctx *ctx)
{
    return ef_log_clean(ctx);
}
