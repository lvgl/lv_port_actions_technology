/*
 * Copyright (c) 2019 Piotr Mienkowski
 * Copyright (c) 2018 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_GPIO_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_GPIO_H_

/**
 * @brief GPIO Driver APIs
 * @defgroup gpio_interface GPIO Driver APIs
 * @ingroup io_interfaces
 * @{
 */

/**
 * @name GPIO pin active level flags
 * @{
 */

/** GPIO pin is active (has logical value '1') in low state. */
#define GPIO_ACTIVE_LOW         (1 << 0)
/** GPIO pin is active (has logical value '1') in high state. */
#define GPIO_ACTIVE_HIGH        (0 << 0)

/** @} */

/**
 * @name GPIO pin drive flags
 * @{
 */

/** @cond INTERNAL_HIDDEN */

/* Configures GPIO output in single-ended mode (open drain or open source). */
#define GPIO_SINGLE_ENDED       (1 << 1)
/* Configures GPIO output in push-pull mode */
#define GPIO_PUSH_PULL          (0 << 1)

/* Indicates single ended open drain mode (wired AND). */
#define GPIO_LINE_OPEN_DRAIN    (1 << 2)
/* Indicates single ended open source mode (wired OR). */
#define GPIO_LINE_OPEN_SOURCE   (0 << 2)

/** @endcond */

/** Configures GPIO output in open drain mode (wired AND).
 *
 * @note 'Open Drain' mode also known as 'Open Collector' is an output
 * configuration which behaves like a switch that is either connected to ground
 * or disconnected.
 */
#define GPIO_OPEN_DRAIN         (GPIO_SINGLE_ENDED | GPIO_LINE_OPEN_DRAIN)
/** Configures GPIO output in open source mode (wired OR).
 *
 * @note 'Open Source' is a term used by software engineers to describe output
 * mode opposite to 'Open Drain'. It behaves like a switch that is either
 * connected to power supply or disconnected. There exist no corresponding
 * hardware schematic and the term is generally unknown to hardware engineers.
 */
#define GPIO_OPEN_SOURCE        (GPIO_SINGLE_ENDED | GPIO_LINE_OPEN_SOURCE)

/** @} */

/**
 * @name GPIO pin bias flags
 * @{
 */

/** Enables GPIO pin pull-up. */
#define GPIO_PULL_UP            (1 << 4)

/** Enable GPIO pin pull-down. */
#define GPIO_PULL_DOWN          (1 << 5)

/** @} */

/**
 * @name GPIO pin voltage flags
 *
 * The voltage flags are a Zephyr specific extension of the standard GPIO
 * flags specified by the Linux GPIO binding. Only applicable if SoC allows
 * to configure pin voltage per individual pin.
 *
 * @{
 */

/** @cond INTERNAL_HIDDEN */
#define GPIO_VOLTAGE_POS	6
#define GPIO_VOLTAGE_MASK	(3U << GPIO_VOLTAGE_POS)
/** @endcond */

/** Set pin at the default voltage level */
#define GPIO_VOLTAGE_DEFAULT	(0U << GPIO_VOLTAGE_POS)
/** Set pin voltage level at 1.8 V */
#define GPIO_VOLTAGE_1P8	(1U << GPIO_VOLTAGE_POS)
/** Set pin voltage level at 3.3 V */
#define GPIO_VOLTAGE_3P3	(2U << GPIO_VOLTAGE_POS)
/** Set pin voltage level at 5.0 V */
#define GPIO_VOLTAGE_5P0	(3U << GPIO_VOLTAGE_POS)

/** @} */

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_GPIO_H_ */
