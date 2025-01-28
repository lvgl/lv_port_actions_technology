/*
 * Copyright (c) 2018 Justin Watson
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <errno.h>
#include <kernel.h>
#include <device.h>
#include <init.h>
#include <drivers/gpio.h>
#include <drivers/i2c.h>
#include <board_cfg.h>
#include "gpio_utils.h"


#include <logging/log.h>
//LOG_MODULE_REGISTER(et6416, CONFIG_GPIO_LOG_LEVEL);
LOG_MODULE_REGISTER(et6416, LOG_LEVEL_DBG);


#include "gpio_utils.h"

/* Number of pins supported by the device */
#define NUM_PINS 16

/* Max to select all pins supported on the device. */
#define ALL_PINS ((u16_t)BIT_MASK(NUM_PINS))

/* Reset delay is 2.5 ms, round up for Zephyr resolution */
#define RESET_DELAY_MS 3

#define ET6416_EDGE_FALLING   1
#define ET6416_EDGE_RISING  2
#define ET6416_EDGE_BOTH  3

/** Cache of the output configuration and data of the pins. */
struct et6416_pin_state {
	u16_t input_dat;    /* 0x00 */
	u16_t output_data;        /* 0x02 */
	u16_t polarity;        /* 0x04 */
	u16_t dir;          /* 0x06, 1=input 0= output */
} ;

struct et6416_irq_state {
	u16_t interrupt_mask;   /*  */
	u32_t interrupt_sense;  /* */
};



/** Runtime driver data */
struct et6416_drv_data {
	/* gpio_driver_data needs to be first */
	struct gpio_driver_data common;
	const struct device *i2c_master;
	struct et6416_pin_state pin_state;
	struct k_sem lock;

#ifdef CONFIG_GPIO_ET6416_INTERRUPT
	struct device *gpio_int;
	struct gpio_callback gpio_cb;
	struct k_work work;
	struct et6416_irq_state irq_state;
	struct device *dev;
	/* user ISR cb */
	sys_slist_t cb;
	/* Enabled INT pins generating a cb */
	u16_t cb_pins;
#endif /* CONFIG_GPIO_ET6416_INTERRUPT */

};

/** Configuration data */
struct et6416_config {
	/* gpio_driver_config needs to be first */
	struct gpio_driver_config common;
	const char *i2c_master_dev_name;
#ifdef CONFIG_GPIO_ET6416_INTERRUPT
	struct gpio_cfg et6416_irq;
#endif /* CONFIG_GPIO_ET6416_INTERRUPT */
	u16_t i2c_slave_addr;
};


/* Pin configuration register addresses */
enum {
	ET6416_REG_INPUT               = 0x00,
	ET6416_REG_OUTPUT              = 0x02,
	ET6416_REG_POLARITY_INVERSION  = 0x04,
	ET6416_REG_CFG                 = 0x06,

};



/**
 * @brief Write a  word to an internal address of an I2C slave.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param dev_addr Address of the I2C device for writing.
 * @param reg_addr Address of the internal register being written.
 * @param value Value to be written to internal register.
 *
 * @retval 0 If successful.
 * @retval -EIO General input / output error.
 */
static inline int i2c_reg_write_word(const struct device *dev, u16_t dev_addr,
					u8_t reg_addr, u16_t value)
{
	u8_t tx_buf[3] = { reg_addr, value & 0xff , value >> 8};
	return i2c_write(dev, tx_buf, 3, dev_addr);
}

static inline int i2c_reg_read_word(const struct device *dev, u16_t dev_addr,
					u8_t reg_addr, u16_t *value)
{
	return i2c_burst_read(dev, dev_addr,reg_addr,  (u8_t *)value, 2);
}


#ifdef CONFIG_GPIO_ET6416_INTERRUPT
static int et6416_handle_interrupt(void *arg)
{
	struct device *dev = (struct device *) arg;
	//const struct et6416_config *cfg = dev->config_info;
	struct et6416_drv_data *drv_data = dev->data;
	u16_t pin_data;
	int ret = 0;
	u16_t int_source;

	k_sem_take(&drv_data->lock, K_FOREVER);
	ret = i2c_reg_read_word(drv_data->i2c_master, cfg->i2c_slave_addr,
			    ET6416_REG_INPUT,
			    &pin_data);

	if (ret != 0) {
		goto out;
	}

out:
	k_sem_give(&drv_data->lock);

	if ((ret == 0)
	    && ((int_source & drv_data->cb_pins) != 0)) {
		gpio_fire_callbacks(&drv_data->cb, dev, int_source);
	}

	return ret;
}

static void et6416_work_handler(struct k_work *work)
{
	struct et6416_drv_data *drv_data =
		CONTAINER_OF(work, struct et6416_drv_data, work);

	et6416_handle_interrupt(drv_data->dev);
}

static void et6416_int_cb(struct device *dev, struct gpio_callback *gpio_cb,
			   u32_t pins)
{
	struct et6416_drv_data *drv_data = CONTAINER_OF(gpio_cb,
		struct et6416_drv_data, gpio_cb);

	ARG_UNUSED(pins);

	k_work_submit(&drv_data->work);
}
#endif

static int gpio_et6416_config(const struct device *dev,
			  gpio_pin_t pin,
			  gpio_flags_t flags)
{
	const struct et6416_config *cfg = dev->config;
	struct et6416_drv_data *drv_data = dev->data;
	struct et6416_pin_state *pins = &drv_data->pin_state;
	int rc ;

	/* Can't do I2C bus operations from an ISR */
	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}
	k_sem_take(&drv_data->lock, K_FOREVER);

	if (flags & GPIO_OUTPUT) {
		pins->dir &= ~BIT(pin);
	}else {
		pins->dir |= BIT(pin); // INPUT
	}
	rc = i2c_reg_write_word(drv_data->i2c_master, cfg->i2c_slave_addr,
				   ET6416_REG_CFG, pins->dir);
	if (rc) {
		LOG_ERR("dir write err=%d\n",rc);
	}else{
		LOG_DBG("read dir=0x%x\n", drv_data->pin_state.dir);
	}

	k_sem_give(&drv_data->lock);
	return rc;
}

static int port_get(const struct device *dev,
		    gpio_port_value_t *value)
{
	const struct et6416_config *cfg = dev->config;
	struct et6416_drv_data *drv_data = dev->data;
	u16_t pin_data;
	int rc = 0;

	/* Can't do I2C bus operations from an ISR */
	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	k_sem_take(&drv_data->lock, K_FOREVER);
	rc = i2c_reg_read_word(drv_data->i2c_master, cfg->i2c_slave_addr,
			    ET6416_REG_INPUT,
			    &pin_data);
	LOG_DBG("read %04x got %d", pin_data, rc);
	if (rc != 0) {
		LOG_ERR("read err=%d\n",rc);
	}else{
		*value = pin_data;
		drv_data->pin_state.input_dat = pin_data;
	}

	k_sem_give(&drv_data->lock);
	return rc;
}

static int port_write(const struct device *dev,
		      gpio_port_pins_t mask,
		      gpio_port_value_t value,
		      gpio_port_value_t toggle)
{

	const struct et6416_config *cfg = dev->config;
	struct et6416_drv_data *drv_data = dev->data;
	u16_t *outp = &drv_data->pin_state.output_data;

	/* Can't do I2C bus operations from an ISR */
	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}
	k_sem_take(&drv_data->lock, K_FOREVER);
	u16_t orig_out = *outp;
	u16_t out = ((orig_out & ~mask) | (value & mask)) ^ toggle;
	int rc = i2c_reg_write_word(drv_data->i2c_master, cfg->i2c_slave_addr,
				       ET6416_REG_OUTPUT, out);
	if (rc == 0) {
		*outp = out;
	}else{
		LOG_ERR("write err=%d\n",rc);
	}
	k_sem_give(&drv_data->lock);
	LOG_DBG("write %04x msk %04x val %04x => %04x: %d", orig_out, mask, value, out, rc);

	return rc;
}

static int port_set_masked(const struct device *dev,
			   gpio_port_pins_t mask,
			   gpio_port_value_t value)
{
	return port_write(dev, mask, value, 0);
}

static int port_set_bits(const struct device *dev,
			 gpio_port_pins_t pins)
{
	return port_write(dev, pins, pins, 0);
}

static int port_clear_bits(const struct device *dev,
			   gpio_port_pins_t pins)
{
	return port_write(dev, pins, 0, 0);
}

static int port_toggle_bits(const struct device *dev,
			    gpio_port_pins_t pins)
{
	return port_write(dev, 0, 0, pins);
}

static int pin_interrupt_configure(const struct device *dev,
				   gpio_pin_t pin,
				   enum gpio_int_mode mode,
				   enum gpio_int_trig trig)
{
	int rc = 0;

	if (!IS_ENABLED(CONFIG_GPIO_ET6416_INTERRUPT)
	    && (mode != GPIO_INT_MODE_DISABLED)) {
		return -ENOTSUP;
	}

#ifdef CONFIG_GPIO_ET6416_INTERRUPT
	const struct et6416_config *cfg = dev->config;
	struct et6416_drv_data *drv_data = dev->data;
	struct et6416_irq_state *irq = &drv_data->irq_state;

	k_sem_take(&drv_data->lock, K_FOREVER);

	if (mode == GPIO_INT_MODE_DISABLED) {
		drv_data->cb_pins &= ~BIT(pin);
		irq->interrupt_mask |= BIT(pin);
	} else { /* GPIO_INT_MODE_EDGE */
		drv_data->cb_pins |= BIT(pin);
		irq->interrupt_mask &= ~BIT(pin);
		if (trig == GPIO_INT_TRIG_BOTH) {
			irq->interrupt_sense |= (ET6416_EDGE_BOTH <<
								(pin * 2));
		} else if (trig == GPIO_INT_TRIG_LOW) {
			irq->interrupt_sense |= (ET6416_EDGE_FALLING <<
								(pin * 2));
		} else if (trig == GPIO_INT_TRIG_HIGH) {
			irq->interrupt_sense |= (ET6416_EDGE_RISING <<
								(pin * 2));
		}
	}

	k_sem_give(&drv_data->lock);
#endif /* CONFIG_GPIO_ET6416_INTERRUPT */

	return rc;
}

/**
 * @brief Initialization function of ET6416
 *
 * @param dev Device struct
 * @return 0 if successful, failed otherwise.
 */

static int vdd18v_power_on(void)
{
	const struct device *gpio_dev;
	gpio_dev = device_get_binding(CONFIG_GPIO_PIN2NAME(CONFIG_GPIO_VDD18V_POWER_ON));
	if (!gpio_dev){
		printk("%d, vdd18 gpio dev fail!\n", __LINE__);
		return -EINVAL;
	}

	printk("%d, vdd18 poweron!\n", __LINE__);
	//gpio_pin_configure(gpio_dev, CONFIG_GPIO_VDD18V_POWER_ON%32, GPIO_OUTPUT | GPIO_PULL_UP);
	gpio_pin_configure(gpio_dev, CONFIG_GPIO_VDD18V_POWER_ON%32, GPIO_OUTPUT);
	gpio_pin_set(gpio_dev, CONFIG_GPIO_VDD18V_POWER_ON%32, 1);
	return 0;


}
static int et6416_init(const struct device *dev)
{
	const struct et6416_config *cfg = dev->config;
	struct et6416_drv_data *drv_data = dev->data;
	int rc;


	printk("gpio:et6416_init\n");
	vdd18v_power_on();
	drv_data->i2c_master = device_get_binding(cfg->i2c_master_dev_name);
	if (!drv_data->i2c_master) {
		LOG_ERR("%s: no bus %s", dev->name,
			cfg->i2c_master_dev_name);
		return -EINVAL;
	}

#ifdef CONFIG_GPIO_ET6416_INTERRUPT
	drv_data->dev = dev;
	if(!cfg->et6416_irq.use_gpio){
		LOG_ERR("gpio irq not cfg\n");
		return -EINVAL
	}

	drv_data->gpio_int = device_get_binding(cfg->et6416_irq.gpio_dev_name);
	if (!drv_data->gpio_int) {
		rc = -ENOTSUP;
		goto out;
	}
	k_work_init(&drv_data->work, et6416_work_handler);

	gpio_pin_configure(drv_data->gpio_int, cfg->et6416_irq.gpion,
			   GPIO_INPUT | cfg->et6416_irq.flag);
	gpio_pin_interrupt_configure(drv_data->gpio_int, cfg->et6416_irq.gpion,
				     GPIO_INT_EDGE_TO_ACTIVE);

	gpio_init_callback(&drv_data->gpio_cb, et6416_int_cb,
			   BIT(cfg->et6416_irq.gpion));
	gpio_add_callback(drv_data->gpio_int, &drv_data->gpio_cb);

#endif

	//rc = i2c_reg_write_word(drv_data->i2c_master, cfg->i2c_slave_addr,
				//ET6416_REG_CFG,0x0000); //output
	//LOG_DBG("write cfg 0x0000 ,rc=%d\n", rc);
	rc = i2c_reg_read_word(drv_data->i2c_master, cfg->i2c_slave_addr,
				ET6416_REG_CFG, &drv_data->pin_state.dir);
	LOG_DBG("read cfg=0x%x,rc=%d\n", drv_data->pin_state.dir, rc);

	rc = i2c_reg_read_word(drv_data->i2c_master, cfg->i2c_slave_addr,
				ET6416_REG_POLARITY_INVERSION, &drv_data->pin_state.polarity);
	LOG_DBG("read POLARITY=0x%x,rc=%d\n", drv_data->pin_state.polarity, rc);

	rc = i2c_reg_read_word(drv_data->i2c_master, cfg->i2c_slave_addr,
				ET6416_REG_INPUT, &drv_data->pin_state.input_dat);
	LOG_DBG("read indata=0x%x,rc=%d\n", drv_data->pin_state.input_dat, rc);

	rc = i2c_reg_read_word(drv_data->i2c_master, cfg->i2c_slave_addr,
				ET6416_REG_OUTPUT, &drv_data->pin_state.output_data);
	LOG_DBG("read outdata=0x%x,rc=%d\n", drv_data->pin_state.output_data, rc);


//out:
	if (rc != 0) {
		LOG_ERR("%s init failed: %d", dev->name, rc);
	} else {
		LOG_INF("%s init ok", dev->name);
	}

	return rc;
}

#ifdef CONFIG_GPIO_ET6416_INTERRUPT
static int gpio_et6416_manage_callback(const struct device *dev,
					  struct gpio_callback *callback,
					  bool set)
{
	struct et6416_drv_data *data = dev->data;

	return gpio_manage_callback(&data->cb, callback, set);
}

static int gpio_et6416_enable_callback(const struct device *dev,
					  gpio_pin_t pin)
{
	struct et6416_drv_data *data = dev->data;

	data->cb_pins |= BIT(pin);

	return 0;
}

static int gpio_et6416_disable_callback(const struct device *dev,
					   gpio_pin_t pin)
{
	struct et6416_drv_data *data = dev->data;

	data->cb_pins &= ~BIT(pin);

	return 0;
}
#endif

static const struct gpio_driver_api gpio_api_table = {
	.pin_configure = gpio_et6416_config,
	.port_get_raw = port_get,
	.port_set_masked_raw = port_set_masked,
	.port_set_bits_raw = port_set_bits,
	.port_clear_bits_raw = port_clear_bits,
	.port_toggle_bits = port_toggle_bits,
	.pin_interrupt_configure = pin_interrupt_configure,
#ifdef CONFIG_GPIO_ET6416_INTERRUPT
	.manage_callback = gpio_et6416_manage_callback,
	.enable_callback = gpio_et6416_enable_callback,
	.disable_callback = gpio_et6416_disable_callback,
#endif
};

static const struct et6416_config et6416_cfg = {
	.common = {
		.port_pin_mask = 0xffff,
	},
	.i2c_master_dev_name = CONFIG_EXTEND_GPIO_I2C_NAME,
#ifdef CONFIG_GPIO_ET6416_INTERRUPT
	.et6416_irq = CONFIG_EXTEND_GPIO_ISR_GPIO,
#endif
	.i2c_slave_addr = 0x20,
};

static struct et6416_drv_data et6416_drvdata = {
	.lock = Z_SEM_INITIALIZER(et6416_drvdata.lock, 1, 1),
};


#if IS_ENABLED(CONFIG_EXTEND_GPIO)
DEVICE_DEFINE(et6416, CONFIG_EXTEND_GPIO_NAME,
		    et6416_init, NULL, &et6416_drvdata, &et6416_cfg,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_OBJECTS,
		    &gpio_api_table);
#endif

