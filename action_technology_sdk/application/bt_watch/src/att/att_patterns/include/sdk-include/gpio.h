#ifndef __GPIO_H
#define __GPIO_H

/** @cond INTERNAL_HIDDEN */
#define GPIO_ACCESS_BY_PIN 0
#define GPIO_ACCESS_BY_PORT 1
/** @endcond */

/** GPIO pin to be input. */
#define GPIO_DIR_IN		(0 << 0)

/** GPIO pin to be output. */
#define GPIO_DIR_OUT		(1 << 0)

/** @cond INTERNAL_HIDDEN */
#define GPIO_DIR_MASK		0x1
/** @endcond */

/** GPIO pin to trigger interrupt. */
#define GPIO_INT		(1 << 1)

/** GPIO pin trigger on level low or falling edge. */
#define GPIO_INT_ACTIVE_LOW	(0 << 2)

/** GPIO pin trigger on level high or rising edge. */
#define GPIO_INT_ACTIVE_HIGH	(1 << 2)

/** GPIO pin trigger to be synchronized to clock pulses. */
#define GPIO_INT_CLOCK_SYNC     (1 << 3)

/** Enable GPIO pin debounce. */
#define GPIO_INT_DEBOUNCE       (1 << 4)

/** Do Level trigger. */
#define GPIO_INT_LEVEL		(0 << 5)

/** Do Edge trigger. */
#define GPIO_INT_EDGE		(1 << 5)

/** Interrupt triggers on both rising and falling edge. */
#define GPIO_INT_DOUBLE_EDGE	(1 << 6)

/*
 * GPIO_POL_* define the polarity of the GPIO (1 bit).
 */

/** @cond INTERNAL_HIDDEN */
#define GPIO_POL_POS		7
/** @endcond */

/** GPIO pin polarity is normal. */
#define GPIO_POL_NORMAL		(0 << GPIO_POL_POS)

/** GPIO pin polarity is inverted. */
#define GPIO_POL_INV		(1 << GPIO_POL_POS)

/** @cond INTERNAL_HIDDEN */
#define GPIO_POL_MASK		(1 << GPIO_POL_POS)
/** @endcond */

/*
 * GPIO_PUD_* are related to pull-up/pull-down.
 */

/** @cond INTERNAL_HIDDEN */
#define GPIO_PUD_POS		8
/** @endcond */

/** GPIO pin to have no pull-up or pull-down. */
#define GPIO_PUD_NORMAL		(0 << GPIO_PUD_POS)

/** Enable GPIO pin pull-up. */
#define GPIO_PUD_PULL_UP	(1 << GPIO_PUD_POS)

/** Enable GPIO pin pull-down. */
#define GPIO_PUD_PULL_DOWN	(2 << GPIO_PUD_POS)

/** @cond INTERNAL_HIDDEN */
#define GPIO_PUD_MASK		(3 << GPIO_PUD_POS)
/** @endcond */

/*
 * GPIO_PIN_(EN-/DIS-)ABLE are for pin enable / disable.
 *
 * Individual pins can be enabled or disabled
 * if the controller supports this operation.
 */

/** Enable GPIO pin. */
#define GPIO_PIN_ENABLE		(1 << 10)

/** Disable GPIO pin. */
#define GPIO_PIN_DISABLE	(1 << 11)

/* GPIO_DS_* are for pin drive strength configuration.
 *
 * The drive strength of individual pins can be configured
 * independently for when the pin output is low and high.
 *
 * The GPIO_DS_*_LOW enumerations define the drive strength of a pin
 * when output is low.

 * The GPIO_DS_*_HIGH enumerations define the drive strength of a pin
 * when output is high.
 *
 * The DISCONNECT drive strength indicates that the pin is placed in a
 * high impedance state and not driven, this option is used to
 * configure hardware that supports a open collector drive mode.
 *
 * The interface supports two different drive strengths:
 * DFLT - The lowest drive strength supported by the HW
 * ALT - The highest drive strength supported by the HW
 *
 * On hardware that supports only one standard drive strength, both
 * DFLT and ALT have the same behavior.
 *
 * On hardware that does not support a disconnect mode, DISCONNECT
 * will behave the same as DFLT.
 */

/** @cond INTERNAL_HIDDEN */
#define GPIO_DS_LOW_POS 12
#define GPIO_DS_LOW_MASK (0x3 << GPIO_DS_LOW_POS)
/** @endcond */

/** Default drive strength standard when GPIO pin output is low.
 */
#define GPIO_DS_DFLT_LOW (0x0 << GPIO_DS_LOW_POS)

/** Alternative drive strength when GPIO pin output is low.
 * For hardware that does not support configurable drive strength
 * use the default drive strength.
 */
#define GPIO_DS_ALT_LOW (0x1 << GPIO_DS_LOW_POS)

/** Disconnect pin when GPIO pin output is low.
 * For hardware that does not support disconnect use the default
 * drive strength.
 */
#define GPIO_DS_DISCONNECT_LOW (0x3 << GPIO_DS_LOW_POS)

/** @cond INTERNAL_HIDDEN */
#define GPIO_DS_HIGH_POS 14
#define GPIO_DS_HIGH_MASK (0x3 << GPIO_DS_HIGH_POS)
/** @endcond */

/** Default drive strength when GPIO pin output is high.
 */
#define GPIO_DS_DFLT_HIGH (0x0 << GPIO_DS_HIGH_POS)

/** Alternative drive strength when GPIO pin output is high.
 * For hardware that does not support configurable drive strengths
 * use the default drive strength.
 */
#define GPIO_DS_ALT_HIGH (0x1 << GPIO_DS_HIGH_POS)

/** Disconnect pin when GPIO pin output is high.
 * For hardware that does not support disconnect use the default
 * drive strength.
 */
#define GPIO_DS_DISCONNECT_HIGH (0x3 << GPIO_DS_HIGH_POS)



#define GPIO_PADDRV_POS 16
#define GPIO_PADDRV_MASK (15 << GPIO_PADDRV_POS)
#define GPIO_PADDRV_SET_MASK (8 << GPIO_PADDRV_POS)
#define GPIO_PADDRV_LEVEL_MASK (7 << GPIO_PADDRV_POS)
#define GPIO_PADDRV_LEVEL(x) ((((int)x & 7) << GPIO_PADDRV_POS) | GPIO_PADDRV_SET_MASK)


/**
 * @cond INTERNAL_HIDDEN
 *
 * GPIO driver API definition.
 *
 * (Internal use only.)
 */
typedef int (*gpio_config_t)(struct device *port, int access_op,
			     u32_t pin, int flags);
typedef int (*gpio_write_t)(struct device *port, int access_op,
			    u32_t pin, u32_t value);
typedef int (*gpio_read_t)(struct device *port, int access_op,
			   u32_t pin, u32_t *value);

struct gpio_driver_api {
	gpio_config_t config;
	gpio_write_t write;
	gpio_read_t read;
};
/**
 * @endcond
 */

/**
 * @brief Configure a single pin.
 * @param port Pointer to device structure for the driver instance.
 * @param pin Pin number to configure.
 * @param flags Flags for pin configuration. IN/OUT, interrupt ...
 * @return 0 if successful, negative errno code on failure.
 */
static inline int gpio_pin_configure(struct device *port, u32_t pin,
				     int flags)
{
	const struct gpio_driver_api *api = port->driver_api;

	return api->config(port, GPIO_ACCESS_BY_PIN, pin, flags);
}

/**
 * @brief Write the data value to a single pin.
 * @param port Pointer to the device structure for the driver instance.
 * @param pin Pin number where the data is written.
 * @param value Value set on the pin.
 * @return 0 if successful, negative errno code on failure.
 */
static inline int gpio_pin_write(struct device *port, u32_t pin,
				 u32_t value)
{
	const struct gpio_driver_api *api = port->driver_api;

	return api->write(port, GPIO_ACCESS_BY_PIN, pin, value);
}

/**
 * @brief Read the data value of a single pin.
 *
 * Read the input state of a pin, returning the value 0 or 1.
 *
 * @param port Pointer to the device structure for the driver instance.
 * @param pin Pin number where data is read.
 * @param value Integer pointer to receive the data values from the pin.
 * @return 0 if successful, negative errno code on failure.
 */
static inline int gpio_pin_read(struct device *port, u32_t pin,
				u32_t *value)
{
	const struct gpio_driver_api *api = port->driver_api;

	return api->read(port, GPIO_ACCESS_BY_PIN, pin, value);
}


#endif