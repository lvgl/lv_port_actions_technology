/*
 * Copyright (c) 2018 Justin Watson
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <errno.h>
#include <kernel.h>
#include <device.h>
#include <init.h>
#include <soc.h>
#include <drivers/gpio.h>
#include <board_cfg.h>
#include "gpio_utils.h"

#define  GPIO_MAX_GRP	GPIO_MAX_GROUPS
#define  WIO_DELAY_TIME (200)

static struct device *g_gpio_dev[GPIO_MAX_GRP + 1];

struct gpio_acts_config {
	/* gpio_driver_config needs to be first */
	struct gpio_driver_config common;
	uint32_t base;
	uint8_t grp;
};

struct gpio_acts_runtime {
	/* gpio_data needs to be first */
	struct gpio_driver_data common;
	sys_slist_t cb;
};

#define DEV_CFG(dev) \
	((const struct gpio_acts_config * const)(dev)->config)
#define DEV_DATA(dev) \
	((struct gpio_acts_runtime * const)(dev)->data)


static int gpio_acts_port_get_raw(const struct device *dev, uint32_t *value)
{
	const struct gpio_acts_config * const cfg = DEV_CFG(dev);
	int pin_num = cfg->grp*32;
	unsigned int key;
	key = irq_lock();
#if defined(CONFIG_WIO) && (CONFIG_WIO == 1)
	if(cfg->grp != 3){
		*value = sys_read32(GPIO_REG_IDAT(cfg->base, pin_num));
	}else{
		*value = 0;
		for(pin_num = 0; pin_num < 5; pin_num++){
			*value |= ((sys_read32(WIO_REG_CTL(pin_num)) >> 16) & 0x01) << pin_num;
		}
	}
#else
	*value = sys_read32(GPIO_REG_IDAT(cfg->base, pin_num));
#endif
	irq_unlock(key);
	return 0;
}

static int gpio_acts_port_set_masked_raw(const struct device *dev, uint32_t mask,
					uint32_t value)
{
	const struct gpio_acts_config * const cfg = DEV_CFG(dev);
	int pin_num = cfg->grp*32;
	unsigned int val, key;
	key = irq_lock();
#if defined(CONFIG_WIO) && (CONFIG_WIO == 1)
	if(cfg->grp != 3){
		val = sys_read32(GPIO_REG_ODAT(cfg->base, pin_num));
		val = (val&~mask) | (mask & value);
		sys_write32(val, GPIO_REG_ODAT(cfg->base, pin_num));
	}else{
		val = 0;
		for(pin_num = 0; pin_num < 5; pin_num++){
			val |= ((sys_read32(WIO_REG_CTL(pin_num)) >> 16) & 0x01) << pin_num;
		}

		val = (val&~mask) | (mask & value);

		for(pin_num = 0; pin_num < 5; pin_num++){
			if(val & (1 << pin_num)){
				sys_write32(sys_read32(WIO_REG_CTL(pin_num)) | (1 << 16), WIO_REG_CTL(pin_num));
			}else{
				sys_write32(sys_read32(WIO_REG_CTL(pin_num)) & (~(1 << 16)), WIO_REG_CTL(pin_num));
			}
		}
	}
#else
	val = sys_read32(GPIO_REG_ODAT(cfg->base, pin_num));
	val = (val&~mask) | (mask & value);
	sys_write32(val, GPIO_REG_ODAT(cfg->base, pin_num));
#endif
	irq_unlock(key);

	if(cfg->grp == 3){
        k_busy_wait(WIO_DELAY_TIME);
	}
	return 0;
}

static int gpio_acts_port_set_bits_raw(const struct device *dev, uint32_t mask)
{

	const struct gpio_acts_config * const cfg = DEV_CFG(dev);
	int pin_num = cfg->grp*32;
	unsigned int key;
	key = irq_lock();
#if defined(CONFIG_WIO) && (CONFIG_WIO == 1)
	if(cfg->grp != 3){
		sys_write32(mask, GPIO_REG_BSR(cfg->base, pin_num));
	}else{
		for(pin_num = 0; pin_num < 5; pin_num++){
			if(mask & (1 << pin_num)){
				sys_write32(sys_read32(WIO_REG_CTL(pin_num)) | (1 << 16), WIO_REG_CTL(pin_num));
			}
		}
	}
#else
	sys_write32(mask, GPIO_REG_BSR(cfg->base, pin_num));
#endif
	irq_unlock(key);

	if(cfg->grp == 3){
        k_busy_wait(WIO_DELAY_TIME);
	}
	return 0;
}

static int gpio_acts_port_clear_bits_raw(const struct device *dev, uint32_t mask)
{
	const struct gpio_acts_config * const cfg = DEV_CFG(dev);
	int pin_num = cfg->grp*32;
	unsigned int key;
	key = irq_lock();
#if defined(CONFIG_WIO) && (CONFIG_WIO == 1)
	if(cfg->grp != 3){
		sys_write32(mask, GPIO_REG_BRR(cfg->base, pin_num));
	}else{
		for(pin_num = 0; pin_num < 5; pin_num++){
			if(mask & (1 << pin_num)){
				sys_write32(sys_read32(WIO_REG_CTL(pin_num)) & (~(1 << 16)), WIO_REG_CTL(pin_num));
			}
		}
	}
#else
	sys_write32(mask, GPIO_REG_BRR(cfg->base, pin_num));
#endif
	irq_unlock(key);

	if(cfg->grp == 3){
        k_busy_wait(WIO_DELAY_TIME);
	}
	return 0;
}

static int gpio_acts_config(const struct device *dev, gpio_pin_t pin,
			   gpio_flags_t flags)
{
	const struct gpio_acts_config * const cfg = DEV_CFG(dev);
	int pin_num = cfg->grp*32 + pin;
	unsigned int val, key, set_val, set_mask;
	int reg_addr;

	set_val = 0;
	set_mask = 0xfff;
	if (flags & GPIO_OUTPUT){
		if (flags & GPIO_OUTPUT_INIT_HIGH) {
			/* Set the pin. */
			gpio_acts_port_set_bits_raw(dev, BIT(pin));
		}
		if (flags & GPIO_OUTPUT_INIT_LOW) {
			/* Clear the pin. */
			gpio_acts_port_clear_bits_raw(dev, BIT(pin));
		}
		set_val = GPIO_CTL_GPIO_OUTEN;

	}else if (flags & GPIO_INPUT){
		set_val = GPIO_CTL_GPIO_INEN;

	}
	if (flags & GPIO_PULL_UP)
		set_val |= GPIO_CTL_PULLUP;
	if (flags & GPIO_PULL_DOWN)
		set_val |= GPIO_CTL_PULLDOWN;
	if (flags & GPIO_INT_DEBOUNCE)
		set_val |= GPIO_CTL_SMIT;

	key = irq_lock();
#if defined(CONFIG_WIO) && (CONFIG_WIO == 1)
	if(cfg->grp != 3){
		reg_addr = GPIO_REG_CTL(cfg->base, pin_num);
	}else{
		reg_addr = WIO_REG_CTL(pin);
	}
#else
	reg_addr = GPIO_REG_CTL(cfg->base, pin_num);
#endif

	val = sys_read32(reg_addr);
	val &= ~set_mask;
	val |= set_val;
	sys_write32(val, reg_addr);
	irq_unlock(key);

	if(cfg->grp == 3){
        k_busy_wait(WIO_DELAY_TIME);
	}
	return 0;;
}


static int gpio_acts_port_toggle_bits(const struct device *dev, uint32_t mask)
{
	const struct gpio_acts_config * const cfg = DEV_CFG(dev);
	int pin_num = cfg->grp*32;
	unsigned int val, key;
	key = irq_lock();
#if defined(CONFIG_WIO) && (CONFIG_WIO == 1)
	if(cfg->grp != 3){
		val = sys_read32(GPIO_REG_ODAT(cfg->base, pin_num));
		val ^= mask;
		sys_write32(val, GPIO_REG_ODAT(cfg->base, pin_num));
	}else{
		for(pin_num = 0; pin_num < 5; pin_num++){
			if(mask & (1 << pin_num)){
				sys_write32(sys_read32(WIO_REG_CTL(pin_num)) ^ (1 << 16), WIO_REG_CTL(pin_num));
			}
		}
	}
#else
	val = sys_read32(GPIO_REG_ODAT(cfg->base, pin_num));
	val ^= mask;
	sys_write32(val, GPIO_REG_ODAT(cfg->base, pin_num));
#endif
	irq_unlock(key);

	if(cfg->grp == 3){
        k_busy_wait(WIO_DELAY_TIME);
	}
	return 0;
}


static int gpio_acts_pin_interrupt_configure(const struct device *dev,
		gpio_pin_t pin, enum gpio_int_mode mode,
		enum gpio_int_trig trig)
{
	const struct gpio_acts_config * const cfg = DEV_CFG(dev);
	int pin_num, reg_addr;
	unsigned int val, key, set_val, set_mask;

#if defined(CONFIG_WIO) && (CONFIG_WIO == 1)
	if(cfg->grp != 3){
		pin_num = cfg->grp*32 + pin;
		reg_addr = GPIO_REG_CTL(cfg->base, pin_num);
	}else{
		pin_num = pin;
		reg_addr = WIO_REG_CTL(pin_num);
	}
#else
	pin_num = cfg->grp*32 + pin;
	reg_addr = GPIO_REG_CTL(cfg->base, pin_num);
#endif

	//must clear mfp mask to GPIO function
	set_mask = GPIO_CTL_INTC_MASK | GPIO_CTL_INTC_EN | GPIO_CTL_INC_TRIGGER_MASK | GPIO_CTL_MFP_MASK;
	set_val = GPIO_CTL_INTC_MASK | GPIO_CTL_INTC_EN;

	if(cfg->grp == 3){
		if(mode != GPIO_INT_MODE_DISABLED){
			//wio must set input enable
			set_val |= 	GPIO_CTL_GPIO_INEN | GPIO_CTL_PULLUP;
		}
	}

	if(mode == GPIO_INT_MODE_DISABLED){
		set_val = 0;
	}else if(mode == GPIO_INT_MODE_LEVEL){
		if(trig == GPIO_INT_TRIG_LOW)
			set_val |= GPIO_CTL_INC_TRIGGER_LOW_LEVEL;
		else
			set_val |= GPIO_CTL_INC_TRIGGER_HIGH_LEVEL;
	}else{ //GPIO_INT_MODE_EDGE
		if(trig == GPIO_INT_TRIG_LOW)
			set_val |= GPIO_CTL_INC_TRIGGER_FALLING_EDGE;
		else  if(trig == GPIO_INT_TRIG_HIGH)
			set_val |= GPIO_CTL_INC_TRIGGER_RISING_EDGE;
		else
			set_val |= GPIO_CTL_INC_TRIGGER_DUAL_EDGE;
	}

	key = irq_lock();
	val = sys_read32(reg_addr);
	val &= ~set_mask;
	val |= set_val;
	sys_write32(val, reg_addr);

	irq_unlock(key);

	if(cfg->grp == 3){
        k_busy_wait(WIO_DELAY_TIME);
	}
	return 0;
}

static void gpio_acts_isr(void *arg)
{
	struct device *dev = (struct device *)arg;
	const struct gpio_acts_config * const cfg = DEV_CFG(dev);
	struct gpio_acts_runtime *context;
	uint32_t int_stat, i;
	for(i = 0; i < GPIO_MAX_IRQ_GRP; i++ ){
		int_stat = sys_read32(GPIO_REG_IRQ_PD(cfg->base, i*32));
		if(int_stat){
			if(g_gpio_dev[i] != NULL){
				context = g_gpio_dev[i]->data;
				gpio_fire_callbacks(&context->cb, dev, int_stat);
			}else{
				printk("gpio-irq:err,grp=%d,stat=0x%x\n", i, int_stat);
			}
			sys_write32(int_stat, GPIO_REG_IRQ_PD(cfg->base, i*32));
		}
	}

#if defined(CONFIG_WIO) && (CONFIG_WIO == 1)
	for(i = 0; i < GPIO_WIO_MAX_PIN_NUM; i++ ){
		int_stat = sys_read32(WIO_REG_CTL(i)) & WIO_CTL_INT_PD_MASK;
		if(int_stat){
			if(g_gpio_dev[GPIO_MAX_IRQ_GRP] != NULL){
				context = g_gpio_dev[GPIO_MAX_IRQ_GRP]->data;
				gpio_fire_callbacks(&context->cb, dev, (1 << i));
			}else{
				printk("WIO-irq:err,grp=%d,stat=0x%x\n", i, int_stat);
			}
			sys_write32(sys_read32(WIO_REG_CTL(i)), WIO_REG_CTL(i));
		}
	}
#endif
}

static int gpio_acts_manage_callback(const struct device *port,
				    struct gpio_callback *callback,
				    bool set)
{
	struct gpio_acts_runtime *context = port->data;

	return gpio_manage_callback(&context->cb, callback, set);
}

#if 0
static int gpio_acts_irq_set(const struct device *port,
					 gpio_pin_t pin, int eable)
{
	const struct gpio_acts_config * const cfg = DEV_CFG(port);
	int pin_num = cfg->grp*32 + pin;
	unsigned int val, key;
	key = irq_lock();
	val = sys_read32(GPIO_REG_CTL(cfg->base, pin_num));
	if(eable)
		val |= (GPIO_CTL_INTC_MASK | GPIO_CTL_INTC_EN);
	else
		val &= ~(GPIO_CTL_INTC_MASK | GPIO_CTL_INTC_EN);
	sys_write32(val, GPIO_REG_CTL(cfg->base, pin_num));
	irq_unlock(key);
	return 0;
}
#endif

static const struct gpio_driver_api gpio_acts_api = {
	.pin_configure = gpio_acts_config,
	.port_get_raw = gpio_acts_port_get_raw,
	.port_set_masked_raw = gpio_acts_port_set_masked_raw,
	.port_set_bits_raw = gpio_acts_port_set_bits_raw,
	.port_clear_bits_raw = gpio_acts_port_clear_bits_raw,
	.port_toggle_bits = gpio_acts_port_toggle_bits,
	.pin_interrupt_configure = gpio_acts_pin_interrupt_configure,
	.manage_callback = gpio_acts_manage_callback,
};

static void port_acts_config_func(const struct device *dev);

int gpio_acts_init(const struct device *dev)
{
	static uint8_t b_init=0;
	const struct gpio_acts_config * const cfg = DEV_CFG(dev);

	if(cfg->grp <= GPIO_MAX_GRP) {
		if(g_gpio_dev[cfg->grp] != NULL){
			//printk("gpio:grp=%d is register\n", cfg->grp);
		}else{
			g_gpio_dev[cfg->grp] = (struct device *)dev; /*save for irq func*/
			//printk("gpio:grp=%d ok\n", cfg->grp);
		}
	}else{
		//printk("gpio:grp=%d err\n", cfg->grp);
		return 0;
	}

	if(b_init == 0){
		port_acts_config_func(dev);
		b_init = 1;
		printk("gpio irq init\n");
	}
	return 0;
}

#define GPIO_ACTS_INIT(n, devname)						\
 	static const struct gpio_acts_config port_##n##_acts_config = {	\
		.common = {						\
			.port_pin_mask = 0xffffffff,\
		},							\
		.base = GPIO_REG_BASE,			\
		.grp = n,		\
	};								\
									\
	static struct gpio_acts_runtime port_##n##_acts_runtime;		\
									\
	DEVICE_DEFINE(port_##n##_acts, devname,		\
			    gpio_acts_init, NULL, &port_##n##_acts_runtime,	\
			    &port_##n##_acts_config, PRE_KERNEL_1,	\
			    CONFIG_KERNEL_INIT_PRIORITY_OBJECTS,		\
			    &gpio_acts_api);


/*DT_INST_FOREACH_STATUS_OKAY(GPIO_ACTS_INIT)*/

#if defined(CONFIG_GPIO_A) && (CONFIG_GPIO_A == 1)
	GPIO_ACTS_INIT(0, CONFIG_GPIO_A_NAME)
#endif

#if defined(CONFIG_GPIO_B) && (CONFIG_GPIO_B == 1)
	GPIO_ACTS_INIT(1, CONFIG_GPIO_B_NAME)
#endif

#if defined(CONFIG_GPIO_C) && (CONFIG_GPIO_C == 1)
	GPIO_ACTS_INIT(2, CONFIG_GPIO_C_NAME)
#endif

#if defined(CONFIG_WIO) && (CONFIG_WIO == 1)
	GPIO_ACTS_INIT(3, CONFIG_WIO_NAME)
#endif

static void port_acts_config_func(const struct device *dev)
{
	IRQ_CONNECT(IRQ_ID_GPIO, CONFIG_GPIO_IRQ_PRI,
			gpio_acts_isr,
			DEVICE_GET(port_0_acts), 0);
	irq_enable(IRQ_ID_GPIO);
}

