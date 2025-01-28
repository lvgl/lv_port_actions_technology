/*
 * Copyright (c) 2017 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_LEVEL CONFIG_MMC_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(sdio_acts);

#include <kernel.h>
#include <init.h>
#include <device.h>
#include <irq.h>
#include <drivers/mmc/mmc.h>
#include <drivers/mmc/sdio.h>
#include <soc.h>
#include "mmc_ops.h"


#define MAX_FUNC_DEVICE_NUM		1
#define SDIO_CMD_TIMEOUT_US		(200 * 1000)

struct sdio_func
{
	struct device		*host;		/* the card this device belongs to */
#ifdef CONFIG_WIFI_RTL8189FTV
	sdio_irq_handler_t	*callback;
	unsigned int		max_blksize;	/* maximum block size */ //add
	unsigned int		cur_blksize;	/* current block size */
	unsigned int		enable_timeout;	/* max enable timeout in msec */ //add
	unsigned int		num;		/* function number */
	struct k_mutex		host_lock;
#else
	struct k_mutex          host_lock;
	sdio_irq_handler_t      *callback;
	unsigned int            num;            /* function number */
	unsigned                cur_blksize;    /* current block size */
#endif
	uint32_t		rca;		/* device rca */
	bool			dev_scan_done; /* scan and find device */

#ifdef CONFIG_MMC_SDIO_ISR_THREAD
	k_tid_t			sdio_isr_thread_pid;
#endif
};

#ifdef CONFIG_MMC_SDIO_ISR_THREAD
static struct k_thread sdio_isr_thread;
#ifdef CONFIG_WIFI_RTL8189FTV
static K_THREAD_STACK_DEFINE(sdio_isr_thread_stack, 3096);
#else
static K_THREAD_STACK_DEFINE(sdio_isr_thread_stack, 896);
#endif
static struct k_sem sdio_isr_thread_sem;
#endif

#if defined(CONFIG_WIFI_RTL8189FTV) && defined(CONFIG_MMC_SDIO_ISR_THREAD)
#define CHECK_SDIO_IRQ_INTERVAL		2000		/* 2s */
static void sdio_check_irq_timer_callback(struct k_timer *timer);
K_TIMER_DEFINE(check_irq_timer, sdio_check_irq_timer_callback, NULL);
static uint32_t sdio_irq_pre_cnt = 0, sdio_irq_cur_cnt = 0;

static void sdio_check_irq_timer_callback(struct k_timer *timer)
{
	if (sdio_irq_cur_cnt == sdio_irq_pre_cnt) {
		k_sem_give(&sdio_isr_thread_sem);
	} else {
		sdio_irq_pre_cnt = sdio_irq_cur_cnt;
	}
}
#endif

int mmc_send_io_op_cond(struct device *mmc_dev, uint32_t ocr, uint32_t *rocr)
{
	struct mmc_cmd cmd = {0};
	int i, err = 0;

	cmd.opcode = SD_IO_SEND_OP_COND;
	cmd.arg = ocr;
	cmd.flags = MMC_RSP_R4 | MMC_CMD_BCR;

	for (i = 10; i; i--) {
		err = mmc_send_cmd(mmc_dev, &cmd);
		if (err)
			break;

		/* if we're just probing, do a single pass */
		if (ocr == 0)
			break;

		if (cmd.resp[0] & MMC_CARD_BUSY)
			break;

		err = -ETIMEDOUT;

		k_sleep(10);
	}

	if (rocr)
		*rocr = cmd.resp[0];

	return err;
}

int mmc_io_rw_direct(struct device *mmc_dev, int write, unsigned fn,
	unsigned addr, uint8_t in, uint8_t *out)
{
	struct mmc_cmd cmd = {0};
	uint32_t arg;
	int ret;

	/* sanity check */
	if (addr & ~0x1ffff)
		return -EINVAL;

	/*
	 * SD_IO_RW_DIRECT argument format:
	 *
	 *      [31] R/W flag
	 *      [30:28] Function number
	 *      [27] RAW flag
	 *      [25:9] Register address
	 *      [7:0] Data
	 */
	arg = write ? 0x80000000 : 0x00000000;
	arg |= fn << 28;
	arg |= (write && out) ? 0x08000000 : 0x00000000;
	arg |= addr << 9;
	arg |= in;

	cmd.opcode = SD_IO_RW_DIRECT;
	cmd.arg = arg;
	cmd.flags = MMC_RSP_R5 | MMC_CMD_AC;

	ret = mmc_send_cmd(mmc_dev, &cmd);
	if (ret) {
		LOG_ERR("direct r/w failed, ret %d", ret);
		return ret;
	}

	if (cmd.resp[0] & (R5_ERROR | R5_FUNCTION_NUMBER | R5_OUT_OF_RANGE)) {
		LOG_ERR("direct r/w failed, resp 0x%x", cmd.resp[0]);
		return -1;
	}

	if (out)
		*out = cmd.resp[0] & 0xff;

	return 0;
}

int mmc_io_rw_extended(struct device *mmc_dev, int write, uint32_t fn, uint32_t addr,
		uint32_t incr_addr, uint8_t *buf, uint32_t blocks, uint32_t blksz)
{
	struct mmc_cmd cmd = {0};
	uint32_t arg;
	int ret;

	/* sanity check */
	if (addr & ~0x1ffff)
		return -EINVAL;

	/*
	 * SD_IO_RW_EXTENDED argument format:
	 *
	 *      [31] R/W flag
	 *      [30:28] Function number
	 *      [27] Block mode
	 *      [26] Increment address
	 *      [25:9] Register address
	 *      [8:0] Byte/block count
	 */
	arg = write ? 0x80000000 : 0x00000000;
	arg |= fn << 28;
	arg |= incr_addr ? 0x04000000 : 0x00000000;
	arg |= addr << 9;
	if (blocks == 0)
		arg |= (blksz == 512) ? 0 : blksz;	/* byte mode */
	else
		arg |= 0x08000000 | blocks;		/* block mode */

	cmd.opcode = SD_IO_RW_EXTENDED;
	cmd.arg = arg;
	cmd.blk_size = blksz;
	cmd.blk_num = (blocks == 0) ? 1 : blocks;
	cmd.buf = buf;

	cmd.flags = MMC_RSP_R5 | MMC_CMD_ADTC;
	cmd.flags |= write ? MMC_DATA_WRITE : MMC_DATA_READ;

	ret = mmc_send_cmd(mmc_dev, &cmd);
	if (ret) {
		LOG_ERR("sdio extended r/w failed, ret %d\n", ret);
		return ret;
	}

	if (cmd.resp[0] & (R5_ERROR | R5_FUNCTION_NUMBER | R5_OUT_OF_RANGE)) {
		LOG_ERR("sdio direct r/w failed, resp 0x%x\n", cmd.resp[0]);
		return -1;
	}

	return 0;
}

/*
 * Test if the card supports high-speed mode and, if so, switch to it.
 */
static int mmc_sdio_switch_hs(struct device *mmc_dev, int enable)
{
	int ret;
	uint8_t speed;

	ret = mmc_io_rw_direct(mmc_dev, 0, 0, SDIO_CCCR_SPEED, 0, &speed);
	if (ret)
		return ret;

	if (!(speed & SDIO_SPEED_SHS)) {
		return 0;
	}

	if (enable)
		speed |= SDIO_SPEED_EHS;
	else
		speed &= ~SDIO_SPEED_EHS;

	ret = mmc_io_rw_direct(mmc_dev, 1, 0, SDIO_CCCR_SPEED, speed, NULL);
	if (ret)
		return ret;

	return 1;
}

static int sdio_enable_4bit_bus(struct device *mmc_dev)
{
	int ret;
	uint32_t caps;
	uint8_t ctrl;

	caps = mmc_get_capability(mmc_dev);
	if (!(caps & MMC_CAP_4_BIT_DATA))
		return 0;

	ret = mmc_io_rw_direct(mmc_dev, 0, 0, SDIO_CCCR_IF, 0, &ctrl);
	if (ret)
		return ret;

	/* set as 4-bit bus width */
	ctrl &= ~SDIO_BUS_WIDTH_MASK;
	ctrl |= SDIO_BUS_WIDTH_4BIT;

	ret = mmc_io_rw_direct(mmc_dev, 1, 0, SDIO_CCCR_IF, ctrl, NULL);
	if (ret)
		return ret;

	ret = mmc_io_rw_direct(mmc_dev, 0, 0, SDIO_CCCR_IF, 0, &ctrl);
	if (ret)
		return ret;

	return 1;
}

/**
 *	sdio_claim_host - exclusively claim a bus for a certain SDIO function
 *	@func: SDIO function that will be accessed
 *
 *	Claim a bus for a set of operations. The SDIO function given
 *	is used to figure out which bus is relevant.
 */
void sdio_claim_host(struct sdio_func *func)
{
	if (!func->dev_scan_done) {
		return;
	}

	k_mutex_lock(&func->host_lock, K_FOREVER);
}

/**
 *	sdio_release_host - release a bus for a certain SDIO function
 *	@func: SDIO function that was accessed
 *
 *	Release a bus, allowing others to claim the bus for their
 *	operations.
 */
void sdio_release_host(struct sdio_func *func)
{
	if (!func->dev_scan_done) {
		return;
	}

	k_mutex_unlock(&func->host_lock);
}

/**
 *	sdio_enable_func - enables a SDIO function for usage
 *	@func: SDIO function to enable
 *
 *	Powers up and activates a SDIO function so that register
 *	access is possible.
 */
int sdio_enable_func(struct sdio_func *func)
{
	int ret, cnt = 0;
	unsigned char reg;

	LOG_DBG("Enabling function %d\n", func->num);

	if (!func->dev_scan_done) {
		return -1;
	}

	ret = mmc_io_rw_direct(func->host, 0, 0, SDIO_CCCR_IOEx, 0, &reg);
	if (ret)
		goto err;

	reg |= 1 << func->num;

	ret = mmc_io_rw_direct(func->host, 1, 0, SDIO_CCCR_IOEx, reg, NULL);
	if (ret)
		goto err;

	while (1) {
		ret = mmc_io_rw_direct(func->host, 0, 0, SDIO_CCCR_IORx, 0, &reg);
		if (ret)
			goto err;
		if (reg & (1 << func->num))
			break;

		/* wait 1us */
		k_busy_wait(1);
		if (++cnt > SDIO_CMD_TIMEOUT_US)
			goto err;
	}

	return 0;

err:
	LOG_DBG("Failed to enable function %d", func->num);
	return ret;
}

/**
 *	sdio_disable_func - disable a SDIO function
 *	@func: SDIO function to disable
 *
 *	Powers down and deactivates a SDIO function. Register access
 *	to this function will fail until the function is reenabled.
 */
int sdio_disable_func(struct sdio_func *func)
{
	int ret;
	unsigned char reg;

	if (!func->dev_scan_done) {
		return -1;
	}

	LOG_DBG("Disabling function %d", func->num);

	ret = mmc_io_rw_direct(func->host, 0, 0, SDIO_CCCR_IOEx, 0, &reg);
	if (ret)
		goto err;

	reg &= ~(1 << func->num);

	ret = mmc_io_rw_direct(func->host, 1, 0, SDIO_CCCR_IOEx, reg, NULL);
	if (ret)
		goto err;

	return 0;

err:
	LOG_DBG("Failed to disable function %d", func->num);
	return -EIO;
}

/**
 *	sdio_set_block_size - set the block size of an SDIO function
 *	@func: SDIO function to change
 *	@blksz: new block size or 0 to use the default.
 *
 *	The default block size is the largest supported by both the function
 *	and the host, with a maximum of 512 to ensure that arbitrarily sized
 *	data transfer use the optimal (least) number of commands.
 *
 *	A driver may call this to override the default block size set by the
 *	core. This can be used to set a block size greater than the maximum
 *	that reported by the card; it is the driver's responsibility to ensure
 *	it uses a value that the card supports.
 *
 *	Returns 0 on success, -EINVAL if the host does not support the
 *	requested block size, or -EIO (etc.) if one of the resultant FBR block
 *	size register writes failed.
 *
 */
int sdio_set_block_size(struct sdio_func *func, unsigned blksz)
{
	int ret;

	LOG_DBG("function %d set blksize %d", func->num, blksz);

	if (!func->dev_scan_done) {
		return -1;
	}

	ret = mmc_io_rw_direct(func->host, 1, 0,
		SDIO_FBR_BASE(func->num) + SDIO_FBR_BLKSIZE,
		blksz & 0xff, NULL);
	if (ret)
		return ret;
	ret = mmc_io_rw_direct(func->host, 1, 0,
		SDIO_FBR_BASE(func->num) + SDIO_FBR_BLKSIZE + 1,
		(blksz >> 8) & 0xff, NULL);
	if (ret)
		return ret;

	func->cur_blksize = blksz;
#ifdef CONFIG_WIFI_RTL8189FTV
	func->max_blksize = blksz;
#endif

	return 0;
}

/* Split an arbitrarily sized data transfer into several
 * IO_RW_EXTENDED commands. */
static int sdio_io_rw_ext_helper(struct sdio_func *func, int write,
	unsigned addr, int incr_addr, unsigned char *buf, unsigned size)
{
	unsigned remainder = size;
	unsigned max_blocks = 511;
	int ret;

	if (!func->dev_scan_done) {
		return -1;
	}

	/* Do the bulk of the transfer using block mode (if supported). */
	if (size > func->cur_blksize) {
		while (remainder >= func->cur_blksize) {
			unsigned blocks;

			blocks = remainder / func->cur_blksize;
			if (blocks > max_blocks)
				blocks = max_blocks;
			size = blocks * func->cur_blksize;

			ret = mmc_io_rw_extended(func->host, write,
				func->num, addr, incr_addr, buf,
				blocks, func->cur_blksize);
			if (ret)
				return ret;

			remainder -= size;
			buf += size;
			if (incr_addr)
				addr += size;
		}
	}

	/* Write the remainder using byte mode. */
	while (remainder > 0) {
		size = min(remainder, func->cur_blksize);

		/* Indicate byte mode by setting "blocks" = 0 */
		ret = mmc_io_rw_extended(func->host, write, func->num, addr,
			 incr_addr, buf, 0, size);
		if (ret)
			return ret;

		remainder -= size;
		buf += size;
		if (incr_addr)
			addr += size;
	}
	return 0;
}

/**
 *	sdio_readb - read a single byte from a SDIO function
 *	@func: SDIO function to access
 *	@addr: address to read
 *	@err_ret: optional status value from transfer
 *
 *	Reads a single byte from the address space of a given SDIO
 *	function. If there is a problem reading the address, 0xff
 *	is returned and @err_ret will contain the error code.
 */
unsigned char sdio_readb(struct sdio_func *func, unsigned int addr, int *err_ret)
{
	int ret;
	unsigned char val;

	if (!func->dev_scan_done) {
		return -1;
	}

	if (err_ret)
		*err_ret = 0;

	ret = mmc_io_rw_direct(func->host, 0, func->num, addr, 0, &val);
	if (ret) {
		if (err_ret)
			*err_ret = ret;
		return 0xFF;
	}

	return val;
}

/**
 *	sdio_writeb - write a single byte to a SDIO function
 *	@func: SDIO function to access
 *	@b: byte to write
 *	@addr: address to write to
 *	@err_ret: optional status value from transfer
 *
 *	Writes a single byte to the address space of a given SDIO
 *	function. @err_ret will contain the status of the actual
 *	transfer.
 */
void sdio_writeb(struct sdio_func *func, unsigned char b, unsigned int addr,
		 int *err_ret)
{
	int ret;

	if (!func->dev_scan_done) {
		return;
	}

	ret = mmc_io_rw_direct(func->host, 1, func->num, addr, b, NULL);
	if (err_ret)
		*err_ret = ret;
}

/**
 *	sdio_memcpy_fromio - read a chunk of memory from a SDIO function
 *	@func: SDIO function to access
 *	@dst: buffer to store the data
 *	@addr: address to begin reading from
 *	@count: number of bytes to read
 *
 *	Reads from the address space of a given SDIO function. Return
 *	value indicates if the transfer succeeded or not.
 */
int sdio_memcpy_fromio(struct sdio_func *func, void *dst,
	unsigned int addr, int count)
{
	if (!func->dev_scan_done) {
		return -1;
	}

	return sdio_io_rw_ext_helper(func, 0, addr, 1, dst, count);
}

/**
 *	sdio_memcpy_toio - write a chunk of memory to a SDIO function
 *	@func: SDIO function to access
 *	@addr: address to start writing to
 *	@src: buffer that contains the data to write
 *	@count: number of bytes to write
 *
 *	Writes to the address space of a given SDIO function. Return
 *	value indicates if the transfer succeeded or not.
 */
int sdio_memcpy_toio(struct sdio_func *func, unsigned int addr,
	void *src, int count)
{
	if (!func->dev_scan_done) {
		return -1;
	}

	return sdio_io_rw_ext_helper(func, 1, addr, 1, src, count);
}

/**
 *	sdio_f0_readb - read a single byte from SDIO function 0
 *	@func: an SDIO function of the card
 *	@addr: address to read
 *	@err_ret: optional status value from transfer
 *
 *	Reads a single byte from the address space of SDIO function 0.
 *	If there is a problem reading the address, 0xff is returned
 *	and @err_ret will contain the error code.
 */
unsigned char sdio_f0_readb(struct sdio_func *func, unsigned int addr,
	int *err_ret)
{
	int ret;
	unsigned char val;

	if (!func->dev_scan_done) {
		return -1;
	}

	if (err_ret)
		*err_ret = 0;

	ret = mmc_io_rw_direct(func->host, 0, 0, addr, 0, &val);
	if (ret) {
		if (err_ret)
			*err_ret = ret;
		return 0xFF;
	}

	return val;
}

/**
 *	sdio_f0_writeb - write a single byte to SDIO function 0
 *	@func: an SDIO function of the card
 *	@b: byte to write
 *	@addr: address to write to
 *	@err_ret: optional status value from transfer
 *
 *	Writes a single byte to the address space of SDIO function 0.
 *	@err_ret will contain the status of the actual transfer.
 *
 *	Only writes to the vendor specific CCCR registers (0xF0 -
 *	0xFF) are permiited; @err_ret will be set to -EINVAL for *
 *	writes outside this range.
 */
void sdio_f0_writeb(struct sdio_func *func, unsigned char b, unsigned int addr,
	int *err_ret)
{
	int ret;

	ret = mmc_io_rw_direct(func->host, 1, 0, addr, b, NULL);
	if (err_ret)
		*err_ret = ret;
}

static void sdio_isr_handler(void *arg)
{
	struct sdio_func *func = arg;

	mmc_enable_sdio_irq(func->host, 0);

#ifdef CONFIG_MMC_SDIO_ISR_THREAD
	k_sem_give(&sdio_isr_thread_sem);
#else
	if (func->sdio_func_callback) {
		func->sdio_func_callback(func);
		mmc_enable_sdio_irq(func->host, 1);
	}
#endif
}

#ifdef CONFIG_MMC_SDIO_ISR_THREAD
static void sdio_isr_thread_func(void *p1, void *p2, void *p3)
{
	struct sdio_func *func = p1;

	LOG_DBG("irq thread is ready");

	while (1) {
		k_sem_take(&sdio_isr_thread_sem, K_FOREVER);
#ifdef CONFIG_WIFI_RTL8189FTV
		sdio_irq_cur_cnt++;
#endif

		if (func->callback) {
			func->callback(func);
			mmc_enable_sdio_irq(func->host, 1);
		}
	}
}
#endif

/**
 *	sdio_claim_irq - claim the IRQ for a SDIO function
 *	@func: SDIO function
 *	@handler: IRQ handler callback
 *
 *	Claim and activate the IRQ for the given SDIO function. The provided
 *	handler will be called when that IRQ is asserted.  The host is always
 *	claimed already when the handler is called so the handler must not
 *	call sdio_claim_host() nor sdio_release_host().
 */
int sdio_claim_irq(struct sdio_func *func, sdio_irq_handler_t *handler)
{
	int err;
	unsigned char reg;

	LOG_DBG("Enabling IRQ for function %d", func->num);

	reg = sdio_f0_readb(func, SDIO_CCCR_IENx, &err);
	if (err) {
		return err;
	}

	reg |= 1 << func->num;
	 /* Master interrupt enable */
	reg |= 1;

	sdio_f0_writeb(func, reg, SDIO_CCCR_IENx, &err);
	if (err) {
		return err;
	}

#ifdef CONFIG_MMC_SDIO_ISR_THREAD
	k_sem_init(&sdio_isr_thread_sem, 0, 1);

	func->sdio_isr_thread_pid = k_thread_create(&sdio_isr_thread,
		sdio_isr_thread_stack,
		K_THREAD_STACK_SIZEOF(sdio_isr_thread_stack),
		(k_thread_entry_t) sdio_isr_thread_func, func,
		NULL, NULL, K_PRIO_COOP(1), 0, K_NO_WAIT);
#endif

	func->callback = handler;
	mmc_set_sdio_irq_callback(func->host, sdio_isr_handler, func);
	mmc_enable_sdio_irq(func->host, 1);
#if defined(CONFIG_WIFI_RTL8189FTV) && defined(CONFIG_MMC_SDIO_ISR_THREAD)
	k_timer_start(&check_irq_timer, CHECK_SDIO_IRQ_INTERVAL, CHECK_SDIO_IRQ_INTERVAL);
#endif

	return err;
}

/**
 *	sdio_release_irq - release the IRQ for a SDIO function
 *	@func: SDIO function
 *
 *	Disable and release the IRQ for the given SDIO function.
 */
int sdio_release_irq(struct sdio_func *func)
{
	int err;
	unsigned char reg;

	LOG_DBG("Disabling IRQ for function %d", func->num);

#if defined(CONFIG_WIFI_RTL8189FTV) && defined(CONFIG_MMC_SDIO_ISR_THREAD)
	k_timer_stop(&check_irq_timer);
#endif

	mmc_enable_sdio_irq(func->host, 0);
#ifdef CONFIG_MMC_SDIO_ISR_THREAD
	k_thread_abort(func->sdio_isr_thread_pid);
	func->sdio_isr_thread_pid = NULL;
#endif
	func->callback = NULL;
	mmc_set_sdio_irq_callback(func->host, NULL, NULL);

	reg = sdio_f0_readb(func, SDIO_CCCR_IENx, &err);
	if (err) {
		return err;
	}

	reg &= ~(1 << func->num);

	/* Disable master interrupt with the last function interrupt */
	if (!(reg & 0xFE))
		reg = 0;

	sdio_f0_writeb(func, reg, SDIO_CCCR_IENx, &err);
	if (err) {
		return err;
	}

	return 0;
}

static int sdio_scan_host(struct device *mmc_dev, struct sdio_func *func)
{
	int err;
	uint32_t ocr;

	mmc_go_idle(mmc_dev);

	err = mmc_send_io_op_cond(mmc_dev, 0, &ocr);
	if (err) {
		LOG_ERR("cannot found sdio device");
		return -ENODEV;
	}

	if (((ocr >> 28) & 0x7) == 0) {
		LOG_ERR("cannot found any sdio function");
		return -ENODEV;
	}

	ocr = ocr & 0xffffff;
	if (!ocr) {
		LOG_ERR("invalid ocr 0x%x", ocr);
		return -ENODEV;
	}

	err = mmc_send_io_op_cond(mmc_dev, ocr, &ocr);
	if (err) {
		LOG_ERR("cannot found sdio device");
		return -ENODEV;
	}

	err = mmc_send_relative_addr(mmc_dev, &func->rca);
	if (err) {
		LOG_ERR("failed to get rca");
		return err;
	}

	err = mmc_select_card(mmc_dev, func->rca);
	if (err) {
		LOG_ERR("failed to select card %d\n", func->rca);
		return err;
	}

	err = mmc_sdio_switch_hs(mmc_dev, true);
	if (err > 0) {
		mmc_set_clock(mmc_dev, 50000000);
	}

	/* set bus width */
	err = sdio_enable_4bit_bus(mmc_dev);
	if (err > 0) {
		mmc_set_bus_width(mmc_dev, MMC_BUS_WIDTH_4);
		err = 0;
	}

	func->dev_scan_done = true;

	return  0;
}

static struct sdio_func sdio_funcs[MAX_FUNC_DEVICE_NUM];

struct sdio_func *sdio_get_func(int index)
{
	struct sdio_func *func;

	if (index > MAX_FUNC_DEVICE_NUM)
		return NULL;

	func = &sdio_funcs[index];

	if (!func->dev_scan_done) {
		return NULL;
	}

	return func;
}

int sdio_bus_scan(int bus_id)
{
	int ret;
	struct sdio_func *func = &sdio_funcs[0];

	ARG_UNUSED(bus_id);

	LOG_INF("scan sdio on mmc bus %s",CONFIG_MMC_SDIO_MMC_DEV_NAME);

	func->host = device_get_binding(CONFIG_MMC_SDIO_MMC_DEV_NAME);
	if (!func->host) {
		LOG_ERR("Cannot find mmc device %s!\n",
			    CONFIG_MMC_SDIO_MMC_DEV_NAME);
		return -ENODEV;
	}

	func->callback = NULL;
	func->num = 1;
	func->cur_blksize = 0;
	func->dev_scan_done = false;

	k_mutex_init(&func->host_lock);

	ret = sdio_scan_host(func->host, func);
	if (ret) {
		LOG_ERR("host scan error, ret %d", ret);
		return ret;
	}

	return 0;
}

static int sdio_init(struct device *dev)
{
	/* Do nothing */
	ARG_UNUSED(dev);

	return 0;
}

DEVICE_DEFINE(sdio, "sdio_0", sdio_init, NULL,
	    NULL, NULL, POST_KERNEL, 20, NULL);
