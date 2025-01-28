/**
 * @file
 *
 * @brief Public devices config for drivers.
 */


#ifndef ZEPHYR_INCLUDE_DEV_CONFIG_H_
#define ZEPHYR_INCLUDE_DEV_CONFIG_H_

#include <board_cfg.h>
#if defined(CONFIG_SOC_SERIES_LARK)
#include <drivers/cfg_drv/pinctrl_lark.h>
#include <drivers/cfg_drv/dev_lark.h>
#elif defined(CONFIG_SOC_SERIES_LEOPARD)
#include <drivers/cfg_drv/pinctrl_leopard.h>
#include <drivers/cfg_drv/dev_leopard.h>
#else
#errro "not soc series"
#endif


struct dma_cfg {
	const char *dma_dev_name;
	uint8_t dma_id;
	uint8_t dma_chan; // 0xff vchan, 0-3 pchan
	uint8_t use_dma; // 0 not use
};

struct gpio_cfg {
	const char *gpio_dev_name;
	uint8_t		gpion; // 0-31
	uint8_t		flag; //GPIO_ACTIVE_HIGH or GPIO_ACTIVE_LOW
	uint8_t		use_gpio; // 0 not use
};

struct pwm_cfg {
	const char *dev_name;
	uint16_t	period; // period (max duty) in pwm clock cycles
	uint8_t		chan; // 0-31
	uint8_t		flag; // 1: active high; 0: active low
};

#define DMA_CFG_MAKE(dev_name, id, chan, use) {\
		.dma_dev_name =  dev_name, \
		.dma_id = id,\
		.dma_chan = chan,\
		.use_dma = use,	\
	}

#define DMA_CFG_SET(dev_name, id, chan, use) (\
		.dma_dev_name =  dev_name, \
		.dma_id = id,\
		.dma_chan = chan,\
		.use_dma = use,	\
		)

#define COND_DMA_CODE(_flag, id, chan) COND_CODE_1(_flag,DMA_CFG_SET(CONFIG_DMA_0_NAME, id, chan, 1), DMA_CFG_SET(NULL, 0, 0, 0))

#define GPIO_CFG_MAKE(dev_name, gpio, gflag, use) {\
		.gpio_dev_name =  dev_name, \
		.gpion = gpio,\
		.flag = gflag,\
		.use_gpio = use,\
	}

#define GPIO_CFG_SET(dev_name, gpio, gflag, use) (\
		.gpio_dev_name =  dev_name, \
		.gpion = gpio,\
		.flag = gflag,\
		.use_gpio = use,\
		)

#define COND_GPIO_CODE(_flag, gpio_dev, gpio, gflag) COND_CODE_1(_flag,GPIO_CFG_SET(gpio_dev, gpio, gflag, 1), GPIO_CFG_SET(NULL, 0, 0, 0))

#define PWM_CFG_MAKE(_dev_name, _chan, _period, _flag) {\
		.dev_name = _dev_name, \
		.chan = _chan,\
		.period = _period,\
		.flag = _flag,	\
	}

#endif //ZEPHYR_INCLUDE_DEV_CONFIG_H_
