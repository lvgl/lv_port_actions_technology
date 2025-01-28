/*
 * Copyright (c) 2017 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ADC Keyboard driver for Actions SoC
 */

#include <errno.h>
#include <kernel.h>
#include <string.h>
#include <init.h>
#include <irq.h>
#include <drivers/adc.h>
#include <drivers/input/input_dev.h>
#include <sys/util.h>
#include <sys/byteorder.h>
#include <board.h>
#include <board_cfg.h>
#include <soc_pmu.h>
#include <logging/log.h>

#ifdef CONFIG_CFG_DRV
#include <config.h>
#include <drivers/cfg_drv/driver_config.h>
#endif

LOG_MODULE_REGISTER(keyadc, CONFIG_SYS_LOG_INPUT_DEV_LEVEL);

#ifdef CONFIG_CFG_DRV
struct adckey_map {
	uint16_t key_code;
	uint16_t adc_min;
	uint16_t adc_max;
};
#else
struct adckey_map {
	uint16_t key_code;
	uint16_t adc_val;
};
#endif

struct acts_adckey_data {
	const struct device *adc_dev;
	const struct device *this_dev;
#ifdef CONFIG_ADCKEY_POLL_TIMER
	struct k_timer timer;
#else
	struct k_delayed_work timer;
#endif
	struct adc_sequence sequence;

	uint8_t adc_buf[4];

	int scan_count;
	uint16_t prev_keycode;
	uint16_t prev_stable_keycode;

	input_notify_t notify;
	uint32_t timestamp;
	uint8_t state; /* if 1 indicates the polling delay worker is runing */

#ifdef CONFIG_CFG_DRV
	struct adckey_map adckey_keymaps[CFG_MAX_LRADC_KEYS];
#endif
};

struct acts_adckey_config {
	char *adc_name;
	uint16_t adc_chan;

	uint16_t poll_interval_ms;
	uint16_t sample_filter_dep;

	uint32_t poll_total_ms;

#ifndef CONFIG_CFG_DRV
	uint16_t key_cnt;
	const struct adckey_map *key_maps;
#endif
};

static void adckey_pmu_notify(void *cb_data, int state);
static void adckey_acts_disable(const struct device *dev);

/* adc_val -> key_code */
static uint16_t adckey_acts_get_keycode(const struct acts_adckey_config *cfg,
				     struct acts_adckey_data *adckey,
				     int adc_val)
{
#ifdef CONFIG_CFG_DRV
	struct adckey_map *map = adckey->adckey_keymaps;
	int i;

	if (!map)
		goto out;

	for (i = 0; i < CFG_MAX_LRADC_KEYS; i++) {
		if ((adc_val < map->adc_max) && (adc_val > map->adc_min))
			return map->key_code;
		map++;
	}

	out:
		return KEY_RESERVED;
#else
	const struct adckey_map *map = cfg->key_maps;
	int i;

	if (!map)
		goto out;

	for (i = 0; i < cfg->key_cnt; i++) {
		if (adc_val < map->adc_val)
			return map->key_code;

		map++;
	}

out:
	return KEY_RESERVED;
#endif
}

static void adckey_acts_report_key(struct acts_adckey_data *adckey,
				   int key_code, int value)
{
	struct input_value val;

	if (adckey->notify) {
		val.keypad.type = EV_KEY;
		val.keypad.code = key_code;
		val.keypad.value = value;

		LOG_DBG("report key_code %d value %d",
			key_code, value);

		adckey->notify(NULL, &val);
	}
}


void adckey_acts_inquiry(const struct device *dev, struct input_value *val)
{
   struct acts_adckey_data *adckey = dev->data;
   const struct device *adckeydev = adckey->this_dev;
   const struct acts_adckey_config *cfg = adckeydev->config;
   uint16_t keycode, adcval;
   int ret;

   /* get adc value */
   ret = adc_read(adckey->adc_dev, &adckey->sequence);
   if (ret) {
	   LOG_ERR("ADC read error %d", ret);
	   return ;
   }

   adcval = sys_get_le16(adckey->sequence.buffer);

   LOG_DBG("adc value: %d\n", adcval);
   /* get keycode */
   keycode = adckey_acts_get_keycode(cfg, adckey, adcval);

   LOG_DBG("adcval %d, keycode %d\n", adcval, keycode);

   if (keycode != KEY_RESERVED && val->keypad.code == keycode)
   {
	   val->keypad.type = EV_KEY;
	   val->keypad.value = 1;
   }

}

#ifdef CONFIG_ADCKEY_POLL_TIMER
static void adckey_acts_poll(struct k_timer *timer)
#else
static void adckey_acts_poll(struct k_work *work)
#endif
{
#ifdef CONFIG_ADCKEY_POLL_TIMER
	struct device *dev = k_timer_user_data_get(timer);
	struct acts_adckey_data *adckey = dev->data;
#else
	struct acts_adckey_data *adckey =
			CONTAINER_OF(work, struct acts_adckey_data, timer);
	const struct device *dev = adckey->this_dev;
#endif
	const struct acts_adckey_config *cfg = dev->config;
	uint16_t keycode, adcval;
	int ret;
	bool key_release = false;

	struct detect_param_t detect_param =
			{PMU_DETECT_DEV_REMOTE, adckey_pmu_notify, (void *)dev};

	/* get adc value */
	ret = adc_read(adckey->adc_dev, &adckey->sequence);
	if (ret) {
		LOG_ERR("ADC read error %d", ret);
		return ;
	}

	adcval = sys_get_le16(adckey->sequence.buffer);

	/* get keycode */
	keycode = adckey_acts_get_keycode(cfg, adckey, adcval);

	LOG_DBG("adcval 0x%x, keycode %d", adcval, keycode);

	/* no key is pressed or releasing */
	if (keycode == KEY_RESERVED &&
	    adckey->prev_stable_keycode == KEY_RESERVED){
	    key_release = true;
		goto out;
    }

    key_release = false;
	if (keycode == adckey->prev_keycode) {
		adckey->scan_count++;
		if (adckey->scan_count == cfg->sample_filter_dep) {
			/* previous key is released? */
			if (adckey->prev_stable_keycode != KEY_RESERVED
				&& keycode != adckey->prev_stable_keycode)
				adckey_acts_report_key(adckey,
					adckey->prev_stable_keycode, 0);

			/* a new key press? */
			if (keycode != KEY_RESERVED)
				adckey_acts_report_key(adckey, keycode, 1);

			/* clear counter for new checking */
			adckey->prev_stable_keycode = keycode;
			adckey->scan_count = 0;
		}
	} else {
		/* new key pressed? */
		adckey->prev_keycode = keycode;
		adckey->scan_count = 0;
	}

out:
	/* Only LRADC1 can trigger PMU interrupt */
	if (cfg->adc_chan != PMUADC_ID_LRADC1) {
		k_delayed_work_submit(&adckey->timer, K_MSEC(cfg->poll_interval_ms));
	} else {
		if (((k_uptime_get_32() - adckey->timestamp) > cfg->poll_total_ms) && key_release) {
			adckey_acts_disable(dev);
			if (soc_pmu_register_notify(&detect_param))
				LOG_ERR("failed to register pmu notify");
		} else {
#ifdef CONFIG_ADCKEY_POLL_TIMER
			k_timer_start(&adckey->timer, K_MSEC(cfg->poll_interval_ms), K_NO_WAIT);
#else
			k_delayed_work_submit(&adckey->timer, K_MSEC(cfg->poll_interval_ms));
#endif
		}
	}
}

static void adckey_acts_enable(const struct device *dev)
{
	struct acts_adckey_data *adckey = dev->data;
	uint32_t key;

	LOG_DBG("enable adckey");

	key = irq_lock();
	if (adckey->state) {
		irq_unlock(key);
		return ;
	}
	adckey->state = 1;
	irq_unlock(key);

	adckey->timestamp = k_uptime_get_32();

#ifdef CONFIG_ADCKEY_POLL_TIMER
	const struct acts_adckey_config *cfg = dev->config;
	k_timer_start(&adckey->timer, K_MSEC(cfg->poll_interval_ms), K_NO_WAIT);
#else
	k_delayed_work_submit(&adckey->timer, K_NO_WAIT);
#endif
}

static void adckey_acts_disable(const struct device *dev)
{
	struct acts_adckey_data *adckey = dev->data;
	uint32_t key;

	LOG_DBG("disable adckey");

	key = irq_lock();
	if (!adckey->state) {
		irq_unlock(key);
		return ;
	}
	adckey->state = 0;
	irq_unlock(key);

	adckey->timestamp = 0;

#ifdef CONFIG_ADCKEY_POLL_TIMER
	k_timer_stop(&adckey->timer);
#else
	k_delayed_work_cancel(&adckey->timer);
#endif
}

static void adckey_acts_register_notify(const struct device *dev, input_notify_t notify)
{
	struct acts_adckey_data *adckey = dev->data;

	LOG_DBG("register notify 0x%x", (uint32_t)notify);

	adckey->notify = notify;
}

static void adckey_acts_unregister_notify(const struct device *dev, input_notify_t notify)
{
	struct acts_adckey_data *adckey = dev->data;

	LOG_DBG("unregister notify 0x%x", (uint32_t)notify);

	adckey->notify = NULL;
}

const struct input_dev_driver_api adckey_acts_driver_api = {
	.enable = adckey_acts_enable,
	.disable = adckey_acts_disable,
	.inquiry = adckey_acts_inquiry,
	.register_notify = adckey_acts_register_notify,
	.unregister_notify = adckey_acts_unregister_notify,
};

static void adckey_pmu_notify(void *cb_data, int state)
{
	struct device *dev = (struct device *)cb_data;
	LOG_INF("PMU notify state:%d", state);

	adckey_acts_enable(dev);
}

#ifdef CONFIG_CFG_DRV
static int adckey_acts_config_init(const struct device *dev, uint8_t *channel_id)
{
	struct acts_adckey_data *adckey = dev->data;
	int ret;
	CFG_Type_LRADC_Key Key[CFG_MAX_LRADC_KEYS];
	uint32_t LRADC_Ctrl;
	uint8_t i;

	ret = cfg_get_by_key(ITEM_LRADC_KEY, Key, sizeof(Key));
	if (!ret) {
		LOG_ERR("failed to get ITEM_LRADC_KEY");
		return -ESRCH;
	}

	for (i = 0; i < CFG_MAX_LRADC_KEYS; i++) {
		adckey->adckey_keymaps[i].key_code = Key[i].Key_Value;
		adckey->adckey_keymaps[i].adc_max = Key[i].ADC_Max;
		adckey->adckey_keymaps[i].adc_min = Key[i].ADC_Min;
	}

	ret = cfg_get_by_key(ITEM_LRADC_CTRL, &LRADC_Ctrl, sizeof(LRADC_Ctrl));
	if (!ret) {
		LOG_ERR("failed to get ITEM_LRADC_CTRL");
		return -ESRCH;
	}

	if (ret > 0) {
		uint8_t channel_no = LRADC_Ctrl & 0xff;
		uint8_t gpio_no = (LRADC_Ctrl & 0xff00) >> 8;
		uint32_t mfp_sel = (LRADC_Ctrl & 0xff0000) >> 16;

		/* remap channel number to LRADC index */
		channel_no += 5;

		if (channel_no < GPIO_MAX_PIN_NUM)
			sys_write32(mfp_sel, GPIO_REG_BASE + (1 + gpio_no)*4);
		else if (gpio_no < (GPIO_MAX_PIN_NUM + 4))
			sys_write32(mfp_sel, WIO0_CTL + (gpio_no - GPIO_MAX_PIN_NUM) * 4);

		*channel_id = channel_no;
	}

	return 0;
}
#endif

int adckey_acts_init(const struct device *dev)
{
	const struct acts_adckey_config *cfg = dev->config;
	struct acts_adckey_data *adckey = dev->data;
	struct adc_channel_cfg channel_cfg = {0};
	struct detect_param_t detect_param = {PMU_DETECT_DEV_REMOTE, adckey_pmu_notify, (void *)dev};

	adckey->adc_dev = device_get_binding(cfg->adc_name);
	if (!adckey->adc_dev) {
		LOG_ERR("cannot found adc dev %s\n", cfg->adc_name);
		return -ENODEV;
	}

	channel_cfg.channel_id = cfg->adc_chan;

#ifdef CONFIG_CFG_DRV
	uint8_t channel_cfg_id;
	if (adckey_acts_config_init(dev, &channel_cfg_id))
		return -ENXIO;
	channel_cfg.channel_id = channel_cfg_id;
#endif

	if (adc_channel_setup(adckey->adc_dev, &channel_cfg)) {
		LOG_ERR("setup channel_id %d error", channel_cfg.channel_id);
		return -EFAULT;
	}

	adckey->sequence.channels = BIT(cfg->adc_chan);
	adckey->sequence.buffer = &adckey->adc_buf[0];
	adckey->sequence.buffer_size = sizeof(adckey->adc_buf);
	adckey->this_dev = dev;

	if (soc_pmu_register_notify(&detect_param)) {
		LOG_ERR("failed to register pmu notify");
		return -ENXIO;
	}

#ifdef CONFIG_ADCKEY_POLL_TIMER
	k_timer_init(&adckey->timer, adckey_acts_poll, NULL);
	k_timer_user_data_set(&adckey->timer, (void *)dev);
#else
	k_delayed_work_init(&adckey->timer, adckey_acts_poll);
#endif

	return 0;
}

static struct acts_adckey_data adckey_acts_ddata;

#ifndef CONFIG_CFG_DRV
static const struct adckey_map adckey_acts_keymaps[] = {
#ifdef BOARD_ADCKEY_KEY_MAPPINGS
	BOARD_ADCKEY_KEY_MAPPINGS
#endif
};
#endif

static const struct acts_adckey_config adckey_acts_cdata = {
	.adc_name = CONFIG_PMUADC_NAME,
	.adc_chan = CONFIG_ADCKEY_LRADC_CHAN,

	.poll_interval_ms = CONFIG_ADCKEY_POLL_INTERVAL_MS,
	.poll_total_ms = CONFIG_ADCKEY_POLL_TOTAL_MS,
	.sample_filter_dep = CONFIG_ADCKEY_SAMPLE_FILTER_CNT,

#ifndef CONFIG_CFG_DRV
	.key_cnt = ARRAY_SIZE(adckey_acts_keymaps),
	.key_maps = &adckey_acts_keymaps[0],
#endif
};

#if IS_ENABLED(CONFIG_ADCKEY)
DEVICE_DEFINE(adckey_acts, CONFIG_INPUT_DEV_ACTS_ADCKEY_NAME,
			adckey_acts_init, NULL,
		    &adckey_acts_ddata, &adckey_acts_cdata,
		    PRE_KERNEL_2, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &adckey_acts_driver_api);
#endif
