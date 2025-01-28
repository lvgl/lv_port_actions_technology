/*
 * Copyright (c) 2016 Intel Corporation.
 * Copyright (c) 2020-2021 Vestas Wind Systems A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public PWM Driver APIs
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_PWM_H_
#define ZEPHYR_INCLUDE_DRIVERS_PWM_H_

/**
 * @brief PWM Interface
 * @defgroup pwm_interface PWM Interface
 * @ingroup io_interfaces
 * @{
 */

#include <errno.h>
#include <zephyr/types.h>
#include <stddef.h>
#include <sys/math_extras.h>
#include <device.h>
#include <dt-bindings/pwm/pwm.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @name PWM capture configuration flags
 * @{
 */

/** @cond INTERNAL_HIDDEN */
/* Bit 0 is used for PWM_POLARITY_NORMAL/PWM_POLARITY_INVERTED */
#define PWM_CAPTURE_TYPE_SHIFT		1U
#define PWM_CAPTURE_TYPE_MASK		(3U << PWM_CAPTURE_TYPE_SHIFT)
#define PWM_CAPTURE_MODE_SHIFT		3U
#define PWM_CAPTURE_MODE_MASK		(1U << PWM_CAPTURE_MODE_SHIFT)

/** @endcond */

/** PWM pin capture captures period. */
#define PWM_CAPTURE_TYPE_PERIOD		(1U << PWM_CAPTURE_TYPE_SHIFT)

/** PWM pin capture captures pulse width. */
#define PWM_CAPTURE_TYPE_PULSE		(2U << PWM_CAPTURE_TYPE_SHIFT)

/** PWM pin capture captures both period and pulse width. */
#define PWM_CAPTURE_TYPE_BOTH		(PWM_CAPTURE_TYPE_PERIOD | \
					 PWM_CAPTURE_TYPE_PULSE)

/** PWM pin capture captures a single period/pulse width. */
#define PWM_CAPTURE_MODE_SINGLE		(0U << PWM_CAPTURE_MODE_SHIFT)

/** PWM pin capture captures period/pulse width continuously. */
#define PWM_CAPTURE_MODE_CONTINUOUS	(1U << PWM_CAPTURE_MODE_SHIFT)

typedef struct {
#define PWM_BREATH_RISE_TIME_DEFAULT	(500)
#define PWM_BREATH_DOWN_TIME_DEFAULT	(500)
#define PWM_BREATH_HIGH_TIME_DEFAULT	(500)
#define PWM_BREATH_LOW_TIME_DEFAULT		(2000)
/* lark */
	u16_t rise_time_ms;
	u16_t down_time_ms;
	u16_t high_time_ms;
	u16_t low_time_ms;
/* pearlriver */
	u16_t pwm_count_max;

	u8_t stage_high_wait;
	u8_t stage_low_wait;

	u8_t stage_a_step;
	u8_t stage_b_step;
	u8_t stage_c_step;
	u8_t stage_d_step;
	u8_t stage_e_step;
	u8_t stage_f_step;

	u8_t stage_a_repeat;
	u8_t stage_b_repeat;
	u8_t stage_c_repeat;
	u8_t stage_d_repeat;
	u8_t stage_e_repeat;
	u8_t stage_f_repeat;

	u8_t stage_a_pwm;
	u8_t stage_b_pwm;
	u8_t stage_c_pwm;
	u8_t stage_d_pwm;
	u8_t stage_e_pwm;
	u8_t stage_f_pwm;
	u8_t start_pwm;
	u8_t start_dir;
} pwm_breath_ctrl_t;

typedef struct {
#define PWM_PROGRAM_DMA_IRQ_HF	(1 << 0)
#define PWM_PROGRAM_DMA_IRQ_TC	(1 << 1)
	u32_t period_cycles;
	u16_t *ram_buf;
	u32_t ram_buf_len;
	int (*program_callback)(void *cb_data, u8_t reason);
	void *cb_data;
	u8_t reload_en : 1;
	u16_t repeat;
	u16_t cntmax;
} pwm_program_ctrl_t;

typedef struct {
	volatile u32_t ir_period;
	volatile u32_t ir_duty;
	volatile u32_t ir_lc;
	volatile u32_t ir_pl0_pre;
	volatile u32_t ir_pl0_post;
	volatile u32_t ir_pl1_pre;
	volatile u32_t ir_pl1_post;
	volatile u32_t ir_ll;
	volatile u32_t ir_ld;
	volatile u32_t ir_pl;
	volatile u32_t ir_pd0;
	volatile u32_t ir_pd1;
	volatile u32_t ir_sl;
	volatile u32_t ir_asc;
	volatile u32_t buf_num;
	u16_t mode;

} pwm_ir_ctrl_t;

/** @} */

/**
 * @brief Provides a type to hold PWM configuration flags.
 */
typedef uint8_t pwm_flags_t;

/**
 * @typedef pwm_pin_set_t
 * @brief Callback API upon setting the pin
 * See @a pwm_pin_set_cycles() for argument description
 */
typedef int (*pwm_pin_set_t)(const struct device *dev, uint32_t pwm,
			     uint32_t period_cycles, uint32_t pulse_cycles,
			     pwm_flags_t flags);

/**
 * @typedef pwm_set_breath_t
 * @brief Callback API upon setting pwm breath mode.
 * See @a pwm_set_breath_mode() for argument description
 */
typedef int (*pwm_set_breath_t)(const struct device *dev, u32_t pwm, pwm_breath_ctrl_t *ctrl);

/**
 * @typedef pwm_set_program_t
 * @brief Callback API upon setting pwm program mode.
 * See @a pwm_set_program_mode() for argument description
 */
typedef int (*pwm_set_program_t)(const struct device *dev, u32_t pwm, pwm_program_ctrl_t *ctrl);

/**
 * @typedef pwm_ir_transfer_t
 * @brief Callback API upon setting pwm ir mode.
 * See @a pwm_ir_transfer_mode() for argument description
 */
typedef int (*pwm_ir_transfer_t)(const struct device *dev, u32_t pwm, pwm_ir_ctrl_t *ctrl);

/**
 * @typedef pwm_pin_stop_t
 * @brief Callback API upon stopping the specified pwm.
 * See @a pwm_pin_stop() for argument description
 */
typedef int (*pwm_pin_stop_t)(const struct device *dev, u32_t pwm);

/**
 * @typedef pwm_pin_mfp_set_t
 * @brief Callback API upon init the specified pwm.
 * See @a pwm_pin_mfp_set() for argument description
 */
typedef int (*pwm_pin_mfp_set_t)(const struct device *dev, u32_t pwm, u32_t pin);

/**
 * @typedef pwm_pin_freq_duty_set_t
 * @brief Callback API upon setting the pin
 * See @a pwm_pin_freq_duty_set() for argument description
 */
typedef int (*pwm_pin_freq_duty_set_t)(const struct device *dev, uint32_t pwm,
			     uint32_t freq, uint8_t duty, pwm_flags_t flags);

/**
 * @typedef pwm_capture_callback_handler_t
 * @brief PWM capture callback handler function signature
 *
 * @note The callback handler will be called in interrupt context.
 *
 * @note @kconfig{CONFIG_PWM_CAPTURE} must be selected to enable PWM capture
 * support.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param pwm PWM pin.

 * @param period_cycles Captured PWM period width (in clock cycles). HW
 *                      specific.
 * @param pulse_cycles Captured PWM pulse width (in clock cycles). HW specific.
 * @param status Status for the PWM capture (0 if no error, negative errno
 *               otherwise. See @a pwm_pin_capture_cycles() return value
 *               descriptions for details).
 * @param user_data User data passed to @a pwm_pin_configure_capture()
 */
typedef void (*pwm_capture_callback_handler_t)(const struct device *dev,
					       uint32_t pwm,
					       uint32_t period_cycles,
					       uint32_t pulse_cycles,
					       int status,
					       void *user_data);

/**
 * @typedef pwm_pin_configure_capture_t
 * @brief Callback API upon configuring PWM pin capture
 * See @a pwm_pin_configure_capture() for argument description
 */
typedef int (*pwm_pin_configure_capture_t)(const struct device *dev,
					   uint32_t pwm,
					   pwm_flags_t flags,
					   pwm_capture_callback_handler_t cb,
					   void *user_data);
/**
 * @typedef pwm_pin_enable_capture_t
 * @brief Callback API upon enabling PWM pin capture
 * See @a pwm_pin_enable_capture() for argument description
 */
typedef int (*pwm_pin_enable_capture_t)(const struct device *dev,
					uint32_t pwm);

/**
 * @typedef pwm_pin_disable_capture_t
 * @brief Callback API upon disabling PWM pin capture
 * See @a pwm_pin_disable_capture() for argument description
 */
typedef int (*pwm_pin_disable_capture_t)(const struct device *dev,
					 uint32_t pwm);

/**
 * @typedef pwm_get_cycles_per_sec_t
 * @brief Callback API upon getting cycles per second
 * See @a pwm_get_cycles_per_sec() for argument description
 */
typedef int (*pwm_get_cycles_per_sec_t)(const struct device *dev,
					uint32_t pwm,
					uint64_t *cycles);

/** @brief PWM driver API definition. */
__subsystem struct pwm_driver_api {
	pwm_pin_set_t pin_set;
	pwm_set_breath_t set_breath;
	pwm_set_program_t set_program;
	pwm_ir_transfer_t ir_transfer;
	pwm_pin_stop_t pin_stop;
	pwm_pin_mfp_set_t pin_mfp;
	pwm_pin_freq_duty_set_t pin_set_freq_duty;

#ifdef CONFIG_PWM_CAPTURE
	pwm_pin_configure_capture_t pin_configure_capture;
	pwm_pin_enable_capture_t pin_enable_capture;
	pwm_pin_disable_capture_t pin_disable_capture;
#endif /* CONFIG_PWM_CAPTURE */
	pwm_get_cycles_per_sec_t get_cycles_per_sec;
};

/**
 * @brief Set the period and pulse width for a single PWM output.
 *
 * The PWM period and pulse width will synchronously be set to the new values
 * without glitches in the PWM signal, but the call will not block for the
 * change to take effect.
 *
 * @note Not all PWM controllers support synchronous, glitch-free updates of the
 * PWM period and pulse width. Depending on the hardware, changing the PWM
 * period and/or pulse width may cause a glitch in the generated PWM signal.
 *
 * @note Some multi-channel PWM controllers share the PWM period across all
 * channels. Depending on the hardware, changing the PWM period for one channel
 * may affect the PWM period for the other channels of the same PWM controller.
 *
 * Passing 0 as @p pulse will cause the pin to be driven to a constant
 * inactive level.
 * Passing a non-zero @p pulse equal to @p period will cause the pin
 * to be driven to a constant active level.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param pwm PWM pin.
 * @param period Period (in clock cycle) set to the PWM. HW specific.
 * @param pulse Pulse width (in clock cycle) set to the PWM. HW specific.
 * @param flags Flags for pin configuration (polarity).
 *
 * @retval 0 If successful.
 * @retval Negative errno code if failure.
 */
__syscall int pwm_pin_set_cycles(const struct device *dev, uint32_t pwm,
				 uint32_t period, uint32_t pulse, pwm_flags_t flags);

static inline int z_impl_pwm_pin_set_cycles(const struct device *dev,
					    uint32_t pwm,
					    uint32_t period, uint32_t pulse,
					    pwm_flags_t flags)
{
	struct pwm_driver_api *api;

	api = (struct pwm_driver_api *)dev->api;
	return api->pin_set(dev, pwm, period, pulse, flags);
}

/**
 * @brief Set the pwm breath mode for for a single PWM output.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param pwm PWM pin.
 * @param ctrl Pointer to the control parameters of pwm breath mode.
 * @retval 0 If successful.
 * @retval Negative errno code if failure.
 */
static inline int pwm_set_breath_mode(const struct device *dev, u32_t pwm, pwm_breath_ctrl_t *ctrl)
{
	struct pwm_driver_api *api;

	api = (struct pwm_driver_api *)dev->api;
	return api->set_breath(dev, pwm, ctrl);
}

/**
 * @brief Set the pwm program mode for for a single PWM output.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param pwm PWM pin.
 * @param ctrl Pointer to the control parameters of pwm program mode.
 * @retval 0 If successful.
 * @retval Negative errno code if failure.
 */
static inline int pwm_set_program_mode(const struct device *dev, u32_t pwm, pwm_program_ctrl_t *ctrl)
{
	struct pwm_driver_api *api;

	api = (struct pwm_driver_api *)dev->api;
	return api->set_program(dev, pwm, ctrl);
}

/**
 * @brief Set the pwm ir transfer for for a single PWM output.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param pwm PWM pin.
 * @param ctrl Pointer to the control parameters of pwm ir mode.
 * @retval 0 If successful.
 * @retval Negative errno code if failure.
 */
static inline int pwm_ir_transfer(const struct device *dev, u32_t pwm, pwm_ir_ctrl_t *ctrl)
{
	struct pwm_driver_api *api;

	api = (struct pwm_driver_api *)dev->api;
	return api->ir_transfer(dev, pwm, ctrl);
}

/**
 * @brief Stop the PWM output by the specified pwm pin
 * @note This API cooperates with pwm_set_program_mode() function.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param pwm PWM pin.
 * @retval 0 If successful.
 * @retval Negative errno code if failure.
 */
static inline int pwm_pin_stop(const struct device *dev, u32_t pwm)
{
	struct pwm_driver_api *api;

	api = (struct pwm_driver_api *)dev->api;
	return api->pin_stop(dev, pwm);
}

/**
 * @brief Stop the PWM output by the specified pwm pin
 * @note This API cooperates with pwm_set_program_mode() function.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param pwm PWM chan.
 * @param pwm PWM pin.
 * @retval 0 If successful.
 * @retval Negative errno code if failure.
 */
static inline int pwm_pin_mfp_set(const struct device *dev, u32_t pwm, u32_t pin)
{
	struct pwm_driver_api *api;

	api = (struct pwm_driver_api *)dev->api;
	return api->pin_mfp(dev, pwm, pin);
}

/**
 * @brief Set the frequency and duty cycle for a single PWM output.
 * @param dev Pointer to the device structure for the driver instance.
 * @param pwm PWM pin.
 * @param freq frequency of PWM.
 * @param duty duty cycle of PWM.
 * @param flags Flags for pin configuration (polarity).
 *
 * @retval 0 If successful.
 * @retval Negative errno code if failure.
 */
static inline int pwm_pin_set_freq_duty(const struct device *dev,
					    uint32_t pwm, uint32_t freq, uint8_t duty, pwm_flags_t flags)
{
	struct pwm_driver_api *api;

	api = (struct pwm_driver_api *)dev->api;
	return api->pin_set_freq_duty(dev, pwm, freq, duty, flags);
}

/**
 * @brief Configure PWM period/pulse width capture for a single PWM input.
 *
 * After configuring PWM capture using this function, the capture can be
 * enabled/disabled using @a pwm_pin_enable_capture() and @a
 * pwm_pin_disable_capture().
 *
 * @note This API function cannot be invoked from user space due to the use of a
 * function callback. In user space, one of the simpler API functions (@a
 * pwm_pin_capture_cycles(), @a pwm_pin_capture_usec(), or @a
 * pwm_pin_capture_nsec()) can be used instead.
 *
 * @note @kconfig{CONFIG_PWM_CAPTURE} must be selected for this function to be
 * available.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param pwm PWM pin.
 * @param flags PWM capture flags
 * @param cb Application callback handler function to be called upon capture
 * @param user_data User data to pass to the application callback handler
 *                  function
 *
 * @retval -EINVAL if invalid function parameters were given
 * @retval -ENOSYS if PWM capture is not supported or the given flags are not
 *                  supported
 * @retval -EIO if IO error occurred while configuring
 * @retval -EBUSY if PWM capture is already in progress
 */
#ifdef CONFIG_PWM_CAPTURE
static inline int pwm_pin_configure_capture(const struct device *dev,
					    uint32_t pwm,
					    pwm_flags_t flags,
					    pwm_capture_callback_handler_t cb,
					    void *user_data)
{
	const struct pwm_driver_api *api = (struct pwm_driver_api *)dev->api;

	if (api->pin_configure_capture == NULL) {
		return -ENOSYS;
	}

	return api->pin_configure_capture(dev, pwm, flags, cb, user_data);
}
#endif /* CONFIG_PWM_CAPTURE */

/**
 * @brief Enable PWM period/pulse width capture for a single PWM input.
 *
 * The PWM pin must be configured using @a pwm_pin_configure_capture() prior to
 * calling this function.
 *
 * @note @kconfig{CONFIG_PWM_CAPTURE} must be selected for this function to be
 * available.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param pwm PWM pin.
 *
 * @retval 0 If successful.
 * @retval -EINVAL if invalid function parameters were given
 * @retval -ENOSYS if PWM capture is not supported
 * @retval -EIO if IO error occurred while enabling PWM capture
 * @retval -EBUSY if PWM capture is already in progress
 */
__syscall int pwm_pin_enable_capture(const struct device *dev, uint32_t pwm);

#ifdef CONFIG_PWM_CAPTURE
static inline int z_impl_pwm_pin_enable_capture(const struct device *dev,
						uint32_t pwm)
{
	const struct pwm_driver_api *api = (struct pwm_driver_api *)dev->api;

	if (api->pin_enable_capture == NULL) {
		return -ENOSYS;
	}

	return api->pin_enable_capture(dev, pwm);
}
#endif /* CONFIG_PWM_CAPTURE */

/**
 * @brief Disable PWM period/pulse width capture for a single PWM input.
 *
 *
 * @note @kconfig{CONFIG_PWM_CAPTURE} must be selected for this function to be
 * available.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param pwm PWM pin.
 *
 * @retval 0 If successful.
 * @retval -EINVAL if invalid function parameters were given
 * @retval -ENOSYS if PWM capture is not supported
 * @retval -EIO if IO error occurred while disabling PWM capture
 */
__syscall int pwm_pin_disable_capture(const struct device *dev, uint32_t pwm);

#ifdef CONFIG_PWM_CAPTURE
static inline int z_impl_pwm_pin_disable_capture(const struct device *dev,
						 uint32_t pwm)
{
	const struct pwm_driver_api *api = (struct pwm_driver_api *)dev->api;

	if (api->pin_disable_capture == NULL) {
		return -ENOSYS;
	}

	return api->pin_disable_capture(dev, pwm);
}
#endif /* CONFIG_PWM_CAPTURE */

/**
 * @brief Capture a single PWM period/pulse width in clock cycles for a single
 *        PWM input.
 *
 * This API function wraps calls to @a pwm_pin_configure_capture(), @a
 * pwm_pin_enable_capture(), and @a pwm_pin_disable_capture() and passes the
 * capture result to the caller. The function is blocking until either the PWM
 * capture is completed or a timeout occurs.
 *
 * @note @kconfig{CONFIG_PWM_CAPTURE} must be selected for this function to be
 * available.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param pwm PWM pin.
 * @param flags PWM capture flags.
 * @param period Pointer to the memory to store the captured PWM period width
 *               (in clock cycles). HW specific.
 * @param pulse Pointer to the memory to store the captured PWM pulse width (in
 *              clock cycles). HW specific.
 * @param timeout Waiting period for the capture to complete.
 *
 * @retval 0 If successful.
 * @retval -EBUSY PWM capture already in progress.
 * @retval -EAGAIN Waiting period timed out.
 * @retval -EIO IO error while capturing.
 * @retval -ERANGE If result is too large.
 */
__syscall int pwm_pin_capture_cycles(const struct device *dev, uint32_t pwm,
				     pwm_flags_t flags,
				     uint32_t *period,
				     uint32_t *pulse,
				     k_timeout_t timeout);

/**
 * @brief Get the clock rate (cycles per second) for a single PWM output.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param pwm PWM pin.
 * @param cycles Pointer to the memory to store clock rate (cycles per sec).
 *		 HW specific.
 *
 * @retval 0 If successful.
 * @retval Negative errno code if failure.
 */
__syscall int pwm_get_cycles_per_sec(const struct device *dev, uint32_t pwm,
				     uint64_t *cycles);

static inline int z_impl_pwm_get_cycles_per_sec(const struct device *dev,
						uint32_t pwm,
						uint64_t *cycles)
{
	struct pwm_driver_api *api;

	api = (struct pwm_driver_api *)dev->api;
	return api->get_cycles_per_sec(dev, pwm, cycles);
}

/**
 * @brief Set the period and pulse width for a single PWM output.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param pwm PWM pin.
 * @param period Period (in microseconds) set to the PWM.
 * @param pulse Pulse width (in microseconds) set to the PWM.
 * @param flags Flags for pin configuration (polarity).
 *
 * @retval 0 If successful.
 * @retval Negative errno code if failure.
 */
static inline int pwm_pin_set_usec(const struct device *dev, uint32_t pwm,
				   uint32_t period, uint32_t pulse,
				   pwm_flags_t flags)
{
	uint64_t period_cycles, pulse_cycles, cycles_per_sec;

	if (pwm_get_cycles_per_sec(dev, pwm, &cycles_per_sec) != 0) {
		return -EIO;
	}

	period_cycles = (period * cycles_per_sec) / USEC_PER_SEC;
	if (period_cycles >= ((uint64_t)1 << 32)) {
		return -ENOTSUP;
	}

	pulse_cycles = (pulse * cycles_per_sec) / USEC_PER_SEC;
	if (pulse_cycles >= ((uint64_t)1 << 32)) {
		return -ENOTSUP;
	}

	return pwm_pin_set_cycles(dev, pwm, (uint32_t)period_cycles,
				  (uint32_t)pulse_cycles, flags);
}

/**
 * @brief Set the period and pulse width for a single PWM output.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param pwm PWM pin.
 * @param period Period (in nanoseconds) set to the PWM.
 * @param pulse Pulse width (in nanoseconds) set to the PWM.
 * @param flags Flags for pin configuration (polarity).
 *
 * @retval 0 If successful.
 * @retval Negative errno code if failure.
 */
static inline int pwm_pin_set_nsec(const struct device *dev, uint32_t pwm,
				   uint32_t period, uint32_t pulse,
				   pwm_flags_t flags)
{
	uint64_t period_cycles, pulse_cycles, cycles_per_sec;

	if (pwm_get_cycles_per_sec(dev, pwm, &cycles_per_sec) != 0) {
		return -EIO;
	}

	period_cycles = (period * cycles_per_sec) / NSEC_PER_SEC;
	if (period_cycles >= ((uint64_t)1 << 32)) {
		return -ENOTSUP;
	}

	pulse_cycles = (pulse * cycles_per_sec) / NSEC_PER_SEC;
	if (pulse_cycles >= ((uint64_t)1 << 32)) {
		return -ENOTSUP;
	}

	return pwm_pin_set_cycles(dev, pwm, (uint32_t)period_cycles,
				  (uint32_t)pulse_cycles, flags);
}

/**
 * @brief Convert from PWM cycles to microseconds.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param pwm PWM pin.
 * @param cycles Cycles to be converted.
 * @param usec Pointer to the memory to store calculated usec.
 *
 * @retval 0 If successful.
 * @retval -EIO If cycles per second cannot be determined.
 * @retval -ERANGE If result is too large.
 */
static inline int pwm_pin_cycles_to_usec(const struct device *dev, uint32_t pwm,
					 uint32_t cycles, uint64_t *usec)
{
	uint64_t cycles_per_sec;
	uint64_t temp;

	if (pwm_get_cycles_per_sec(dev, pwm, &cycles_per_sec) != 0) {
		return -EIO;
	}

	if (u64_mul_overflow(cycles, (uint64_t)USEC_PER_SEC, &temp)) {
		return -ERANGE;
	}

	*usec = temp / cycles_per_sec;

	return 0;
}

/**
 * @brief Convert from PWM cycles to nanoseconds.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param pwm PWM pin.
 * @param cycles Cycles to be converted.
 * @param nsec Pointer to the memory to store the calculated nsec.
 *
 * @retval 0 If successful.
 * @retval -EIO If cycles per second cannot be determined.
 * @retval -ERANGE If result is too large.
 */
static inline int pwm_pin_cycles_to_nsec(const struct device *dev, uint32_t pwm,
					 uint32_t cycles, uint64_t *nsec)
{
	uint64_t cycles_per_sec;
	uint64_t temp;

	if (pwm_get_cycles_per_sec(dev, pwm, &cycles_per_sec) != 0) {
		return -EIO;
	}

	if (u64_mul_overflow(cycles, (uint64_t)NSEC_PER_SEC, &temp)) {
		return -ERANGE;
	}

	*nsec = temp / cycles_per_sec;

	return 0;
}

/**
 * @brief Capture a single PWM period/pulse width in microseconds for a single
 *        PWM input.
 *
 * This API function wraps calls to @a pwm_pin_capture_cycles() and @a
 * pwm_pin_cycles_to_usec() and passes the capture result to the caller. The
 * function is blocking until either the PWM capture is completed or a timeout
 * occurs.
 *
 * @note @kconfig{CONFIG_PWM_CAPTURE} must be selected for this function to be
 * available.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param pwm PWM pin.
 * @param flags PWM capture flags.
 * @param period Pointer to the memory to store the captured PWM period width
 *               (in usec).
 * @param pulse Pointer to the memory to store the captured PWM pulse width (in
 *              usec).
 * @param timeout Waiting period for the capture to complete.
 *
 * @retval 0 If successful.
 * @retval -EBUSY PWM capture already in progress.
 * @retval -EAGAIN Waiting period timed out.
 * @retval -EIO IO error while capturing.
 * @retval -ERANGE If result is too large.
 */
static inline int pwm_pin_capture_usec(const struct device *dev, uint32_t pwm,
				       pwm_flags_t flags,
				       uint64_t *period,
				       uint64_t *pulse,
				       k_timeout_t timeout)
{
	uint32_t period_cycles;
	uint32_t pulse_cycles;
	int err;

	err = pwm_pin_capture_cycles(dev, pwm, flags, &period_cycles,
				     &pulse_cycles, timeout);
	if (err) {
		return err;
	}

	err = pwm_pin_cycles_to_usec(dev, pwm, period_cycles, period);
	if (err) {
		return err;
	}

	err = pwm_pin_cycles_to_usec(dev, pwm, pulse_cycles, pulse);
	if (err) {
		return err;
	}

	return 0;
}

/**
 * @brief Capture a single PWM period/pulse width in nanoseconds for a single
 *        PWM input.
 *
 * This API function wraps calls to @a pwm_pin_capture_cycles() and @a
 * pwm_pin_cycles_to_nsec() and passes the capture result to the caller. The
 * function is blocking until either the PWM capture is completed or a timeout
 * occurs.
 *
 * @note @kconfig{CONFIG_PWM_CAPTURE} must be selected for this function to be
 * available.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param pwm PWM pin.
 * @param flags PWM capture flags.
 * @param period Pointer to the memory to store the captured PWM period width
 *               (in nsec).
 * @param pulse Pointer to the memory to store the captured PWM pulse width (in
 *              nsec).
 * @param timeout Waiting period for the capture to complete.
 *
 * @retval 0 If successful.
 * @retval -EBUSY PWM capture already in progress.
 * @retval -EAGAIN Waiting period timed out.
 * @retval -EIO IO error while capturing.
 * @retval -ERANGE If result is too large.
 */
static inline int pwm_pin_capture_nsec(const struct device *dev, uint32_t pwm,
				       pwm_flags_t flags,
				       uint64_t *period,
				       uint64_t *pulse,
				       k_timeout_t timeout)
{
	uint32_t period_cycles;
	uint32_t pulse_cycles;
	int err;

	err = pwm_pin_capture_cycles(dev, pwm, flags, &period_cycles,
				     &pulse_cycles, timeout);
	if (err) {
		return err;
	}

	err = pwm_pin_cycles_to_nsec(dev, pwm, period_cycles, period);
	if (err) {
		return err;
	}

	err = pwm_pin_cycles_to_nsec(dev, pwm, pulse_cycles, pulse);
	if (err) {
		return err;
	}

	return 0;
}

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#include <syscalls/pwm.h>

#endif /* ZEPHYR_INCLUDE_DRIVERS_PWM_H_ */
