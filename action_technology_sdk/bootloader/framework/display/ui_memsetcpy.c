#include <os_common_api.h>
#include <drivers/dma.h>
#include <drivers/cfg_drv/dev_config.h>
#include <display/ui_memsetcpy.h>

static const struct device * dma_dev;
static int dma_chan;
static struct dma_config dma_cfg;
static struct dma_block_config dma_block_cfg;
static K_SEM_DEFINE(dma_sem, 0, 1);

static void dma_complete_callback(const struct device *dev, void *user_data,
		uint32_t channel, int status)
{
	k_sem_give(&dma_sem);
}

static int dma_transfer_init(void)
{
	if (dma_dev) {
		return 0;
	}

	dma_dev = device_get_binding(CONFIG_DMA_0_NAME);
	if (dma_dev == NULL) {
		SYS_LOG_ERR(CONFIG_DMA_0_NAME " not found");
		return -ENODEV;
	}

	dma_chan = dma_request(dma_dev, 0xFF);
	if (dma_chan < 0) {
		SYS_LOG_ERR("dma_request failed");
		dma_dev = NULL;
		return -ENODEV;
	}

	dma_cfg.complete_callback_en = 1;
	dma_cfg.dma_callback = dma_complete_callback;
	dma_cfg.dma_slot = 0;
	dma_cfg.dest_data_size = 4;
	dma_cfg.block_count = 1;
	dma_cfg.head_block = &dma_block_cfg;

	return 0;
}

static int _memset32(void * buf, uint32_t c32, size_t n)
{
	static uint32_t dma_c32 __in_section_unique(ram.noinit);

	if (ui_memsetcpy_wait_finish(5000)) {
		k_sem_reset(&dma_sem);
	}

	dma_c32 = c32;

	dma_cfg.channel_direction = PERIPHERAL_TO_MEMORY;
	dma_block_cfg.block_size = n;
	dma_block_cfg.source_address = (uint32_t)&dma_c32;
	dma_block_cfg.dest_address = (uint32_t)buf;

	if (dma_config(dma_dev, dma_chan, &dma_cfg)) {
		SYS_LOG_ERR("dma%d config error\n", dma_chan);
		dma_block_cfg.block_size = 0;
		return -1;
	}

	if (dma_start(dma_dev, dma_chan)) {
		SYS_LOG_ERR("dma%d start error\n", dma_chan);
		dma_block_cfg.block_size = 0;
		return -1;
	}

	return 0;
}

void ui_memset(void * buf, uint8_t c, size_t n)
{
	if (((uintptr_t)buf & 0x3) || (n & 0x3)) {
		memset(buf, c, n);
		return;
	}

	if (dma_transfer_init()) {
		memset(buf, c, n);
		return;
	}

	uint32_t c32 = c | ((uint32_t)c << 8) | ((uint32_t)c << 16) | ((uint32_t)c << 24);

	if (_memset32(buf, c32, n)) {
		memset(buf, c, n);
	}
}

int ui_memset16(void * buf, uint16_t c16, size_t n16)
{
	if (((uintptr_t)buf & 0x3) || (n16 & 0x1)) {
		return -EINVAL;
	}

	if (dma_transfer_init()) {
		return -EINVAL;
	}

	uint32_t c32 = c16 | ((uint32_t)c16 << 16);

	return _memset32(buf, c32, n16 * 2);
}

int ui_memset32(void * buf, uint32_t c32, size_t n32)
{
	if ((uintptr_t)buf & 0x3) {
		return -EINVAL;
	}

	if (dma_transfer_init()) {
		return -EINVAL;
	}

	return _memset32(buf, c32, n32 * 4);
}

void ui_memcpy(void * dest, const void * src, size_t n)
{
	if (((uintptr_t)dest & 0x3) || ((uintptr_t)src & 0x3) || (n & 0x3)) {
		memcpy(dest, src, n);
		return;
	}

	if (dma_transfer_init()) {
		memcpy(dest, src, n);
		return;
	}

	if (ui_memsetcpy_wait_finish(5000)) {
		k_sem_reset(&dma_sem);
	}

	dma_cfg.channel_direction = MEMORY_TO_MEMORY;
	dma_block_cfg.block_size = n;
	dma_block_cfg.source_address = (uint32_t)src;
	dma_block_cfg.dest_address = (uint32_t)dest;

	if (dma_config(dma_dev, dma_chan, &dma_cfg)) {
		SYS_LOG_ERR("dma%d config error\n", dma_chan);
		dma_block_cfg.block_size = 0;
		memcpy(dest, src, n);
		return;
	}

	if (dma_start(dma_dev, dma_chan)) {
		SYS_LOG_ERR("dma%d start error\n", dma_chan);
		dma_block_cfg.block_size = 0;
		memcpy(dest, src, n);
		return;
	}
}

int ui_memsetcpy_wait_finish(int timeout_ms)
{
	int res = 0;

	if (dma_dev == NULL) {
		return 0;
	}

	if (dma_block_cfg.block_size > 0) {
		if (timeout_ms < 0) {
			res = k_sem_take(&dma_sem, K_FOREVER);
		} else {
			res = k_sem_take(&dma_sem, K_MSEC(timeout_ms));
		}

		if (res == 0) {
			dma_block_cfg.block_size = 0;
		}
	}

	return res;
}
