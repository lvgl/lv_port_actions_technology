/*
 * Copyright (c) 2020 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Audio OUTPUT Channels (DAC/I2S-TX/SPDIF-TX/PDM-TX) core management layer.
 */

#include <kernel.h>
#include <device.h>
#include <ksched.h>
#include <string.h>
#include <errno.h>
#include <soc.h>
#include <board_cfg.h>
#include <drivers/dma.h>
#include <drivers/audio/audio_out.h>
#include "phy_audio_common.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(aout, LOG_LEVEL_DBG);

#ifdef CONFIG_BOARD_EXTERNAL_PA_ENABLE
#include <board.h>
#endif

/* Audio out session resource definitions */
#define AOUT_SESSION_MAGIC                 (0x1a2b3c4d)
#define AOUT_SESSION_CHECK_MAGIC(x)        ((x) == AOUT_SESSION_MAGIC)

/* Audio output channel sessions which depends on different chipsets */
#ifdef CONFIG_SOC_SERIES_PEARLRIVER
#define AOUT_DAC_SESSION_MAX               (0) /* 0 DAC */
#define AOUT_I2STX_SESSION_MAX             (1) /* 1 I2STX */
#define AOUT_SPDIFTX_SESSION_MAX           (0) /* 0 SPDIFTX */
#define AOUT_PDMTX_SESSION_MAX             (1) /* 1 PDMTX */
#else

#ifdef CONFIG_AUDIO_OUT_DAC_SUPPORT
#define AOUT_DAC_SESSION_MAX               (2) /* 2 DAC */
#else
#define AOUT_DAC_SESSION_MAX               (0)
#endif

#ifdef CONFIG_AUDIO_OUT_I2STX_SUPPORT
#define AOUT_I2STX_SESSION_MAX             (2) /* 1 I2STX */
#else
#define AOUT_I2STX_SESSION_MAX             (0)
#endif

#ifdef CONFIG_AUDIO_OUT_SPDIFTX_SUPPORT
#define AOUT_SPDIFTX_SESSION_MAX           (2) /* 1 SPDIFTX */
#else
#define AOUT_SPDIFTX_SESSION_MAX           (0)
#endif

#define AOUT_PDMTX_SESSION_MAX             (0) /* 0 PDMTX */
#endif

#define AOUT_SESSION_MAX                   (AOUT_DAC_SESSION_MAX + AOUT_I2STX_SESSION_MAX + \
											AOUT_SPDIFTX_SESSION_MAX + AOUT_PDMTX_SESSION_MAX) /* totally max sessions */

#define AOUT_SESSION_OPEN                  (1 << 0) /* session open flag */
#define AOUT_SESSION_CONFIG                (1 << 1) /* session config flag */
#define AOUT_SESSION_START                 (1 << 2) /* session start flag */

#define AOUT_SESSION_WAIT_PA_TIMEOUT       (2000) /* timeout of waitting pa session */

#define DMA_IRQ_TC                         (0) /* DMA completion flag */
#define DMA_IRQ_HF                         (1) /* DMA half-full flag */

/* audio out io-commands following by the AUDIO FIFO usage */
#define AOUT_IS_FIFO_CMD(x) ((x) & AOUT_FIFO_CMD_FLAG)

/**
 * struct aout_dynamic_debug_t
 * @brief audio out session dynamic debug structure
 */
typedef struct {
	uint32_t timestamp; /* record the per-second timestamp */
	uint32_t per_second_size; /* record the per-second play size */
	uint8_t dump_len; /* show the length of buffer per-second */
	uint8_t performance: 1; /* enable flag of showing the play performance */
} aout_dynamic_debug_t;

/**
 * struct aout_session_t
 * @brief audio out session structure
 */
typedef struct {
	int magic; /* session magic value as AOUT_SESSION_MAGIC */
	int dma_chan; /* DMA channel handler */
	audio_outfifo_sel_e outfifo_type; /* indicator of the audio output fifo type */
	uint16_t channel_type; /* indicator of the channel type selection */
	uint16_t flags; /* session flags */
	int (*callback)(void *cb_data, uint32_t reason); /* callback function of the channel data link */
	void *cb_data; /* callback user data */
	uint8_t *reload_addr; /* reload buffer address to transfer */
	uint32_t reload_len;  /* the length of the reload buffer */
	uint8_t sample_rate; /* the channel sample rate setting */
	uint8_t id; /* the session id */
	uint8_t dma_width; /* dma width */
#ifdef CONFIG_AUDIO_DYNAMIC_DEBUG
	aout_dynamic_debug_t debug; /* dynamic debug */
#endif
	uint8_t reload_en : 1; /* the flag of reload mode enable or not */
	uint8_t dma_separated_en : 1; /* the flag of DMA interleaved mode enable or not */
    uint8_t dsp_fifo_src : 1;
} aout_session_t;

/**
 * struct aout_drv_data
 * @brief The software related data that used by audio out driver.
 */
struct aout_drv_data {
	struct k_sem lock; /* the semaphore to lock the channel resource */
	struct device *dma_dev; /* DMA device handler */
	struct device *dac0_dev; /* DAC0 device handler */
	struct device *i2stx0_dev; /* I2STX0 device handler */
	struct device *spdiftx0_dev; /* SPFITX0 device handler */
	struct device *pdmtx0_dev; /* PDMTX0 device handler */
	#ifdef CONFIG_AUDIO_PA_AW87390_SUPPORT
	struct device *pa_aw87390_dev;
	#endif
	uint8_t pa_session_indicator : 1; /* indicates the stage of the pa session operations */
	uint8_t dac_fifo_ref_counter : 2; /* DAC FIFO request by other's module reference counter */
	uint8_t external_pa_opened : 1; /* If 1 to indicate the external PA has already opened */
};

/**
 * struct aout_config_data
 * @brief The hardware related data that used by audio out driver.
 */
struct aout_config_data {
#ifdef CONFIG_AUDIO_OUT_DAC_SUPPORT
	const char *dac0_name; /* physical DAC0 device name */
#endif

#ifdef CONFIG_AUDIO_OUT_I2STX_SUPPORT
	const char *i2stx0_name; /* physical I2STX0 device name */
#endif

#ifdef CONFIG_AUDIO_OUT_SPDIFTX_SUPPORT
	const char *spdiftx0_name; /* physical SPDIFTX device name */
#endif

#ifdef CONFIG_AUDIO_OUT_PDMTX_SUPPORT
	const char *pdmtx0_name; /* physical PDMTX device name */
#endif
#ifdef CONFIG_AUDIO_PA_AW87390_SUPPORT
	const char *pa_aw87390_name;
#endif

};

/* audio out sessions */
static aout_session_t aout_sessions[AOUT_SESSION_MAX];

/*
 * @brief check if there is the same channel within audio out sessions.
 * Moreover check whether the session type is invalid or not.
 */
static bool audio_out_session_check(uint16_t type)
{
	int i;
	aout_session_t *sess = aout_sessions;
	uint8_t max_sess, sess_opened = 0;

	if (AUDIO_CHANNEL_DAC & type) {
		max_sess = AOUT_DAC_SESSION_MAX;
	} else if (AUDIO_CHANNEL_I2STX & type) {
		max_sess = AOUT_I2STX_SESSION_MAX;
	} else if (AUDIO_CHANNEL_SPDIFTX & type) {
		max_sess = AOUT_SPDIFTX_SESSION_MAX;
	} else if (AUDIO_CHANNEL_PDMTX & type) {
		max_sess = AOUT_PDMTX_SESSION_MAX;
	} else {
		LOG_ERR("Invalid session type %d", type);
		return true;
	}

	/* check if max session is 0 */
	if (!max_sess) {
		LOG_ERR("type#%d max session (%d) forbidden", type, max_sess);
		return true;
	}

	/* find the session that already opened */
	for (i = 0; i < AOUT_SESSION_MAX; i++, sess++) {
		if (AOUT_SESSION_CHECK_MAGIC(sess->magic)
			&& (sess->flags & AOUT_SESSION_OPEN)
			&& (sess->channel_type & type)) {
			sess_opened++;
			LOG_DBG("Found aout channel type %d in use", type);
		}
	}

	/* check if the number of opened sessions is bigger than the max limit */
	if (sess_opened >= max_sess) {
		LOG_ERR("channel type:%d session max %d and opened %d",
				type, max_sess, sess_opened);
		return true;
	}

	return false;
}

/*
 * @brief Get audio out session by specified channel type.
 */
static aout_session_t *audio_out_session_get(uint16_t type)
{
	aout_session_t *sess = aout_sessions;
	int i;

	if (audio_out_session_check(type))
		return NULL;

	for (i = 0; i < AOUT_SESSION_MAX; i++, sess++) {
		if (!(sess->flags & AOUT_SESSION_OPEN)) {
			memset(sess, 0, sizeof(aout_session_t));
			sess->magic = AOUT_SESSION_MAGIC;
			sess->flags = AOUT_SESSION_OPEN;
			sess->channel_type = type;
			sess->id = i;
            sess->dma_chan = -1;
            sess->dsp_fifo_src = 0;
			return sess;
		}
	}

	return NULL;
}

/*
 * @brief Put audio out session by given session address
 */
static void audio_out_session_put(aout_session_t *s)
{
	aout_session_t *sess = aout_sessions;
	int i;
	for (i = 0; i < AOUT_SESSION_MAX; i++, sess++) {
		if ((uint32_t)s == (uint32_t)sess) {
			memset(s, 0, sizeof(aout_session_t));
			break;
		}
	}
}

/* @brief audio out session lock */
static inline void audio_out_lock(struct device *dev)
{
	struct aout_drv_data *data = dev->data;
	k_sem_take(&data->lock, K_FOREVER);
}

/* @brief audio out session unlock */
static inline void audio_out_unlock(struct device *dev)
{
	struct aout_drv_data *data = dev->data;
	k_sem_give(&data->lock);
}

#if defined(CONFIG_AUDIO_OUT_DAC_SUPPORT) || defined(CONFIG_AUDIO_POWERON_OPEN_I2STX)
/* @brief the dummy callback for aout inner driver usage */
static int acts_audio_dummy_callback(void *cb_data, uint32_t reason)
{
	ARG_UNUSED(cb_data);
	ARG_UNUSED(reason);

	return 0;
}
#endif

#ifdef CONFIG_AUDIO_OUT_DAC_SUPPORT
/* @brief wait the pa operations finish */
static int acts_audio_wait_pa_session(struct device *dev, uint32_t timeout_ms)
{
	struct aout_drv_data *data = dev->data;
	uint32_t start_time, curr_time;

	start_time = k_cycle_get_32();
	while (data->pa_session_indicator) {
		curr_time = k_cycle_get_32();
		if (k_cyc_to_us_floor32(curr_time - start_time)
			>= (timeout_ms * 1000)) {
			LOG_ERR("wait pa session timeout");
			return -ETIMEDOUT;
		}

		/* PM works in IDLE thread and not allow to sleep */
		if (!z_is_idle_thread_object(_current))
			k_sleep(K_MSEC(2));
	}

	return 0;
}

/* @brief open a pa session to power charge the DAC digital and analog part */
static void *acts_audio_open_pa_session(struct device *dev)
{
	aout_param_t aout_setting = {0};
	dac_setting_t dac_setting = {0};
	void *handle;

	aout_setting.sample_rate = SAMPLE_RATE_48KHZ;
	aout_setting.channel_type = AUDIO_CHANNEL_DAC;
	aout_setting.channel_width = CHANNEL_WIDTH_16BITS;
	aout_setting.callback = acts_audio_dummy_callback;
	aout_setting.cb_data = (void *)dev;
	dac_setting.channel_mode = MONO_MODE;
	aout_setting.dac_setting = &dac_setting;

	handle = audio_out_open(dev, &aout_setting);

#ifdef CONFIG_BOARD_EXTERNAL_PA_ENABLE
	struct aout_drv_data *data = dev->data;
	if (handle)
		data->pa_session_indicator = 1;
#endif

	return handle;
}

/* @brief close the pa session */
static void acts_audio_close_pa_session(struct device *dev, void *hdl)
{
	audio_out_close(dev, hdl);
#ifdef CONFIG_BOARD_EXTERNAL_PA_ENABLE
	struct aout_drv_data *data = dev->data;
	data->pa_session_indicator = 0;
#endif
}

#ifdef CONFIG_BOARD_EXTERNAL_PA_ENABLE
/* @brief open external PA */
static void acts_audio_open_external_pa(struct device *dev, bool force)
{
	struct aout_drv_data *data = dev->data;
	if (((!data->external_pa_opened) && (!data->pa_session_indicator)) || (force)) {
		board_extern_pa_ctl(CONFIG_EXTERN_PA_CLASS, true);
		data->external_pa_opened = 1;
	}
}

/* @brief close external PA */
static void acts_audio_close_external_pa(struct device *dev)
{
	struct aout_drv_data *data = dev->data;

	if (data->external_pa_opened) {
		board_extern_pa_ctl(CONFIG_EXTERN_PA_CLASS, false);
		data->external_pa_opened = 0;
	}
}
#endif

/* @brief open the DAC internal and external PA */
static int acts_audio_open_pa(struct device *dev, bool open_exteranl_pa)
{
	void *pa_session = NULL;
	uint32_t pcm_data = 0;
	uint8_t i = 0;
	struct aout_drv_data *data = dev->data;

	phy_audio_control(data->dac0_dev, AOUT_CMD_OPEN_PA, NULL);

	pa_session = acts_audio_open_pa_session(dev);
	if (!pa_session) {
		LOG_ERR("Failed to open pa session");
		return -ENXIO;
	}

	/* FIXME: it is necessary to send some samples(zero data) when startup
	*   to avoid the noise with probability.
	*/
	for (i = 0; i < 16; i++) // TODO check this issue!!
		audio_out_write(dev, pa_session, (uint8_t *)&pcm_data, 4);

#ifdef CONFIG_BOARD_EXTERNAL_PA_ENABLE
	if (open_exteranl_pa)
		acts_audio_open_external_pa(dev, true);
#endif

	acts_audio_close_pa_session(dev, pa_session);

	LOG_INF("Open PA successfully");

	return 0;
}

/* @brief close DAC internal and external PA */
static int acts_audio_close_pa(struct device *dev)
{
	void *pa_session = NULL;
	struct aout_drv_data *data = dev->data;

	pa_session = acts_audio_open_pa_session(dev);
	if (!pa_session) {
		LOG_ERR("Failed to open pa session");
		return -ENXIO;
	}

	/* close external pa */
#ifdef CONFIG_BOARD_EXTERNAL_PA_ENABLE
	acts_audio_close_external_pa(dev);
#endif

	acts_audio_close_pa_session(dev, pa_session);

	/* close internal pa */
	phy_audio_control(data->dac0_dev, AOUT_CMD_CLOSE_PA, NULL);

	LOG_INF("Close PA successfully");

	return 0;
}

static int acts_audio_pa_class_select(struct device *dev, uint8_t pa_class)
{
	ARG_UNUSED(dev);
#ifdef CONFIG_BOARD_EXTERNAL_PA_ENABLE
	return board_extern_pa_ctl(pa_class, true);
#else
	return -1;
#endif
}
#endif

#ifdef CONFIG_AUDIO_POWERON_OPEN_I2STX
static int acts_audio_open_i2stx_dev(struct device *dev)
{
	aout_param_t aout_setting = {0};
	i2stx_setting_t i2stx_setting = {0};
	void *handle;

	aout_setting.sample_rate = SAMPLE_RATE_48KHZ;
	aout_setting.channel_type = AUDIO_CHANNEL_I2STX;
	aout_setting.channel_width = CHANNEL_WIDTH_16BITS;
	aout_setting.callback = acts_audio_dummy_callback;
	aout_setting.cb_data = (void *)dev;
	i2stx_setting.mode = I2S_MASTER_MODE;
	aout_setting.i2stx_setting = &i2stx_setting;

	handle = audio_out_open(dev, &aout_setting);

	/* close interface will not disable i2stx truly*/
	audio_out_close(dev, handle);

	return 0;
}

static int acts_audio_close_i2stx_dev(struct device *dev)
{
	struct aout_drv_data *data = dev->data;
	phy_audio_control(data->i2stx, AOUT_CMD_CLOSE_I2STX_DEVICE, NULL);

	return 0;
}
#endif

#ifdef CONFIG_AUDIO_DYNAMIC_DEBUG
/* @brief show the audio out performance */
static void audio_out_debug_perf(aout_session_t *session, uint8_t *buffer, uint32_t len)
{
	uint32_t delta;

	if (session->debug.performance) {
		delta = (uint32_t)k_cyc_to_ns_floor64(k_cycle_get_32() - session->debug.timestamp);
		session->debug.per_second_size += len;
		if (delta > 1000000000UL) {
			session->debug.timestamp = k_cycle_get_32();
			LOG_INF("** out session@%p - %dB/s **\n",
					session, session->debug.per_second_size);
			session->debug.per_second_size = 0;
			if (session->debug.dump_len) {
				AUDIO_DUMP_MEM(buffer, session->debug.dump_len);
			}
		}
	}
}

/* @brief control all sessions to enable or disable performance statistics */
static int audio_out_debug_perf_all_sessions(uint8_t is_en)
{
	int i;
	aout_session_t *sess = aout_sessions;

	for (i = 0; i < AOUT_SESSION_MAX; i++, sess++) {
		if (AOUT_SESSION_CHECK_MAGIC(sess->magic)
			&& (sess->flags & AOUT_SESSION_OPEN)) {
			LOG_INF("session@%p perf debug status %d", sess, is_en);
			sess->debug.performance = is_en;
			sess->debug.per_second_size = 0;
			sess->debug.timestamp = 0;
			sess->debug.dump_len = 0;
		}
	}

	return 0;
}

/* @brief control all sessions to set the dump length per second */
static int audio_out_debug_dump_length(uint8_t length)
{
	int i;
	aout_session_t *sess = aout_sessions;

	for (i = 0; i < AOUT_SESSION_MAX; i++, sess++) {
		if (AOUT_SESSION_CHECK_MAGIC(sess->magic)
			&& (sess->flags & AOUT_SESSION_OPEN)) {
			sess->debug.dump_len = length;
		}
	}

	return 0;
}
#endif

/* @brief Enable the DAC channel */
#ifdef CONFIG_AUDIO_OUT_DAC_SUPPORT
static int audio_out_enable_dac(struct device *dev, aout_session_t *session, aout_param_t *param)
{
	uint8_t channel_type = param->channel_type;
	struct aout_drv_data *data = dev->data;
	struct device *dac = data->dac0_dev;
	dac_setting_t *dac_setting = param->dac_setting;

	if (!dac) {
		LOG_ERR("no dac device");
		return -ENXIO;
	}

	if (!dac_setting) {
		LOG_ERR("DAC setting is NULL");
		return -EINVAL;
	}

	/* Check the DAC FIFO usage */
	if ((param->outfifo_type != AOUT_FIFO_DAC0)
		&& (param->outfifo_type != AOUT_FIFO_DAC1)) {
		LOG_ERR("Channel fifo type invalid %d", param->outfifo_type);
		return -EINVAL;
	}

	if ((param->channel_width != CHANNEL_WIDTH_16BITS)
		&& (param->channel_width != CHANNEL_WIDTH_24BITS)) {
		LOG_ERR("Invalid channel width %d", param->channel_width);
		return -EINVAL;
	}

	/* Linkage with I2STX */
	if (channel_type & AUDIO_CHANNEL_I2STX) {
		if (!data->i2stx0_dev) {
			LOG_ERR("no i2stx device");
			return -ENXIO;
		}
		if (phy_audio_enable(data->i2stx0_dev, (void *)param)) {
			LOG_ERR("Failed to enable I2STX");
			return -EFAULT;
		}
		else
			return 0;
	}

	/* Linkage with SPDIFTX */
	if (channel_type & AUDIO_CHANNEL_SPDIFTX) {
		if (!data->spdiftx0_dev) {
			LOG_ERR("no spdiftx device");
			return -ENXIO;
		}
		if (!phy_audio_enable(data->spdiftx0_dev, (void *)param))
			phy_audio_control(dac, PHY_CMD_CLAIM_WITH_128FS, NULL);
	}

	return phy_audio_enable(dac, (void *)param);
}
#endif

/* @brief Claim the use of the DAC/I2STX/PDMTX FIFO */
#if defined(CONFIG_AUDIO_OUT_I2STX_SUPPORT) \
	|| defined(CONFIG_AUDIO_OUT_SPDIFTX_SUPPORT)
static int audio_out_fifo_request(struct device *dev, aout_param_t *param)
{
	if (!dev)
		return -EINVAL;

	if (phy_audio_control(dev, PHY_CMD_FIFO_GET, param)) {
		LOG_ERR("Request FIFO failed");
		return -EBUSY;
	}

	return 0;
}

/* @brief Release the use of the DAC/I2STX FIFO */
static int audio_out_fifo_release(struct device *dev, uint8_t idx)
{
	if (!dev)
		return -EINVAL;

	if (phy_audio_control(dev, PHY_CMD_FIFO_PUT, &idx)) {
		LOG_ERR("Release DAC FIFO failed");
		return -EFAULT;
	}

	return 0;
}
#endif

/* @brief Enable the I2STX channel */
#ifdef CONFIG_AUDIO_OUT_I2STX_SUPPORT
static int audio_out_enable_i2stx(struct device *dev, aout_session_t *session, aout_param_t *param)
{
	uint8_t channel_type = param->channel_type;
	struct aout_drv_data *data = dev->data;
	struct device *i2stx = data->i2stx0_dev;
	i2stx_setting_t *i2stx_setting = param->i2stx_setting;
	int ret;

	if (!i2stx) {
		LOG_ERR("no i2s-tx device");
		return -ENXIO;
	}

	if (!i2stx_setting) {
		LOG_ERR("I2STX setting is NULL");
		return -EINVAL;
	}

	/* Check the DAC FIFO usage */
	if ((param->outfifo_type != AOUT_FIFO_DAC0)
		&& (param->outfifo_type != AOUT_FIFO_DAC1)
		&& (param->outfifo_type != AOUT_FIFO_I2STX0)) {
		LOG_ERR("Channel fifo type invalid %d", param->outfifo_type);
		return -EINVAL;
	}

	if ((param->channel_width != CHANNEL_WIDTH_16BITS)
		&& (param->channel_width != CHANNEL_WIDTH_24BITS)
		&& (param->channel_width != CHANNEL_WIDTH_20BITS)) {
		LOG_ERR("Invalid channel width %d", param->channel_width);
		return -EINVAL;
	}

	/* Linkage with SPDIFTX */
	if (channel_type & AUDIO_CHANNEL_SPDIFTX) {
		if (!data->spdiftx0_dev) {
			LOG_ERR("no spdiftx device");
			return -ENXIO;
		}
		if (!phy_audio_enable(data->spdiftx0_dev, param))
			phy_audio_control(data->dac0_dev, PHY_CMD_CLAIM_WITH_128FS, NULL);
	}

	if ((AOUT_FIFO_DAC0 == param->outfifo_type)
		|| (AOUT_FIFO_DAC1 == param->outfifo_type)) {
		if (audio_out_fifo_request(data->dac0_dev, param)) {
			LOG_ERR("Failed to take DAC FIFO");
			return -ENXIO;
		}
		++data->dac_fifo_ref_counter;
	}

	ret = phy_audio_enable(i2stx, (void *)param);
	if (ret) {
		if ((AOUT_FIFO_DAC0 == param->outfifo_type)
			|| (AOUT_FIFO_DAC1 == param->outfifo_type)) {
			audio_out_fifo_release(data->dac0_dev, param->outfifo_type);
			--data->dac_fifo_ref_counter;
		}
	}

	return ret;
}
#endif

/* @brief Enable the SPDIFTX channel */
#ifdef CONFIG_AUDIO_OUT_SPDIFTX_SUPPORT
static int audio_out_enable_spdiftx(struct device *dev, aout_session_t *session, aout_param_t *param)
{
	struct aout_drv_data *data = dev->data;
	struct device *spdiftx = data->spdiftx0_dev;
	spdiftx_setting_t *spdiftx_setting = param->spdiftx_setting;
	audio_dma_width_e width;
	int ret;

	if ((param->channel_width != CHANNEL_WIDTH_16BITS)
		&& (param->channel_width != CHANNEL_WIDTH_24BITS)) {
		LOG_ERR("Invalid channel width %d", param->channel_width);
		return -EINVAL;
	}

	if (param->channel_width == CHANNEL_WIDTH_16BITS)
		width = DMA_WIDTH_16BITS;
	else
		width = DMA_WIDTH_32BITS;

	if (!spdiftx) {
		LOG_ERR("Physical SPDIFTX device is not esixted");
		return -EFAULT;
	}

	if (!spdiftx_setting) {
		LOG_ERR("SPDIFTX setting is NULL");
		return -EINVAL;
	}

	/* Check the DAC FIFO usage */
	if ((param->outfifo_type != AOUT_FIFO_DAC0)
		&& (param->outfifo_type != AOUT_FIFO_DAC1)
		&& (param->outfifo_type != AOUT_FIFO_I2STX0)
		&& (param->outfifo_type != AOUT_FIFO_DAC1_ONLY_SPDIF)) {
		LOG_ERR("Channel fifo type invalid %d", param->outfifo_type);
		return -EINVAL;
	}

	if ((AOUT_FIFO_DAC0 == param->outfifo_type)
		|| (AOUT_FIFO_DAC1 == param->outfifo_type)
		|| (AOUT_FIFO_DAC1_ONLY_SPDIF == param->outfifo_type)) {
		if (!data->dac0_dev) {
			LOG_ERR("no dac device");
			return -ENXIO;
		}
		if (audio_out_fifo_request(data->dac0_dev, param)) {
			LOG_ERR("Failed to take DAC FIFO");
			return -ENXIO;
		}
		++data->dac_fifo_ref_counter;
	} else if (AOUT_FIFO_I2STX0 == param->outfifo_type) {
		if (!data->i2stx0_dev) {
			LOG_ERR("no i2stx device");
			return -ENXIO;
		}
		if (audio_out_fifo_request(data->i2stx0_dev, param)) {
			LOG_ERR("Failed to take I2STX FIFO");
			return -ENXIO;
		}
	}

	phy_audio_control(data->dac0_dev, PHY_CMD_CLAIM_WITH_128FS, NULL);

	ret = phy_audio_enable(spdiftx, (void *)param);
	if (ret) {
		if ((AOUT_FIFO_DAC0 == param->outfifo_type)
			|| (AOUT_FIFO_DAC1 == param->outfifo_type)
			|| (AOUT_FIFO_DAC1_ONLY_SPDIF == param->outfifo_type)) {
			audio_out_fifo_release(data->dac0_dev, param->outfifo_type);
			--data->dac_fifo_ref_counter;
		}
		else if (AOUT_FIFO_I2STX0 == param->outfifo_type)
			audio_out_fifo_release(data->i2stx0_dev, param->outfifo_type);
	}

	return ret;
}
#endif

/* @brief Enable the PDMTX channel */
#ifdef CONFIG_AUDIO_OUT_PDMTX_SUPPORT
static int audio_out_enable_pdmtx(struct device *dev, aout_session_t *session, aout_param_t *param)
{
	uint8_t channel_type = param->channel_type;
	struct aout_drv_data *data = dev->data;
	struct device *pdmtx = data->pdmtx0_dev;
	int ret;

	if (!pdmtx) {
		LOG_ERR("no pdm-tx device");
		return -ENXIO;
	}

	if (channel_type != AUDIO_CHANNEL_PDMTX) {
		LOG_ERR("invalid channel type:0x%x", channel_type);
		return -EINVAL;
	}

	/* Check the DAC FIFO usage */
	if (param->outfifo_type != AOUT_FIFO_I2STX0) {
		LOG_ERR("Channel fifo type invalid %d", param->outfifo_type);
		return -EINVAL;
	}

	if (param->channel_width != CHANNEL_WIDTH_16BITS) {
		LOG_ERR("Invalid channel width %d", param->channel_width);
		return -EINVAL;
	}

	ret = phy_audio_enable(pdmtx, (void *)param);
	if (ret)
		LOG_ERR("physical pdmtx enable error:%d", ret);

	return ret;
}
#endif

/* @brief DMA irq callback on direct method */
static void audio_out_dma_direct(const struct device *dev, void *user_data,
					   uint32_t channel, int status)
{
	aout_session_t *session = (aout_session_t *)user_data;
	uint32_t key;

	ARG_UNUSED(dev);
	ARG_UNUSED(channel);

	if (session && AOUT_SESSION_CHECK_MAGIC(session->magic)) {
		/* Only notify when DMA transfer completly */
		if ((status == DMA_IRQ_TC) && session->callback) {
			key = irq_lock();
			session->callback(session->cb_data, AOUT_DMA_IRQ_TC);
			irq_unlock(key);
		}
	}
}

/* @brief DMA irq callback on reload method */
static void audio_out_dma_reload(const struct device *dev, void *user_data,
					   uint32_t channel, int status)
{
	aout_session_t *session = (aout_session_t *)user_data;
	uint32_t ret_reason;
	int ret;

	ARG_UNUSED(dev);
	ARG_UNUSED(channel);

	if (session && AOUT_SESSION_CHECK_MAGIC(session->magic)) {
		if (session->callback) {
			if (status == DMA_IRQ_HF) {
				ret_reason = AOUT_DMA_IRQ_HF;
			} else if (status == DMA_IRQ_TC){
				ret_reason = AOUT_DMA_IRQ_TC;
			} else {
				LOG_ERR("Unknown DMA reson %d", status);
				return ;
			}
			ret = session->callback(session->cb_data, ret_reason);
			if (ret < 0)
				LOG_DBG("reload callback error:%d", ret);

#ifdef CONFIG_AUDIO_DYNAMIC_DEBUG
			audio_out_debug_perf(session, session->reload_addr,
				session->reload_len / 2);
#endif
		}
	}
}

/* @brief audio out dma enable interleaved mode */
static void audio_out_dma_separated_enable(struct device *dev, aout_session_t *session)
{
	ARG_UNUSED(dev);

	if (session) {
		LOG_INF("Enable AOUT DMA separated mode");
		session->dma_separated_en = 1;
	}
}

/* @brief prepare the DMA configuration for the audio out transfer */
static int audio_out_dma_prepare(struct device *dev, aout_session_t *session)
{
	struct aout_drv_data *data = dev->data;
	struct dma_config dma_cfg = {0};
	struct dma_block_config dma_block_cfg = {0};
	struct audio_out_dma_info info = {0};

	/* check already configured */
	if (session->flags & AOUT_SESSION_CONFIG)
		return 0;

	dma_cfg.channel_direction = MEMORY_TO_PERIPHERAL;
	dma_cfg.source_burst_length = 8;
	dma_cfg.dest_burst_length = 8;

	dma_block_cfg.source_reload_en = session->reload_en;

	dma_cfg.head_block = &dma_block_cfg;

	dma_cfg.source_data_size = session->dma_width;

	info.fifo_type = session->outfifo_type;

	if ((AOUT_FIFO_DAC0 == session->outfifo_type)
		|| (AOUT_FIFO_DAC1 == session->outfifo_type)
		|| (AOUT_FIFO_DAC1_ONLY_SPDIF == session->outfifo_type)) {
		if (phy_audio_control(data->dac0_dev, PHY_CMD_GET_AOUT_DMA_INFO, &info)) {
			LOG_ERR("Failed to get DAC DMA info");
			return -EFAULT;
		}
		uint8_t level, fifo_type = session->outfifo_type;
		if (fifo_type == AOUT_FIFO_DAC1_ONLY_SPDIF)
			fifo_type = AOUT_FIFO_DAC1;
		uint32_t fifo_cmd = PHY_FIFO_CMD(fifo_type, 0);
		if (!phy_audio_control(data->dac0_dev,
					PHY_CMD_FIFO_DRQ_LEVEL_GET, &fifo_cmd)) {
			level = PHY_GET_FIFO_CMD_VAL(fifo_cmd);
			LOG_DBG("DRQ level %d", level);
			/* When DRQ level < 8 levels shall use DMA single mode */
			if (level < 3)
				dma_cfg.dest_burst_length = 1;
		} else {
			LOG_ERR("Failed to get DRQ level");
			return -EFAULT;
		}
	} else if (AOUT_FIFO_I2STX0 == session->outfifo_type) {
		if (phy_audio_control(data->i2stx0_dev, PHY_CMD_GET_AOUT_DMA_INFO, &info)) {
			LOG_ERR("Failed to get I2STX DMA info");
			return -EFAULT;
		}
	}

	if (!data->dma_dev) {
		data->dma_dev = (struct device *)device_get_binding(info.dma_info.dma_dev_name);
		if (!data->dma_dev) {
			LOG_ERR("Bind DMA device %s error", info.dma_info.dma_dev_name);
			return -ENOENT;
		}
	}

	/* request dma channel handle */
	session->dma_chan = dma_request(data->dma_dev, 0xFF);
	if (session->dma_chan < 0) {
		LOG_ERR("Failed to request dma channel");
		return -ENXIO;
	}

	dma_cfg.dma_slot = info.dma_info.dma_id;

#ifdef CONFIG_AUDIO_OUT_DAC_PCMBUF_SUPPORT
	if ((AOUT_FIFO_DAC0 != session->outfifo_type)
		&& (AOUT_FIFO_DAC1 != session->outfifo_type))
#endif
	{
		if (session->callback) {
			if (session->reload_en)
				dma_cfg.dma_callback = audio_out_dma_reload;
			else
				dma_cfg.dma_callback = audio_out_dma_direct;
			dma_cfg.user_data = session;
			dma_cfg.complete_callback_en = 1;
		}
	}

	if (session->dma_separated_en) {
		dma_cfg.reserved = 1;
	}

	if (dma_config(data->dma_dev, session->dma_chan, &dma_cfg)) {
		LOG_ERR("DMA config error");
		return -EFAULT;
	}

	if (session->reload_en) {
		if (session->dma_separated_en) {
			dma_reload(data->dma_dev, session->dma_chan,
				(uint32_t)session->reload_addr,
				(uint32_t)session->reload_addr, session->reload_len * 2);
		} else {
			dma_reload(data->dma_dev, session->dma_chan,
				(uint32_t)session->reload_addr, 0, session->reload_len);
		}
	}

	LOG_DBG("request DMA channel:0x%x, DMA slot:%d", session->dma_chan, dma_cfg.dma_slot);

	session->flags |= AOUT_SESSION_CONFIG;

	return 0;
}

/* @brief audio out open and return the session handler */
static void *acts_audio_out_open(struct device *dev, aout_param_t *param)
{
	aout_session_t *session = NULL;
	struct aout_drv_data *data = dev->data;
	struct device *dac = data->dac0_dev;
	dac_fifosrc_setting_t def_dac_fifosrc;
	uint8_t channel_type;
	int ret;

	if (!param) {
		LOG_ERR("NULL parameter");
		return NULL;
	}
  #ifdef CONFIG_AUDIO_PA_AW87390_SUPPORT
    phy_audio_enable(data->pa_aw87390_dev,NULL);
  #endif
	/* There is a 120ms yield scheduer under extern pa operation and need to wait until the pa session close */
#ifdef CONFIG_AUDIO_OUT_DAC_SUPPORT
	if (acts_audio_wait_pa_session(dev, AOUT_SESSION_WAIT_PA_TIMEOUT))
		return NULL;
#endif

	audio_out_lock(dev);

	channel_type = param->channel_type;
	session = audio_out_session_get(channel_type);
	if (!session) {
		LOG_ERR("Failed to get audio session (type:%d)", channel_type);
		audio_out_unlock(dev);
		return NULL;
	}

	if (!param->callback) {
		LOG_ERR("Channel callback is NULL");
		goto err;
	}

	session->outfifo_type = param->outfifo_type;
	session->sample_rate = param->sample_rate;
	session->channel_type = channel_type;
	session->callback = param->callback;
	session->cb_data = param->cb_data;
	def_dac_fifosrc.fifo_idx = session->id;
	def_dac_fifosrc.fifo_from_dsp = session->dsp_fifo_src;
	def_dac_fifosrc.dsp_audio_set_param = NULL;
	session->dma_width = (CHANNEL_WIDTH_16BITS == param->channel_width) ? 2 : 4;

	if (param->reload_setting) {
		if ((!param->reload_setting->reload_addr)
			|| (!param->reload_setting->reload_len)) {
			LOG_ERR("Invalid reload parameter addr:0x%x, len:0x%x",
				(uint32_t)param->reload_setting->reload_addr,
				param->reload_setting->reload_len);
			goto err;
		}
		session->reload_en = 1;
		session->reload_addr = param->reload_setting->reload_addr;
		session->reload_len = param->reload_setting->reload_len;
		LOG_INF("In reload mode [0x%08x %d]",
			(uint32_t)session->reload_addr, session->reload_len);
	} else {
		session->reload_en = 0;
	}

	if (channel_type & AUDIO_CHANNEL_DAC) {
#ifdef CONFIG_AUDIO_OUT_DAC_SUPPORT
		ret = audio_out_enable_dac(dev, session, param);
		phy_audio_control(dac, AOUT_CMD_SET_FIFO_SRC, &def_dac_fifosrc);
#else
		ret = -ENXIO;
#endif
	} else if (channel_type & AUDIO_CHANNEL_I2STX) {
#ifdef CONFIG_AUDIO_OUT_I2STX_SUPPORT
		ret = audio_out_enable_i2stx(dev, session, param);
#else
		ret = -ENXIO;
#endif
	} else if (channel_type & AUDIO_CHANNEL_SPDIFTX) {
#ifdef CONFIG_AUDIO_OUT_SPDIFTX_SUPPORT
		ret = audio_out_enable_spdiftx(dev, session, param);
#else
		ret = -ENXIO;
#endif
	} else if (channel_type & AUDIO_CHANNEL_PDMTX) {
#ifdef CONFIG_AUDIO_OUT_PDMTX_SUPPORT
		ret = audio_out_enable_pdmtx(dev, session, param);
#else
		ret = -ENXIO;
#endif
	} else {
		LOG_ERR("Invalid channel type %d", channel_type);
		goto err;
	}

	if (ret) {
		LOG_ERR("Enable channel type %d error:%d", channel_type, ret);
		goto err;
	}

	LOG_INF("channel@%d sess:%p type:%d fifo:%d sr:%d opened",
				session->id, session, channel_type,
				param->outfifo_type, param->sample_rate);

	audio_out_unlock(dev);
	return (void *)session;
err:
	audio_out_session_put(session);
	audio_out_unlock(dev);
	return NULL;
}

/* @brief Disable the DAC channel */
#ifdef CONFIG_AUDIO_OUT_DAC_SUPPORT
static int audio_out_disable_dac(struct device *dev, aout_session_t *session)
{
	struct aout_drv_data *data = dev->data;
	struct device *dac = data->dac0_dev;
	uint8_t fifo_idx = session->outfifo_type;

	if (!dac) {
		LOG_ERR("no dac device");
		return -EFAULT;
	}

	if (session->channel_type & AUDIO_CHANNEL_SPDIFTX)
		phy_audio_control(dac, PHY_CMD_CLAIM_WITHOUT_128FS, NULL);

#if defined(CONFIG_BOARD_EXTERNAL_PA_ENABLE) && (CONFIG_AUDIO_DAC_POWER_PREFERRED == 1)
	acts_audio_close_external_pa(dev);
#endif

	return phy_audio_disable(dac, &fifo_idx);
}
#endif

/* @brief Disable the I2STX channel */
#ifdef CONFIG_AUDIO_OUT_I2STX_SUPPORT
static int audio_out_disable_i2stx(struct device *dev, aout_session_t *session)
{
	struct aout_drv_data *data = dev->data;
	struct device *i2stx = data->i2stx0_dev;
	uint8_t fifo_idx = session->outfifo_type;

	if (!i2stx) {
		LOG_ERR("Physical I2STX device is not esixted");
		return -EFAULT;
	}

	if ((AOUT_FIFO_DAC0 == session->outfifo_type)
		|| (AOUT_FIFO_DAC1 == session->outfifo_type)) {
		audio_out_fifo_release(data->dac0_dev, fifo_idx);
		--data->dac_fifo_ref_counter;
	}

	if (data->dac_fifo_ref_counter) {
		LOG_INF("Still remain %d DAC FIFO now is using", data->dac_fifo_ref_counter);
		return 0;
	}

	if (session->channel_type & AUDIO_CHANNEL_SPDIFTX)
		phy_audio_control(data->dac0_dev, PHY_CMD_CLAIM_WITHOUT_128FS, NULL);


	return phy_audio_disable(i2stx, NULL);
}
#endif

/* @brief Disable the SPDIFTX channel */
#ifdef CONFIG_AUDIO_OUT_SPDIFTX_SUPPORT
static int audio_out_disable_spdiftx(struct device *dev, aout_session_t *session)
{
	struct aout_drv_data *data = dev->data;
	struct device *spdiftx = data->spdiftx0_dev;
	uint8_t fifo_idx = session->outfifo_type;

	if (!spdiftx) {
		LOG_ERR("Physical SPDIFTX device is not esixted");
		return -EFAULT;
	}

	if ((AOUT_FIFO_DAC0 == session->outfifo_type)
		|| (AOUT_FIFO_DAC1 == session->outfifo_type)
		|| (AOUT_FIFO_DAC1_ONLY_SPDIF == session->outfifo_type)) {
		audio_out_fifo_release(data->dac0_dev, fifo_idx);
		--data->dac_fifo_ref_counter;
	} else if (AOUT_FIFO_I2STX0 == session->outfifo_type) {
		audio_out_fifo_release(data->i2stx0_dev, fifo_idx);
	}

	if (data->dac_fifo_ref_counter) {
		LOG_INF("Still remain %d DAC FIFO now is using", data->dac_fifo_ref_counter);
		return 0;
	}

	phy_audio_control(data->dac0_dev, PHY_CMD_CLAIM_WITHOUT_128FS, NULL);

	return phy_audio_disable(spdiftx, NULL);
}
#endif

#ifdef CONFIG_AUDIO_OUT_PDMTX_SUPPORT
static int audio_out_disable_pdmtx(struct device *dev, aout_session_t *session)
{
	struct aout_drv_data *data = dev->data;
	struct device *pdmtx = data->pdmtx0_dev;

	if (!pdmtx) {
		LOG_ERR("Physical PDMTX device is not esixted");
		return -EFAULT;
	}

	return phy_audio_disable(pdmtx, NULL);
}
#endif

/* @brief Close audio out channel by specified session handler */
static int acts_audio_out_close(struct device *dev, void *handle)
{
	struct aout_drv_data *data = dev->data;
	uint8_t channel_type;
	int ret;
	aout_session_t *session = (aout_session_t *)handle;

	if (!handle) {
		LOG_ERR("Invalid handle");
		return -EINVAL;
	}

	audio_out_lock(dev);

	if (!AOUT_SESSION_CHECK_MAGIC(session->magic)) {
		LOG_ERR("Session magic error:0x%x", session->magic);
		ret = -EFAULT;
		goto out;
	}

	channel_type = session->channel_type;
	#ifdef CONFIG_AUDIO_PA_AW87390_SUPPORT
    phy_audio_disable(data->pa_aw87390_dev,NULL);
  #endif
	if (channel_type & AUDIO_CHANNEL_DAC) {
#ifdef CONFIG_AUDIO_OUT_DAC_SUPPORT
		ret = audio_out_disable_dac(dev, session);
#else
		ret = -ENXIO;
#endif
	} else if (channel_type & AUDIO_CHANNEL_I2STX) {
#ifdef CONFIG_AUDIO_OUT_I2STX_SUPPORT
		ret = audio_out_disable_i2stx(dev, session);
#else
		ret = -ENXIO;
#endif
	} else if (channel_type & AUDIO_CHANNEL_SPDIFTX) {
#ifdef CONFIG_AUDIO_OUT_SPDIFTX_SUPPORT
		ret = audio_out_disable_spdiftx(dev, session);
#else
		ret = -ENXIO;
#endif
	} else if (channel_type & AUDIO_CHANNEL_PDMTX) {
#ifdef CONFIG_AUDIO_OUT_PDMTX_SUPPORT
		ret = audio_out_disable_pdmtx(dev, session);
#else
		ret = -ENXIO;
#endif
	} else {
		LOG_ERR("Invalid channel type %d", channel_type);
		ret = -EINVAL;
		goto out;
	}

	if (ret) {
		LOG_ERR("Disable channel type %d error:%d", channel_type, ret);
		goto out;
	}

	/* stop and free dma channel resource */
	if (data->dma_dev && (session->dma_chan >= 0)) {
 		dma_stop(data->dma_dev, session->dma_chan);
 		dma_free(data->dma_dev, session->dma_chan);
	}

	LOG_INF("session#%d@%p closed", session->id, session);

	audio_out_session_put(session);

out:
	audio_out_unlock(dev);
	return ret;
}

static int acts_audio_out_control(struct device *dev, void *handle, int cmd, void *param)
{
#if defined(CONFIG_AUDIO_OUT_DAC_SUPPORT) \
			|| defined(CONFIG_AUDIO_OUT_I2STX_SUPPORT) \
			|| defined(CONFIG_AUDIO_OUT_SPDIFTX_SUPPORT)
	struct aout_drv_data *data = dev->data;
#endif
	aout_session_t *session = (aout_session_t *)handle;
	uint8_t channel_type = session->channel_type;
	int ret;

	LOG_DBG("session#0x%x cmd 0x%x", (uint32_t)handle, cmd);

#ifdef CONFIG_AUDIO_OUT_DAC_SUPPORT
	if (AOUT_CMD_OPEN_PA == cmd)
		return acts_audio_open_pa(dev, true);

	if (AOUT_CMD_CLOSE_PA == cmd)
		return acts_audio_close_pa(dev);

	if (AOUT_CMD_PA_CLASS_SEL == cmd)
		return acts_audio_pa_class_select(dev, *(uint8_t *)param);
#endif

#ifdef CONFIG_AUDIO_POWERON_OPEN_I2STX
	if (AOUT_CMD_OPEN_I2STX_DEVICE == cmd)
		return acts_audio_open_i2stx_dev(dev);

	if (AOUT_CMD_CLOSE_I2STX_DEVICE == cmd)
		return acts_audio_close_i2stx_dev(dev);
#endif

#ifdef CONFIG_AUDIO_DYNAMIC_DEBUG
	if (AOUT_CMD_DEBUG_PERFORMANCE_CTL_ALL == cmd)
		return audio_out_debug_perf_all_sessions(*(uint8_t *)param);

	if (AOUT_CMD_DEBUG_DUMP_LENGTH_ALL == cmd)
		return audio_out_debug_dump_length(*(uint8_t *)param);
#endif

	if (cmd == AOUT_CMD_ANC_CONTROL) {
		audio_out_lock(dev);
		ret = phy_audio_control(data->dac0_dev, AOUT_CMD_ANC_CONTROL, param);
		audio_out_unlock(dev);
		return ret;
	}

	if (!AOUT_SESSION_CHECK_MAGIC(session->magic)) {
		LOG_ERR("Session magic error:0x%x", session->magic);
		return -EFAULT;
	}

#ifdef CONFIG_AUDIO_DYNAMIC_DEBUG
	if (AOUT_CMD_DEBUG_PERFORMANCE_CTL == cmd) {
		session->debug.performance = *(uint8_t *)param;
		session->debug.per_second_size = 0;
		session->debug.timestamp = 0;
		session->debug.dump_len = 0;
		return 0;
	}

	if (AOUT_CMD_DEBUG_DUMP_LENGTH == cmd) {
		session->debug.dump_len = *(uint8_t *)param;
		return 0;
	}
#endif

	if (AOUT_CMD_SET_SEPARATED_MODE == cmd) {
		audio_out_dma_separated_enable(dev, session);
		return 0;
	}
    if (AOUT_CMD_SET_FIFO_SRC == cmd) {
		dac_fifosrc_setting_t *dac_fifosrc = (dac_fifosrc_setting_t*)param;
        session->dsp_fifo_src = dac_fifosrc->fifo_from_dsp ? 1:0;
	}

	/* In the case of the commands according to the FIFO attribute */
	if (AOUT_IS_FIFO_CMD(cmd)) {
		if ((AOUT_FIFO_DAC0 == session->outfifo_type)
			|| (AOUT_FIFO_DAC1 == session->outfifo_type)
			|| (AOUT_FIFO_DAC1_ONLY_SPDIF == session->outfifo_type)) {
#ifdef CONFIG_AUDIO_OUT_DAC_SUPPORT
			uint32_t val = session->outfifo_type;
			if (val == AOUT_FIFO_DAC1_ONLY_SPDIF)
				val = AOUT_FIFO_DAC1;

			if (AOUT_CMD_GET_SAMPLE_CNT == cmd) {
				ret = phy_audio_control(data->dac0_dev,
							PHY_CMD_DAC_FIFO_GET_SAMPLE_CNT, &val);
				if (!ret)
					*(uint32_t *)param = val;
				else
					*(uint32_t *)param = 0;
			} else if (AOUT_CMD_RESET_SAMPLE_CNT == cmd) {
				ret = phy_audio_control(data->dac0_dev,
							PHY_CMD_DAC_FIFO_RESET_SAMPLE_CNT, &val);
			} else if (AOUT_CMD_ENABLE_SAMPLE_CNT == cmd) {
				ret = phy_audio_control(data->dac0_dev,
							PHY_CMD_DAC_FIFO_ENABLE_SAMPLE_CNT, &val);
			} else if (AOUT_CMD_DISABLE_SAMPLE_CNT == cmd) {
				ret = phy_audio_control(data->dac0_dev,
							PHY_CMD_DAC_FIFO_DISABLE_SAMPLE_CNT, &val);
			} else if (AOUT_CMD_GET_CHANNEL_STATUS == cmd) {
				ret = phy_audio_control(data->dac0_dev,
							AOUT_CMD_GET_CHANNEL_STATUS, &val);
				if (!ret)
					*(uint8_t *)param = val;
			} else if (PHY_CMD_FIFO_PUT == cmd) {
				ret = phy_audio_control(data->dac0_dev,
							PHY_CMD_FIFO_PUT, &val);
			} else if (AOUT_CMD_GET_DAC_FIFO_DRQ_LEVEL == cmd) {
				uint32_t fifo_cmd = PHY_FIFO_CMD(val, 0);
				ret = phy_audio_control(data->dac0_dev,
							PHY_CMD_FIFO_DRQ_LEVEL_GET, &fifo_cmd);
				if (!ret)
					*(uint8_t *)param = PHY_GET_FIFO_CMD_VAL(fifo_cmd);
			} else if (AOUT_CMD_SET_DAC_FIFO_DRQ_LEVEL == cmd) {
				uint8_t level = *(uint8_t *)param;
				uint32_t fifo_cmd = PHY_FIFO_CMD(val, level);
				ret = phy_audio_control(data->dac0_dev,
							PHY_CMD_FIFO_DRQ_LEVEL_SET, &fifo_cmd);
			} else if (AOUT_CMD_GET_DAC_FIFO_VOLUME == cmd) {
				uint32_t fifo_cmd = PHY_FIFO_CMD(val, 0);
				ret = phy_audio_control(data->dac0_dev,
							PHY_CMD_DAC_FIFO_VOLUME_GET, &fifo_cmd);
				if (!ret)
					*(uint8_t *)param = PHY_GET_FIFO_CMD_VAL(fifo_cmd);
			} else if (AOUT_CMD_SET_DAC_FIFO_VOLUME == cmd) {
				uint8_t volume = *(uint8_t *)param;
				uint32_t fifo_cmd = PHY_FIFO_CMD(val, volume);
				ret = phy_audio_control(data->dac0_dev,
							PHY_CMD_DAC_FIFO_VOLUME_SET, &fifo_cmd);
			} else {
				ret = phy_audio_control(data->dac0_dev, cmd, param);
			}
#else
			ret = -ENXIO;
#endif
		} else if (AOUT_FIFO_I2STX0 == session->outfifo_type) {
#ifdef CONFIG_AUDIO_OUT_I2STX_SUPPORT
			ret = phy_audio_control(data->i2stx0_dev, cmd, param);
			LOG_INF("i2stx0_dev+++cmd %d", cmd);
#else
			ret = -ENXIO;
#endif
		} else {
			ret = -ENXIO;
		}
		goto out;
	}

	if (channel_type & AUDIO_CHANNEL_DAC) {
#ifdef CONFIG_AUDIO_OUT_DAC_SUPPORT
		ret = phy_audio_control(data->dac0_dev, cmd, param);
#else
		ret = -ENXIO;
#endif
	} else if (channel_type & AUDIO_CHANNEL_I2STX) {
#ifdef CONFIG_AUDIO_OUT_I2STX_SUPPORT
		ret = phy_audio_control(data->i2stx0_dev, cmd, param);
#else
		ret = -ENXIO;
#endif
	} else if (channel_type & AUDIO_CHANNEL_SPDIFTX) {
#ifdef CONFIG_AUDIO_OUT_SPDIFTX_SUPPORT
		ret = phy_audio_control(data->spdiftx0_dev, cmd, param);
#else
		ret = -ENXIO;
#endif
	} else if (channel_type & AUDIO_CHANNEL_PDMTX) {
#ifdef CONFIG_AUDIO_OUT_PDMTX_SUPPORT
		ret = phy_audio_control(data->pdmtx0_dev, cmd, param);
#else
		ret = -ENXIO;
#endif
	} else {
		LOG_ERR("Invalid channel type %d", channel_type);
		ret = -EINVAL;
	}

out:
	return ret;
}

static int acts_audio_out_start(struct device *dev, void *handle)
{
	struct aout_drv_data *data = dev->data;
	aout_session_t *session = (aout_session_t *)handle;

	if (!handle) {
		LOG_ERR("Invalid handle");
		return -EINVAL;
	}

	if (!AOUT_SESSION_CHECK_MAGIC(session->magic)) {
		LOG_ERR("Session magic error:0x%x", session->magic);
		return -EFAULT;
	}

    if(!session->dsp_fifo_src) {
    	if (audio_out_dma_prepare(dev, session)) {
    		LOG_ERR("prepare session dma error");
    		return -ENXIO;
    	}
    }

#ifdef CONFIG_BOARD_EXTERNAL_PA_ENABLE
	acts_audio_open_external_pa(dev, false);
#endif

	/* In DMA reload mode only start one time is enough */
	if (session->reload_en
		&& (session->flags & AOUT_SESSION_START)) {
		return 0;
	}

	if (!(session->flags & AOUT_SESSION_START))
		session->flags |= AOUT_SESSION_START;

    if(session->dsp_fifo_src) {
        return 0;
    }

	return dma_start(data->dma_dev, session->dma_chan);
}

#if defined(CONFIG_AUDIO_OUT_DAC_PCMBUF_SUPPORT) && (CONFIG_AUDIO_DAC_WAIT_WRITE_PCMBUF_FINISH == 1)

static bool __acts_audio_check_dma_busy(struct device *dma_dev, uint32_t dma_chan)
{
	int ret;
	struct dma_status stat = {0};

	ret = dma_get_status(dma_dev, dma_chan, &stat);
	if (ret) {
		LOG_ERR("get dma(%d) status error %d\n", dma_chan, ret);
		return -EFAULT;
	}

	/* DMA transfer finish */
	if (!stat.pending_length)
		return false;

	return true;
}

static int acts_audio_dma_wait_timeout(struct device *dma_dev, uint32_t dma_chan, uint32_t timeout)
{
	uint32_t start_time, curr_time;

	LOG_DBG("audio wait write dma_chan:%d", dma_chan);

	start_time = k_cycle_get_32();

	while (1) {
		if (!__acts_audio_check_dma_busy(dma_dev, dma_chan))
			break;

		curr_time = k_cycle_get_32();
		if (k_cyc_to_us_floor32(curr_time - start_time) >= timeout) {
			LOG_ERR("wait dma(%d) finish timeout", dma_chan);
			return -ETIMEDOUT;
		}

		if ((!k_is_in_isr()) && (CONFIG_AUDIO_DAC_WAIT_WRITE_PCMBUF_SLEEP_MS)
			&& (!z_is_idle_thread_object(_current)))
			k_sleep(K_MSEC(CONFIG_AUDIO_DAC_WAIT_WRITE_PCMBUF_SLEEP_MS));
	}

	return 0;
}

/* @brief wait dac FIFO empty */
static int wait_dacfifo_empty(struct device *dev, aout_session_t *session)
{
#ifdef CONFIG_AUDIO_OUT_DAC_SUPPORT
	struct aout_drv_data *data = dev->data;
	int ret;
	uint8_t fifo_idx = session->outfifo_type;
	ret = phy_audio_control(data->dac0_dev, PHY_CMD_DAC_WAIT_EMPTY, &fifo_idx);
	if (ret) {
		LOG_ERR("wait empty error");
	}

	return ret;
#else
	return -ENXIO;
#endif
}

/* @brief Wait writing data to audio DAC channel finishly */
static int acts_audio_out_wait_write_finish(struct device *dev, aout_session_t *session)
{
	struct aout_drv_data *data = dev->data;
	int ret;

	if ((AOUT_FIFO_DAC0 == session->outfifo_type)
		|| (AOUT_FIFO_DAC1 == session->outfifo_type)) {
#if (CONFIG_AUDIO_DAC_WAIT_WRITE_PCMBUF_NEXT_TIME == 1)
		if (session->flags & AOUT_SESSION_START)
#endif
			{
			ret = acts_audio_dma_wait_timeout(data->dma_dev,
					session->dma_chan, CONFIG_AUDIO_DAC_WAIT_WRITE_PCMBUF_TIMEOUT_US);
			if (ret) {
				LOG_ERR("wait dma error %d", ret);
				return -EIO;
			}
			wait_dacfifo_empty(dev, session);
		}
	}

	return 0;
}

#endif

/* @brief Write the data to the audio output channel */
static int acts_audio_out_write(struct device *dev, void *handle, uint8_t *buffer, uint32_t length)
{
	struct aout_drv_data *data = dev->data;
	aout_session_t *session = (aout_session_t *)handle;
	int ret;

	if (!handle || !buffer || !length) {
		LOG_ERR("Invalid parameters (%p, %p, %d)", handle, buffer, length);
		return -EINVAL;
	}

	if (!AOUT_SESSION_CHECK_MAGIC(session->magic)) {
		LOG_ERR("Session magic error:0x%x", session->magic);
		return -EFAULT;
	}

	if (session->reload_en) {
		LOG_INF("Reload mode can start directly");
		return 0;
	}

	if (!(session->flags & AOUT_SESSION_OPEN)) {
		LOG_ERR("session not opened");
		return -EPERM;
	}

if(!session->dsp_fifo_src){
#if defined(CONFIG_AUDIO_OUT_DAC_PCMBUF_SUPPORT) \
		&& (CONFIG_AUDIO_DAC_WAIT_WRITE_PCMBUF_FINISH == 1) \
		&& (CONFIG_AUDIO_DAC_WAIT_WRITE_PCMBUF_NEXT_TIME == 1)
	ret = acts_audio_out_wait_write_finish(dev, session);
	if (ret)
		return ret;
#endif

	if (audio_out_dma_prepare(dev, session)) {
		LOG_ERR("prepare session dma error");
		return -ENXIO;
	}

	LOG_DBG("DMA channel:0x%x, buffer:%p len:%d", session->dma_chan, buffer, length);

	if (session->dma_separated_en) {
		ret = dma_reload(data->dma_dev, session->dma_chan, (uint32_t)buffer,
							(uint32_t)buffer, length * 2);
	} else {
		ret = dma_reload(data->dma_dev, session->dma_chan, (uint32_t)buffer, 0, length);
	}

	if (ret) {
		LOG_ERR("dma reload error %d", ret);
		return ret;
	}

}
	ret = audio_out_start(dev, handle);
	if (ret)
		LOG_ERR("dma start error %d", ret);

if(!session->dsp_fifo_src){
#if defined(CONFIG_AUDIO_OUT_DAC_PCMBUF_SUPPORT) \
			&& (CONFIG_AUDIO_DAC_WAIT_WRITE_PCMBUF_FINISH == 1) \
			&& (CONFIG_AUDIO_DAC_WAIT_WRITE_PCMBUF_NEXT_TIME == 0)
	ret = acts_audio_out_wait_write_finish(dev, session);
	if (ret)
		return ret;
#endif

}
#ifdef CONFIG_AUDIO_DYNAMIC_DEBUG
	if (!session->reload_en)
		audio_out_debug_perf(session, buffer, length);
#endif

	return ret;
}

static int acts_audio_out_stop(struct device *dev, void *handle)
{
	struct aout_drv_data *data = dev->data;
	aout_session_t *session = (aout_session_t *)handle;
	int ret = 0;

	if (session && AOUT_SESSION_CHECK_MAGIC(session->magic)) {
		LOG_INF("session#%p, audio out stop", session);
		if (data->dma_dev && session->dma_chan) {
			ret = dma_stop(data->dma_dev, session->dma_chan);
			if (!ret)
				session->flags &= ~AOUT_SESSION_START;
		}
	}

	return ret;
}

const struct aout_driver_api aout_drv_api = {
	.aout_open = acts_audio_out_open,
	.aout_close = acts_audio_out_close,
	.aout_control = acts_audio_out_control,
	.aout_start = acts_audio_out_start,
	.aout_write = acts_audio_out_write,
	.aout_stop = acts_audio_out_stop
};

/* @brief audio-out channles initialization */
static int audio_out_init(const struct device *dev)
{
	const struct aout_config_data *cfg = dev->config;
	struct aout_drv_data *data = dev->data;

	memset(data, 0, sizeof(struct aout_drv_data));

	k_sem_init(&data->lock, 1, 1);

#ifdef CONFIG_AUDIO_OUT_DAC_SUPPORT
	data->dac0_dev = (struct device *)device_get_binding(cfg->dac0_name);
	if (!data->dac0_dev) {
		LOG_ERR("no DAC device(%s)", cfg->dac0_name);
	}
#endif

#ifdef CONFIG_AUDIO_OUT_I2STX_SUPPORT
	data->i2stx0_dev = (struct device *)device_get_binding(cfg->i2stx0_name);
	if (!data->i2stx0_dev) {
		LOG_ERR("no I2STX device(%s)", cfg->i2stx0_name);
	}
#endif

#ifdef CONFIG_AUDIO_OUT_SPDIFTX_SUPPORT
	data->spdiftx0_dev = (struct device *)device_get_binding(cfg->spdiftx0_name);
	if (!data->spdiftx0_dev) {
		LOG_ERR("no SPDIFTX device(%s)", cfg->spdiftx0_name);
	}
#endif

#ifdef CONFIG_AUDIO_OUT_PDMTX_SUPPORT
	data->pdmtx0_dev = (struct device *)device_get_binding(cfg->pdmtx0_name);
	if (!data->pdmtx0_dev) {
		LOG_ERR("no PDMTX device(%s)", cfg->pdmtx0_name);
	}
#endif

#ifdef CONFIG_AUDIO_PA_AW87390_SUPPORT
	data->pa_aw87390_dev = (struct device *)device_get_binding(cfg->pa_aw87390_name);
	if (!data->pa_aw87390_dev) {
		LOG_ERR("no pa_aw87390_dev device(%s)", cfg->pa_aw87390_name);
	}
#endif
	/* clear pa session indicator */
	data->pa_session_indicator = 0;
	/* clear DAC fifo reference counter */
	data->dac_fifo_ref_counter = 0;

	/* disable AVDD pulldown */
	sys_write32(sys_read32(AUDIOLDO_CTL) & ~(1 << 4), AUDIOLDO_CTL);

	/* open internal and external pa */
#ifdef CONFIG_AUDIO_POWERON_OPEN_PA
  #ifdef CONFIG_AUDIO_PA_AW87390_SUPPORT
    phy_audio_disable(data->pa_aw87390_dev,NULL);
  #endif
	acts_audio_open_pa((struct device *)dev, CONFIG_POWERON_OPEN_EXTERNAL_PA);
  #ifdef CONFIG_AUDIO_PA_AW87390_SUPPORT
    phy_audio_enable(data->pa_aw87390_dev,NULL);
  #endif
#endif

#ifdef CONFIG_AUDIO_POWERON_OPEN_I2STX
	acts_audio_open_i2stx_dev(dev);
#endif

	printk("ACTS-AUDIO OUT initialized\n");

	return 0;
}

static struct aout_drv_data audio_out_drv_data;

static const struct aout_config_data audio_out_config_data = {
#ifdef CONFIG_AUDIO_OUT_DAC_SUPPORT
	.dac0_name = CONFIG_AUDIO_DAC_0_NAME,
#endif
#ifdef CONFIG_AUDIO_OUT_I2STX_SUPPORT
	.i2stx0_name = CONFIG_AUDIO_I2STX_0_NAME,
#endif
#ifdef CONFIG_AUDIO_OUT_SPDIFTX_SUPPORT
	.spdiftx0_name = CONFIG_AUDIO_SPDIFTX_0_NAME,
#endif
#ifdef CONFIG_AUDIO_OUT_PDMTX_SUPPORT
	.pdmtx0_name = CONFIG_AUDIO_PDMTX_0_NAME,
#endif
#ifdef CONFIG_AUDIO_PA_AW87390_SUPPORT
	.pa_aw87390_name = "aw87390_out",
#endif
};

DEVICE_DEFINE(audio_out, CONFIG_AUDIO_OUT_ACTS_DEV_NAME, audio_out_init,
		NULL,
	    &audio_out_drv_data, &audio_out_config_data,
	    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &aout_drv_api);
