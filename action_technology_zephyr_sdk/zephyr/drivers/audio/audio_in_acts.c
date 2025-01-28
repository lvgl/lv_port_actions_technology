/*
 * Copyright (c) 2020 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Audio IN(ADC/I2S-RX/SPDIF-RX) core management layer
 */

#include <kernel.h>
#include <device.h>
#include <string.h>
#include <errno.h>
#include <soc.h>
#include <board_cfg.h>
#include <drivers/dma.h>
#include <drivers/audio/audio_in.h>
#include "phy_audio_common.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(ain, CONFIG_LOG_DEFAULT_LEVEL);

#define AIN_SESSION_MAGIC                    (0x11223344)
#define AIN_SESSION_CHECK_MAGIC(x)           ((x) == AIN_SESSION_MAGIC)

#ifdef CONFIG_AUDIO_IN_ADC_SUPPORT
#define AIN_ADC_SESSION_MAX                  (1) /* 1 ADC channels */
#else
#define AIN_ADC_SESSION_MAX                  (1)
#endif

#ifdef CONFIG_AUDIO_IN_I2SRX_SUPPORT
#define AIN_I2SRX_SESSION_MAX                (1) /* 1 I2SRX channel */
#else
#define AIN_I2SRX_SESSION_MAX                (0)
#endif

#ifdef CONFIG_AUDIO_IN_SPDIFRX_SUPPORT
#define AIN_SPDIFRX_SESSION_MAX              (1) /* 1 SPDIFRX channel */
#else
#define AIN_SPDIFRX_SESSION_MAX              (0)
#endif

#define AIN_SESSION_MAX                      (AIN_ADC_SESSION_MAX + AIN_I2SRX_SESSION_MAX + AIN_SPDIFRX_SESSION_MAX) /* totally max sessions */

#define AIN_SESSION_OPEN                     (1 << 0) /* Session open flag */
#define AIN_SESSION_CONFIG                   (1 << 1) /* Session config flag */
#define AIN_SESSION_START                    (1 << 2) /* Session start flag */
#define AIN_SESSION_BIND                     (1 << 3) /* Session start flag */

#define DMA_IRQ_TC                           (0) /* DMA completion flag */
#define DMA_IRQ_HF                           (1) /* DMA half-full flag */

/**
 * struct ain_dynamic_debug_t
 * @brief audio in session dynamic debug structure
 */
typedef struct {
	uint32_t timestamp; /* record the per-second timestamp */
	uint32_t per_second_size; /* record the per-second play size */
	uint8_t dump_len; /* dump the length of buffer per-second */
	uint8_t performance: 1; /* enable flag of showing the play performance */
} ain_dynamic_debug_t;

/*
 * struct ain_session_t
 * @brief audio in session structure
 */
typedef struct {
	int magic; /* session magic value as AIN_SESSION_MAGIC */
	int dma_chan; /* DMA channel handle */
	uint16_t input_dev; /* input audio device */
	uint16_t channel_type; /* Indicates the channel type selection */
	uint8_t flags; /* session flags */
	uint8_t id; /* the session id */
	uint8_t dma_width; /* dma width */
	int (*callback)(void *cb_data, uint32_t reason); /* The callback function which called when #AIN_DMA_IRQ_HF or #AIN_DMA_IRQ_TC events happened */
	void *cb_data; /* callback user data */
	uint8_t *reload_addr; /* Reload buffer address to transfer */
	uint32_t reload_len;  /* The length of the reload buffer */
#ifdef CONFIG_AUDIO_DYNAMIC_DEBUG
	ain_dynamic_debug_t debug; /* dynamic debug */
#endif
	uint8_t dma_separated_en : 1; /* the flag of DMA interleaved mode enable or not */
	uint8_t dma_separated_mode : 2; /* the DMA interleaved mode which specifies the data format stored in memory */
} ain_session_t;

/*
 * struct aout_drv_data
 * @brief audio in driver data which are software related
 */
struct ain_drv_data {
	struct k_sem lock;
	struct device *dma_dev;
	struct device *adc0_dev; /* ADC0 device handler */
	struct device *i2srx0_dev; /* I2SRX0 device handler */
	struct device *spdifrx0_dev; /* SPFIRX0 device handler */
	struct device *anc0_dev; /* ANC0 device handler */
	uint32_t adc_sess_stack[AIN_ADC_SESSION_MAX + 1]; /* stack to store sync wait sessions */
	uint8_t adc_sess_stack_cur; /* current curtor of adc session stack */
	uint8_t adc_ref_counter; /* ADC channels start reference counter */
	uint8_t anc_en_flag : 1; /* the flag of ANC enable */
};

/*
 * struct aout_config_data
 * @brief audio in config data which are the hardward related
 */
struct ain_config_data {
	const char *adc0_name; /* physical ADC0 device name */
	const char *i2srx0_name; /* physical I2SRX0 device name */
	const char *spdifrx0_name; /* physical SPDIFRX0 device name */
};

static ain_session_t ain_sessions[AIN_SESSION_MAX];

/*
 * @brief checkout if there is the same channel within audio in sessions
 */
static bool audio_in_session_check(uint16_t type)
{
	int i;
	ain_session_t *sess = ain_sessions;
	uint8_t max_sess, sess_opened = 0;

	if (AUDIO_CHANNEL_ADC & type) {
		max_sess = AIN_ADC_SESSION_MAX;
	} else if (AUDIO_CHANNEL_I2SRX & type) {
		max_sess = AIN_I2SRX_SESSION_MAX;
	} else if (AUDIO_CHANNEL_SPDIFRX & type) {
		max_sess = AIN_SPDIFRX_SESSION_MAX;
	} else {
		LOG_ERR("Invalid session type %d", type);
		return true;
	}

	for (i = 0; i < AIN_SESSION_MAX; i++, sess++) {
		if (AIN_SESSION_CHECK_MAGIC(sess->magic)
			&& (sess->flags & AIN_SESSION_OPEN)
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
 * @brief Get audio in session by specified channel
 */
static ain_session_t *audio_in_session_get(uint16_t type)
{
	ain_session_t *sess = ain_sessions;
	int i;

	if (audio_in_session_check(type))
		return NULL;

	for (i = 0; i < AIN_SESSION_MAX; i++, sess++) {
		if (!(sess->flags & AIN_SESSION_OPEN)) {
			memset(sess, 0, sizeof(ain_session_t));
			sess->magic = AIN_SESSION_MAGIC;
			sess->flags = AIN_SESSION_OPEN;
			sess->channel_type = type;
			sess->id = i;
			return sess;
		}
	}

	return NULL;
}

/*
 * @brief Put audio in session by given session address
 */
static void audio_in_session_put(ain_session_t *s)
{
	ain_session_t *sess = ain_sessions;
	int i;
	for (i = 0; i < AIN_SESSION_MAX; i++, sess++) {
		if ((uint32_t)s == (uint32_t)sess) {
			memset(s, 0, sizeof(ain_session_t));
			break;
		}
	}
}

/* @brief audio in session lock */
static inline void audio_in_lock(struct device *dev)
{
	struct ain_drv_data *data = dev->data;
	k_sem_take(&data->lock, K_FOREVER);
}

/* @brief audio in session unlock */
static inline void audio_in_unlock(struct device *dev)
{
	struct ain_drv_data *data = dev->data;
	k_sem_give(&data->lock);
}

/* @brief bind the audio in session to dma start at the same time */
static int audio_in_session_bind(struct device *dev, ain_session_t *this, ain_session_t *bind)
{
	struct ain_drv_data *data = dev->data;

	if ((!bind) || (!AIN_SESSION_CHECK_MAGIC(bind->magic))) {
		LOG_ERR("Invalid session:%p", bind);
		return -EINVAL;
	}

	/* check if the session does not config or already start */
	if (!(bind->flags & AIN_SESSION_CONFIG) || (bind->flags & AIN_SESSION_START)) {
		LOG_ERR("Invalid session status:0x%x", bind->flags);
		return -EPERM;
	}

	if (this == bind) {
		LOG_ERR("Bind the same session@%p", this);
		return -EPERM;
	}

	if (bind->flags & AIN_SESSION_BIND) {
		LOG_INF("Already bind session@%p", bind);
		return 0;
	}

	if (bind->channel_type != AUDIO_CHANNEL_ADC) {
		LOG_ERR("For now only support ADC to bind channels");
		return -EPERM;
	}

	bind->flags |= AIN_SESSION_BIND;
	++data->adc_ref_counter;

	LOG_INF("session@%p binded", bind);

	return 0;
}

#ifdef CONFIG_AUDIO_DYNAMIC_DEBUG
/* @brief show the audio in performance */
static void audio_in_debug_perf(ain_session_t *session, uint8_t *buffer, uint32_t len)
{
	uint32_t delta;

	if (session->debug.performance) {
		delta = (uint32_t)k_cyc_to_ns_floor64(k_cycle_get_32() - session->debug.timestamp);
		session->debug.per_second_size += len;
		if (delta > 1000000000UL) {
			session->debug.timestamp = k_cycle_get_32();
			printk("** in session@%p - %dB/s **\n",
					session, session->debug.per_second_size);
			session->debug.per_second_size = 0;
			if (session->debug.dump_len) {
				AUDIO_DUMP_MEM(buffer, session->debug.dump_len);
			}
		}
	}
}

/* @brief control all sessions to enable or disable performance statistics */
static int audio_in_debug_perf_all_sessions(uint8_t is_en)
{
	int i;
	ain_session_t *sess = ain_sessions;

	for (i = 0; i < AIN_SESSION_MAX; i++, sess++) {
		if (AIN_SESSION_CHECK_MAGIC(sess->magic)
			&& (sess->flags & AIN_SESSION_OPEN)) {
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
static int audio_in_debug_dump_length(uint8_t length)
{
	int i;
	ain_session_t *sess = ain_sessions;

	for (i = 0; i < AIN_SESSION_MAX; i++, sess++) {
		if (AIN_SESSION_CHECK_MAGIC(sess->magic)
			&& (sess->flags & AIN_SESSION_OPEN)) {
			sess->debug.dump_len = length;
		}
	}

	return 0;
}
#endif

/* @brief Enable the ADC channel */
#ifdef CONFIG_AUDIO_IN_ADC_SUPPORT
static int audio_in_enable_adc(struct device *dev, ain_param_t *param)
{
	struct ain_drv_data *data = dev->data;
	struct device *adc = data->adc0_dev;

	if (!adc) {
		LOG_ERR("Physical ADC device is not esixted");
		return -EFAULT;
	}

	if ((param->channel_width != CHANNEL_WIDTH_16BITS)
		&& (param->channel_width != CHANNEL_WIDTH_24BITS)) {
		LOG_ERR("Invalid channel width %d", param->channel_width);
		return -EINVAL;
	}

	return phy_audio_enable(adc, (void *)param);
}
#endif

/* @brief Enable the I2SRX channel */
#ifdef CONFIG_AUDIO_IN_I2SRX_SUPPORT
static int audio_in_enable_i2srx(struct device *dev, ain_param_t *param)
{
	struct ain_drv_data *data = dev->data;
	struct device *i2srx = data->i2srx0_dev;

	if (!i2srx) {
		LOG_ERR("Physical I2SRX device is not esixted");
		return -EFAULT;
	}

	if ((param->channel_width != CHANNEL_WIDTH_16BITS)
		&& (param->channel_width != CHANNEL_WIDTH_20BITS)
		&& (param->channel_width != CHANNEL_WIDTH_24BITS)) {
		LOG_ERR("Invalid channel width %d", param->channel_width);
		return -EINVAL;
	}

	return phy_audio_enable(i2srx, (void *)param);
}
#endif

/* @brief Enable the SPDIFRX channel */
#ifdef CONFIG_AUDIO_IN_SPDIFRX_SUPPORT
static int audio_in_enable_spdifrx(struct device *dev, ain_param_t *param)
{
	struct ain_drv_data *data = dev->data;
	struct device *spdifrx = data->spdifrx0_dev;

	if (!spdifrx) {
		LOG_ERR("Physical SPDIFRX device is not esixted");
		return -EFAULT;
	}

	if ((param->channel_width != CHANNEL_WIDTH_16BITS)
		&& (param->channel_width != CHANNEL_WIDTH_24BITS)) {
		LOG_ERR("Invalid channel width %d", param->channel_width);
		return -EINVAL;
	}

	return phy_audio_enable(spdifrx, (void *)param);
}
#endif

/* @brief DMA irq callback on reload method */
static void audio_in_dma_reload(const struct device *dev, void *user_data,
					   uint32_t channel, int status)
{

	ain_session_t *session = (ain_session_t *)user_data;
	uint32_t ret_reson;
	uint32_t key, delta, len;
	uint16_t time = CONFIG_ADC_STARTUP_DISCARD_TIME;
	static uint32_t timestamp = 0;
	static bool already_discard = false;

	ARG_UNUSED(dev);
	ARG_UNUSED(channel);
  if(session ==NULL)
  {
    LOG_WRN("session is NULL");
    return;
  }
  len = session->reload_len / 2;
	if (!timestamp) {
		timestamp = k_cycle_get_32();
		LOG_DBG("discard data start time:%u ns",k_cyc_to_ns_floor32(timestamp));
	}

	if ((!already_discard) && time) {
		delta = (uint32_t)k_cyc_to_ns_floor64(k_cycle_get_32() - timestamp);
		if (delta > (time * 1000000UL)) {
			already_discard = true;
		}

		LOG_DBG("discard data end time:%u ns", k_cyc_to_ns_floor32(k_cycle_get_32()));
	}

	if (session && AIN_SESSION_CHECK_MAGIC(session->magic)) {
		if (session->callback) {
			key = irq_lock();
			if (status == DMA_IRQ_HF) {
				ret_reson = AIN_DMA_IRQ_HF;

				if (!already_discard && time)
					memset(session->reload_addr, 0, len);

			} else {
				ret_reson = AIN_DMA_IRQ_TC;

				if (!already_discard && time)
					memset(session->reload_addr + len, 0, len);

			}
			session->callback(session->cb_data, ret_reson);
			irq_unlock(key);

#ifdef CONFIG_AUDIO_DYNAMIC_DEBUG
			audio_in_debug_perf(session, session->reload_addr,
				session->reload_len / 2);
#endif
		}
	}
}

/* @brief audio out dma enable separated mode */
static int audio_in_dma_separated_enable(struct device *dev,
				ain_session_t *session, audio_interleave_mode_e *p_mode)
{
	ARG_UNUSED(dev);

	if (!session || !p_mode) {
		LOG_ERR("Invalid parameter");
		return -EINVAL;
	}

	session->dma_separated_en = 1;
	session->dma_separated_mode = *p_mode;

	LOG_INF("Enable AIN DMA separated mode:%d", session->dma_separated_mode);

	return 0;
}

/* @brief prepare the DMA configuration for the audio in transfer */
static int audio_in_dma_prepare(struct device *dev, ain_session_t *session)
{
	struct ain_drv_data *data = dev->data;
	struct dma_config dma_cfg = {0};
	struct dma_block_config dma_block_cfg = {0};
	struct audio_in_dma_info info = {0};

	dma_cfg.channel_direction = PERIPHERAL_TO_MEMORY;

	/* Only support the DMA reload mode */
	dma_block_cfg.dest_reload_en = 1;

	dma_cfg.head_block = &dma_block_cfg;

	dma_cfg.source_data_size = session->dma_width;

	if (AUDIO_CHANNEL_ADC == session->channel_type) {
		info.input_dev = session->input_dev;
		if (phy_audio_control(data->adc0_dev, PHY_CMD_GET_AIN_DMA_INFO, &info)) {
			LOG_ERR("Failed to get ADC DMA info");
			return -EFAULT;
		}
		uint32_t level;
		uint32_t fifo_cmd = PHY_FIFO_CMD(session->input_dev, 0);

		if (!phy_audio_control(data->adc0_dev,
					PHY_CMD_FIFO_DRQ_LEVEL_GET, &fifo_cmd)) {
			level = PHY_GET_FIFO_CMD_VAL(fifo_cmd);
			LOG_DBG("DRQ level %d", level);
			/* When DRQ level < 8 levels shall use DMA single mode */
			if (level < 8)
				dma_cfg.source_burst_length = 1;
		} else {
			LOG_ERR("Failed to get DRQ level");
			return -EFAULT;
		}
	} else if (AUDIO_CHANNEL_I2SRX == session->channel_type) {
		if (phy_audio_control(data->i2srx0_dev, PHY_CMD_GET_AIN_DMA_INFO, &info)) {
			LOG_ERR("Failed to get I2SRX DMA info");
			return -EFAULT;
		}
	} else if (AUDIO_CHANNEL_SPDIFRX == session->channel_type) {
		if (phy_audio_control(data->spdifrx0_dev, PHY_CMD_GET_AIN_DMA_INFO, &info)) {
			LOG_ERR("Failed to get SPDIFRX DMA info");
			return -EFAULT;
		}
	} else {
		LOG_ERR("invalid channel type 0x%x\n", session->channel_type);
		return -EINVAL;
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

	/* Use the DMA irq to notify the status of transfer */
	if (session->callback) {
		dma_cfg.dma_callback = audio_in_dma_reload;
		dma_cfg.user_data = (void *)session;
		dma_cfg.complete_callback_en = 1;
	}

	if (session->dma_separated_en) {
		dma_cfg.reserved = 1;
	}

	if (dma_config(data->dma_dev, session->dma_chan, &dma_cfg)) {
		LOG_ERR("DMA config error");
		return -EFAULT;
	}

	if (session->reload_addr) {
		LOG_INF("reload addr:0x%x, reload len:%d\n", (uint32_t)session->reload_addr, session->reload_len);

		if (session->dma_separated_en) {
			uint32_t dma_length = (session->reload_len << 1);
		    if (session->dma_separated_mode == LEFT_MONO_RIGHT_MUTE_MODE) {
		        dma_reload(data->dma_dev, session->dma_chan, AUDIO_IN_DMA_RESERVED_ADDRESS,
							(uint32_t)session->reload_addr, dma_length);
		    } else if (session->dma_separated_mode == LEFT_RIGHT_SEPERATE){
		        dma_reload(data->dma_dev, session->dma_chan,
					(uint32_t)(session->reload_addr + session->reload_len),
					(uint32_t)session->reload_addr, dma_length);
		    } else if (session->dma_separated_mode == LEFT_MUTE_RIGHT_MONO_MODE){
		        dma_reload(data->dma_dev, session->dma_chan, (uint32_t)session->reload_addr,
					AUDIO_IN_DMA_RESERVED_ADDRESS, dma_length);
		    }
		} else {
			dma_reload(data->dma_dev, session->dma_chan,
				0, (uint32_t)session->reload_addr, session->reload_len);
		}
	}

	return 0;
}

/* @brief audio out enable and return the session handler */
static void *acts_audio_in_open(struct device *dev, ain_param_t *param)
{
	ain_session_t * session = NULL;
	uint8_t channel_type;
	int ret;
	void *rtn_sess = NULL;

	if (!param) {
		LOG_ERR("NULL parameter");
		return NULL;
	}

	audio_in_lock(dev);

	channel_type = param->channel_type;
	session = audio_in_session_get(channel_type);
	if (!session) {
		LOG_ERR("Failed to get audio session (type:%d)", channel_type);
		goto out;
	}

	if (param->reload_setting.reload_addr && !param->reload_setting.reload_len) {
		LOG_ERR("Reload parameter error addr:0x%x, len:0x%x",
			(uint32_t)param->reload_setting.reload_addr, param->reload_setting.reload_len);
		audio_in_session_put(session);
		goto out;
	}

	session->callback = param->callback;
	session->cb_data = param->cb_data;
	session->reload_addr = param->reload_setting.reload_addr;
	session->reload_len = param->reload_setting.reload_len;
	session->dma_width = (CHANNEL_WIDTH_16BITS == param->channel_width) ? 2 : 4;

	if (channel_type == AUDIO_CHANNEL_ADC) {
#ifdef CONFIG_AUDIO_IN_ADC_SUPPORT
		ret = audio_in_enable_adc(dev, param);
		if (!ret)
			session->input_dev = param->adc_setting->device;
#else
		ret = -ENXIO;
#endif
	} else if (channel_type == AUDIO_CHANNEL_I2SRX) {
#ifdef CONFIG_AUDIO_IN_I2SRX_SUPPORT
		ret = audio_in_enable_i2srx(dev, param);
#else
		ret = -ENXIO;
#endif
	} else if (channel_type == AUDIO_CHANNEL_SPDIFRX) {
#ifdef CONFIG_AUDIO_IN_SPDIFRX_SUPPORT
		ret = audio_in_enable_spdifrx(dev, param);
#else
		ret = -ENXIO;
#endif
	} else {
		LOG_ERR("Invalid channel type %d", channel_type);
		audio_in_session_put(session);
		goto out;
	}

	if (ret) {
		LOG_ERR("Enable channel type %d error:%d", channel_type, ret);
		audio_in_session_put(session);
		goto out;
	}

	session->flags |= AIN_SESSION_CONFIG;

	/* The session address is the audio in handler */
	rtn_sess = (void *)session;

	LOG_INF("channel@%d sess:%p type:%d input_dev:0x%x opened",
			session->id, session, channel_type, session->input_dev);

out:
	audio_in_unlock(dev);
	return rtn_sess;
}

/* @brief Disable the ADC channel */
#ifdef CONFIG_AUDIO_IN_ADC_SUPPORT
static int audio_in_disable_adc(struct device *dev, ain_session_t *session)
{
	struct ain_drv_data *data = dev->data;
	struct device *adc = data->adc0_dev;
	uint16_t input_dev = session->input_dev;

	if (!adc) {
		LOG_ERR("Physical ADC device is not esixted");
		return -EFAULT;
	}

	if ((session->flags & AIN_SESSION_BIND)
		&& !(session->flags & AIN_SESSION_START)) {
		/* the session already bind but not start yet */
		if (data->adc_ref_counter)
			--data->adc_ref_counter;
	}

	if (!data->adc_ref_counter)
		data->adc_sess_stack_cur = 0;

	return phy_audio_disable(adc, &input_dev);
}
#endif

/* @brief Disable the I2SRX channel */
#ifdef CONFIG_AUDIO_IN_I2SRX_SUPPORT
static int audio_in_disable_i2srx(struct device *dev, ain_session_t *session)
{
	struct ain_drv_data *data = dev->data;
	struct device *i2srx = data->i2srx0_dev;

	if (!i2srx) {
		LOG_ERR("Physical I2SRX device is not esixted");
		return -EFAULT;
	}

	return phy_audio_disable(i2srx, NULL);
}
#endif

/* @brief Disable the SPDIFRX channel */
#ifdef CONFIG_AUDIO_IN_SPDIFRX_SUPPORT
static int audio_in_disable_spdifrx(struct device *dev, ain_session_t *session)
{
	struct ain_drv_data *data = dev->data;
	struct device *spdifrx = data->spdifrx0_dev;

	if (!spdifrx) {
		LOG_ERR("Physical SPDIFRX device is not esixted");
		return -EFAULT;
	}

	return phy_audio_disable(spdifrx, NULL);
}
#endif

/* @brief Disable audio in channel by specified session handler */
static int acts_audio_in_close(struct device *dev, void *handle)
{
	struct ain_drv_data *data = dev->data;
	uint8_t channel_type;
	int ret;
	ain_session_t *session = (ain_session_t *)handle;

	if (!handle) {
		LOG_ERR("Invalid handle");
		return -EINVAL;
	}

	audio_in_lock(dev);

	if (!AIN_SESSION_CHECK_MAGIC(session->magic)) {
		LOG_ERR("Session magic error:0x%x", session->magic);
		ret = -EFAULT;
		goto out;
	}

	channel_type = session->channel_type;
	if (channel_type == AUDIO_CHANNEL_ADC) {
#ifdef CONFIG_AUDIO_IN_ADC_SUPPORT
		ret = audio_in_disable_adc(dev, session);
#else
		ret = -ENXIO;
#endif
	} else if (channel_type == AUDIO_CHANNEL_I2SRX) {
#ifdef CONFIG_AUDIO_IN_I2SRX_SUPPORT
		ret = audio_in_disable_i2srx(dev, session);
#else
		ret = -ENXIO;
#endif
	} else if (channel_type == AUDIO_CHANNEL_SPDIFRX) {
#ifdef CONFIG_AUDIO_IN_SPDIFRX_SUPPORT
		ret = audio_in_disable_spdifrx(dev, session);
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
	if (data->dma_dev && session->dma_chan) {
 		dma_stop(data->dma_dev, session->dma_chan);
 		dma_free(data->dma_dev, session->dma_chan);
	}

 	LOG_INF("session#%d@%p closed", session->id, session);

	audio_in_session_put(session);

out:
	audio_in_unlock(dev);
	return ret;
}

static int acts_audio_in_control(struct device *dev, void *handle, int cmd, void *param)
{
	struct ain_drv_data *data = dev->data;
	ain_session_t *session = (ain_session_t *)handle;
	uint8_t channel_type = session->channel_type;
	int ret;

	LOG_DBG("session#0x%x cmd 0x%x", (uint32_t)handle, cmd);

	if (cmd == AIN_CMD_ANC_CONTROL) {
		audio_in_lock(dev);
		ret = phy_audio_control(data->adc0_dev, AIN_CMD_ANC_CONTROL, param);
		audio_in_unlock(dev);
		return ret;
	}

#ifdef CONFIG_AUDIO_DYNAMIC_DEBUG
	if (AIN_CMD_DEBUG_PERFORMANCE_CTL_ALL == cmd)
		return audio_in_debug_perf_all_sessions(*(uint8_t *)param);

	if (AIN_CMD_DEBUG_DUMP_LENGTH_ALL == cmd)
		return audio_in_debug_dump_length(*(uint8_t *)param);
#endif

	if (!AIN_SESSION_CHECK_MAGIC(session->magic)) {
		LOG_ERR("Session magic error:0x%x", session->magic);
		return -EFAULT;
	}

#ifdef CONFIG_AUDIO_DYNAMIC_DEBUG
	if (AIN_CMD_DEBUG_PERFORMANCE_CTL == cmd) {
		session->debug.performance = *(uint8_t *)param;
		session->debug.per_second_size = 0;
		session->debug.timestamp = 0;
		session->debug.dump_len = 0;
		return 0;
	}

	if (AIN_CMD_DEBUG_DUMP_LENGTH == cmd) {
		session->debug.dump_len = *(uint8_t *)param;
		return 0;
	}
#endif

	if (cmd == AIN_CMD_BIND_CHANNEL)
		return audio_in_session_bind(dev, session, (ain_session_t *)param);

	if (cmd == AIN_CMD_SET_SEPARATED_MODE) {
		return audio_in_dma_separated_enable(dev, session,
					(audio_interleave_mode_e *)param);
	}

	if (channel_type == AUDIO_CHANNEL_ADC) {
#ifdef CONFIG_AUDIO_IN_ADC_SUPPORT
		if (AIN_CMD_SET_ADC_GAIN == cmd) {
			adc_setting_t setting = {0};
			adc_gain *gain = (adc_gain *)param;
			setting.device = session->input_dev;
			memcpy(&setting.gain, gain, sizeof(adc_gain));
			ret = phy_audio_control(data->adc0_dev, PHY_CMD_ADC_GAIN_CONFIG, &setting);
		} else if ((AIN_CMD_GET_ADC_LEFT_GAIN_RANGE == cmd)
			|| (AIN_CMD_GET_ADC_RIGHT_GAIN_RANGE == cmd)) {
			adc_gain_range *range = (adc_gain_range *)param;
			/* use the 'min' to send the input device info to phy-adc device */
			range->min = (int16_t)session->input_dev;
			ret = phy_audio_control(data->adc0_dev, cmd, param);
		} else if (AIN_CMD_GET_ADC_FIFO_DRQ_LEVEL == cmd) {
			uint32_t fifo_cmd = PHY_FIFO_CMD(session->input_dev, 0);
			ret = phy_audio_control(data->adc0_dev,
						PHY_CMD_FIFO_DRQ_LEVEL_GET, &fifo_cmd);
			if (!ret)
				*(uint8_t *)param = PHY_GET_FIFO_CMD_VAL(fifo_cmd);
		} else if (AIN_CMD_SET_ADC_FIFO_DRQ_LEVEL == cmd) {
			uint8_t level = *(uint8_t *)param;
			uint32_t fifo_cmd = PHY_FIFO_CMD(session->input_dev, level);
			ret = phy_audio_control(data->adc0_dev,
						PHY_CMD_FIFO_DRQ_LEVEL_SET, &fifo_cmd);
		} else {
			ret = phy_audio_control(data->adc0_dev, cmd, param);
		}
#else
		ret = -ENXIO;
#endif
	} else if (channel_type == AUDIO_CHANNEL_I2SRX) {
#ifdef CONFIG_AUDIO_IN_I2SRX_SUPPORT
		ret = phy_audio_control(data->i2srx0_dev, cmd, param);
#else
		ret = -ENXIO;
#endif
	} else if (channel_type == AUDIO_CHANNEL_SPDIFRX) {
#ifdef CONFIG_AUDIO_IN_SPDIFRX_SUPPORT
		ret = phy_audio_control(data->spdifrx0_dev, cmd, param);
#else
		ret = -ENXIO;
#endif
	} else {
		LOG_ERR("Invalid channel type %d", channel_type);
		ret = -EINVAL;
	}

	return ret;
}

/* @brief Start the audio input transfering */
static int acts_audio_in_start(struct device *dev, void *handle)
{
	struct ain_drv_data *data = dev->data;
	ain_session_t *session = (ain_session_t *)handle;
	int ret;

	if (!handle) {
		LOG_ERR("Invalid handle");
		return -EINVAL;
	}

	if (!AIN_SESSION_CHECK_MAGIC(session->magic)) {
		LOG_ERR("Session magic error:0x%x", session->magic);
		return -EFAULT;
	}

	/* In DMA reload mode only start one time is enough */
	if (session->flags & AIN_SESSION_START) {
		LOG_DBG("session%p already started", session);
		return 0;
	}

	if (AUDIO_CHANNEL_ADC == session->channel_type) {
		if (data->adc_ref_counter) {
			if (data->adc_sess_stack_cur >= AIN_ADC_SESSION_MAX) {
				/* unlikely branch */
				LOG_ERR("FIXME: invlaid stack cur%d", data->adc_sess_stack_cur);
				return -EFAULT;
			}
			data->adc_sess_stack[data->adc_sess_stack_cur++] = (uint32_t)session;
			--data->adc_ref_counter;
			LOG_INF("session@%p save to wait stack[%d]", session, data->adc_ref_counter);
			return 0;
		}

		struct aduio_in_adc_en adc_en = {0};
		uint16_t input_dev[AIN_ADC_SESSION_MAX] = {0};
		adc_en.input_dev_array = input_dev;

		if (data->adc_sess_stack_cur > 0) {
			uint8_t i;
			ain_session_t *_sess = NULL;

			data->adc_sess_stack[data->adc_sess_stack_cur++] = (uint32_t)session;

			for (i = 0; i < data->adc_sess_stack_cur; i++) {
				_sess = (ain_session_t *)data->adc_sess_stack[i];
				if (!(_sess->flags & AIN_SESSION_CONFIG)
					|| (_sess->flags & AIN_SESSION_START)
					|| (_sess->channel_type != AUDIO_CHANNEL_ADC)) {
					/* unlikely branch */
					LOG_ERR("FIXME: invalid session@%p status:0x%x", _sess, _sess->flags);
					data->adc_sess_stack_cur = 0;
					return -ESRCH;
				}
				input_dev[i] = _sess->input_dev;
			}
			adc_en.input_dev_num = data->adc_sess_stack_cur;

			/* always set start flag regardless of the result of start */
			session->flags |= AIN_SESSION_START;

			ret = phy_audio_control(data->adc0_dev, PHY_CMD_ADC_DIGITAL_ENABLE, &adc_en);
			if (ret) {
				LOG_ERR("Failed to enable ADC err=%d", ret);
				data->adc_sess_stack_cur = 0;
				return ret;
			}

			for (i = 0; i < data->adc_sess_stack_cur; i++) {
				_sess = (ain_session_t *)data->adc_sess_stack[i];
				ret = audio_in_dma_prepare(dev, _sess);
				if (ret) {
					LOG_ERR("session@%p DMA prepare error=%d", session, ret);
					continue;
				}
				ret = dma_start(data->dma_dev, _sess->dma_chan);
				if (ret) {
					LOG_ERR("session@%p DMA start error:%d", _sess, ret);
				} else {
					LOG_INF("session@%p started", _sess);
				}
			}

			data->adc_sess_stack_cur = 0;
		} else {
			input_dev[0] = session->input_dev;
			adc_en.input_dev_num = 1;
			ret = phy_audio_control(data->adc0_dev, PHY_CMD_ADC_DIGITAL_ENABLE, &adc_en);
			if (ret) {
				LOG_ERR("Failed to enable ADC err=%d", ret);
				return ret;
			}
			ret = audio_in_dma_prepare(dev, session);
			if (ret) {
				LOG_ERR("session@%p DMA prepare error=%d", session, ret);
				return ret;
			}
			ret = dma_start(data->dma_dev, session->dma_chan);
			if (ret) {
				LOG_ERR("session@%p DMA start error:%d", session, ret);
			} else {
				LOG_INF("session@%p started", session);
			}
		}
	} else {
		session->flags |= AIN_SESSION_START;

		ret = audio_in_dma_prepare(dev, session);
		if (ret) {
			LOG_ERR("session@%p DMA prepare error=%d", session, ret);
			return ret;
		}

		ret = dma_start(data->dma_dev, session->dma_chan);
		if (ret) {
			LOG_ERR("session@%p DMA start error:%d", session, ret);
		} else {
			LOG_INF("session@%p started", session);
		}
	}

	return ret;
}

static int acts_audio_in_stop(struct device *dev, void *handle)
{
	struct ain_drv_data *data = dev->data;
	ain_session_t *session = (ain_session_t *)handle;
	int ret = 0;

	if (session && AIN_SESSION_CHECK_MAGIC(session->magic)) {
		LOG_INF("session#%p, audio in stop", session);
		if (data->dma_dev && session->dma_chan)
			ret = dma_stop(data->dma_dev, session->dma_chan);
	}

	return ret;
}

const struct ain_driver_api ain_drv_api = {
	.ain_open = acts_audio_in_open,
	.ain_close = acts_audio_in_close,
	.ain_control = acts_audio_in_control,
	.ain_start = acts_audio_in_start,
	.ain_stop = acts_audio_in_stop
};

/* @brief audio-in channles initialization */
static int audio_in_init(const struct device *dev)
{
	const struct ain_config_data *cfg = dev->config;
	struct ain_drv_data *data = dev->data;

	memset(data, 0, sizeof(struct ain_drv_data));

	k_sem_init(&data->lock, 1, 1);

#ifdef CONFIG_AUDIO_IN_ADC_SUPPORT
	data->adc0_dev = (struct device *)device_get_binding(cfg->adc0_name);
	if (!data->adc0_dev) {
		LOG_ERR("no ADC device(%s)", cfg->adc0_name);
	}
#endif

#ifdef CONFIG_AUDIO_IN_I2SRX_SUPPORT
	data->i2srx0_dev = (struct device *)device_get_binding(cfg->i2srx0_name);
	if (!data->i2srx0_dev) {
		LOG_ERR("no I2SRX device(%s)", cfg->i2srx0_name);
	}
#endif

#ifdef CONFIG_AUDIO_IN_SPDIFRX_SUPPORT
	data->spdifrx0_dev = (struct device *)device_get_binding(cfg->spdifrx0_name);
	if (!data->spdifrx0_dev) {
		LOG_ERR("no SPDIFRX device(%s)", cfg->spdifrx0_name);
	}
#endif

	/* disable AVDD pulldown */
	sys_write32(sys_read32(AUDIOLDO_CTL) & ~(1 << 4), AUDIOLDO_CTL);

	printk("ACTS-AUDIO IN initialized\n");

	return 0;
}

static struct ain_drv_data audio_in_drv_data;

static struct ain_config_data audio_in_config_data = {
#ifdef CONFIG_AUDIO_IN_ADC_SUPPORT
	.adc0_name = CONFIG_AUDIO_ADC_0_NAME,
#endif
#ifdef CONFIG_AUDIO_IN_I2SRX_SUPPORT
	.i2srx0_name = CONFIG_AUDIO_I2SRX_0_NAME,
#endif
#ifdef CONFIG_AUDIO_IN_SPDIFRX_SUPPORT
	.spdifrx0_name = CONFIG_AUDIO_SPDIFRX_0_NAME,
#endif
};

DEVICE_DEFINE(audio_in, CONFIG_AUDIO_IN_ACTS_DEV_NAME, audio_in_init,
		NULL,
	    &audio_in_drv_data, &audio_in_config_data,
	    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &ain_drv_api);
