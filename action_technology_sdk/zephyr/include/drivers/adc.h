/**
 * @file
 * @brief ADC public API header file.
 */

/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_ADC_H_
#define ZEPHYR_INCLUDE_DRIVERS_ADC_H_

#include <device.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief ADC driver APIs
 * @defgroup adc_interface ADC driver APIs
 * @ingroup io_interfaces
 * @{
 */

/** @brief ADC channel gain factors. */
enum adc_gain {
	ADC_GAIN_1_6, /**< x 1/6. */
	ADC_GAIN_1_5, /**< x 1/5. */
	ADC_GAIN_1_4, /**< x 1/4. */
	ADC_GAIN_1_3, /**< x 1/3. */
	ADC_GAIN_1_2, /**< x 1/2. */
	ADC_GAIN_2_3, /**< x 2/3. */
	ADC_GAIN_1,   /**< x 1. */
	ADC_GAIN_2,   /**< x 2. */
	ADC_GAIN_3,   /**< x 3. */
	ADC_GAIN_4,   /**< x 4. */
	ADC_GAIN_6,   /**< x 6. */
	ADC_GAIN_8,   /**< x 8. */
	ADC_GAIN_12,  /**< x 12. */
	ADC_GAIN_16,  /**< x 16. */
	ADC_GAIN_24,  /**< x 24. */
	ADC_GAIN_32,  /**< x 32. */
	ADC_GAIN_64,  /**< x 64. */
	ADC_GAIN_128, /**< x 128. */
};

/**
 * @brief Invert the application of gain to a measurement value.
 *
 * For example, if the gain passed in is ADC_GAIN_1_6 and the
 * referenced value is 10, the value after the function returns is 60.
 *
 * @param gain the gain used to amplify the input signal.
 *
 * @param value a pointer to a value that initially has the effect of
 * the applied gain but has that effect removed when this function
 * successfully returns.  If the gain cannot be reversed the value
 * remains unchanged.
 *
 * @retval 0 if the gain was successfully reversed
 * @retval -EINVAL if the gain could not be interpreted
 */
int adc_gain_invert(enum adc_gain gain,
		    int32_t *value);

/** @brief ADC references. */
enum adc_reference {
	ADC_REF_VDD_1,     /**< VDD. */
	ADC_REF_VDD_1_2,   /**< VDD/2. */
	ADC_REF_VDD_1_3,   /**< VDD/3. */
	ADC_REF_VDD_1_4,   /**< VDD/4. */
	ADC_REF_INTERNAL,  /**< Internal. */
	ADC_REF_EXTERNAL0, /**< External, input 0. */
	ADC_REF_EXTERNAL1, /**< External, input 1. */
};

/**
 * @brief Structure for specifying the configuration of an ADC channel.
 */
struct adc_channel_cfg {
	/** Gain selection. */
	enum adc_gain gain;

	/** Reference selection. */
	enum adc_reference reference;

	/**
	 * Acquisition time.
	 * Use the ADC_ACQ_TIME macro to compose the value for this field or
	 * pass ADC_ACQ_TIME_DEFAULT to use the default setting for a given
	 * hardware (e.g. when the hardware does not allow to configure the
	 * acquisition time).
	 * Particular drivers do not necessarily support all the possible units.
	 * Value range is 0-16383 for a given unit.
	 */
	uint16_t acquisition_time;

	/**
	 * Channel identifier.
	 * This value primarily identifies the channel within the ADC API - when
	 * a read request is done, the corresponding bit in the "channels" field
	 * of the "adc_sequence" structure must be set to include this channel
	 * in the sampling.
	 * For hardware that does not allow selection of analog inputs for given
	 * channels, but rather have dedicated ones, this value also selects the
	 * physical ADC input to be used in the sampling. Otherwise, when it is
	 * needed to explicitly select an analog input for the channel, or two
	 * inputs when the channel is a differential one, the selection is done
	 * in "input_positive" and "input_negative" fields.
	 * Particular drivers indicate which one of the above two cases they
	 * support by selecting or not a special hidden Kconfig option named
	 * ADC_CONFIGURABLE_INPUTS. If this option is not selected, the macro
	 * CONFIG_ADC_CONFIGURABLE_INPUTS is not defined and consequently the
	 * mentioned two fields are not present in this structure.
	 * While this API allows identifiers from range 0-31, particular drivers
	 * may support only a limited number of channel identifiers (dependent
	 * on the underlying hardware capabilities or configured via a dedicated
	 * Kconfig option).
	 */
	uint8_t channel_id   : 5;

	/** Channel type: single-ended or differential. */
	uint8_t differential : 1;

#ifdef CONFIG_ADC_CONFIGURABLE_INPUTS
	/**
	 * Positive ADC input.
	 * This is a driver dependent value that identifies an ADC input to be
	 * associated with the channel.
	 */
	uint8_t input_positive;

	/**
	 * Negative ADC input (used only for differential channels).
	 * This is a driver dependent value that identifies an ADC input to be
	 * associated with the channel.
	 */
	uint8_t input_negative;
#endif /* CONFIG_ADC_CONFIGURABLE_INPUTS */
};

/**
 * @brief Convert a raw ADC value to millivolts.
 *
 * This function performs the necessary conversion to transform a raw
 * ADC measurement to a voltage in millivolts.
 *
 * @param ref_mv the reference voltage used for the measurement, in
 * millivolts.  This may be from adc_ref_internal() or a known
 * external reference.
 *
 * @param gain the ADC gain configuration used to sample the input
 *
 * @param resolution the number of bits in the absolute value of the
 * sample.  For differential sampling this may be one less than the
 * resolution in struct adc_sequence.
 *
 * @param valp pointer to the raw measurement value on input, and the
 * corresponding millivolt value on successful conversion.  If
 * conversion fails the stored value is left unchanged.
 *
 * @retval 0 on successful conversion
 * @retval -EINVAL if the gain is not reversible
 */
static inline int adc_raw_to_millivolts(int32_t ref_mv,
					enum adc_gain gain,
					uint8_t resolution,
					int32_t *valp)
{
	int32_t adc_mv = *valp * ref_mv;
	int ret = adc_gain_invert(gain, &adc_mv);

	if (ret == 0) {
		*valp = (adc_mv >> resolution);
	}

	return ret;
}

/* Forward declaration of the adc_sequence structure. */
struct adc_sequence;

/**
 * @brief Action to be performed after a sampling is done.
 */
enum adc_action {
	/** The sequence should be continued normally. */
	ADC_ACTION_CONTINUE = 0,

	/**
	 * The sampling should be repeated. New samples or sample should be
	 * read from the ADC and written in the same place as the recent ones.
	 */
	ADC_ACTION_REPEAT,

	/** The sequence should be finished immediately. */
	ADC_ACTION_FINISH,
};

/**
 * @brief Type definition of the optional callback function to be called after
 *        a requested sampling is done.
 *
 * @param dev             Pointer to the device structure for the driver
 *                        instance.
 * @param sequence        Pointer to the sequence structure that triggered
 *                        the sampling. This parameter points to a copy of
 *                        the structure that was supplied to the call that
 *                        started the sampling sequence, thus it cannot be
 *                        used with the CONTAINER_OF() macro to retrieve
 *                        some other data associated with the sequence.
 *                        Instead, the adc_sequence_options::user_data field
 *                        should be used for such purpose.
 *
 * @param sampling_index  Index (0-65535) of the sampling done.
 *
 * @returns Action to be performed by the driver. See @ref adc_action.
 */
typedef enum adc_action (*adc_sequence_callback)(const struct device *dev,
						 const struct adc_sequence *sequence,
						 uint16_t sampling_index);

/**
 * @brief Structure defining additional options for an ADC sampling sequence.
 */
struct adc_sequence_options {
	/**
	 * Interval between consecutive samplings (in microseconds), 0 means
	 * sample as fast as possible, without involving any timer.
	 * The accuracy of this interval is dependent on the implementation of
	 * a given driver. The default routine that handles the intervals uses
	 * a kernel timer for this purpose, thus, it has the accuracy of the
	 * kernel's system clock. Particular drivers may use some dedicated
	 * hardware timers and achieve a better precision.
	 */
	uint32_t interval_us;

	/**
	 * Callback function to be called after each sampling is done.
	 * Optional - set to NULL if it is not needed.
	 */
	adc_sequence_callback callback;

	/**
	 * Pointer to user data. It can be used to associate the sequence
	 * with any other data that is needed in the callback function.
	 */
	void *user_data;

	/**
	 * Number of extra samplings to perform (the total number of samplings
	 * is 1 + extra_samplings).
	 */
	uint16_t extra_samplings;
};

/**
 * @brief Structure defining an ADC sampling sequence.
 */
struct adc_sequence {
	/**
	 * Pointer to a structure defining additional options for the sequence.
	 * If NULL, the sequence consists of a single sampling.
	 */
	const struct adc_sequence_options *options;

	/**
	 * Bit-mask indicating the channels to be included in each sampling
	 * of this sequence.
	 * All selected channels must be configured with adc_channel_setup()
	 * before they are used in a sequence.
	 */
	uint32_t channels;

	/**
	 * Pointer to a buffer where the samples are to be written. Samples
	 * from subsequent samplings are written sequentially in the buffer.
	 * The number of samples written for each sampling is determined by
	 * the number of channels selected in the "channels" field.
	 * The buffer must be of an appropriate size, taking into account
	 * the number of selected channels and the ADC resolution used,
	 * as well as the number of samplings contained in the sequence.
	 */
	void *buffer;

	/**
	 * Specifies the actual size of the buffer pointed by the "buffer"
	 * field (in bytes). The driver must ensure that samples are not
	 * written beyond the limit and it must return an error if the buffer
	 * turns out to be not large enough to hold all the requested samples.
	 */
	size_t buffer_size;

	/**
	 * ADC resolution.
	 * For single-ended channels the sample values are from range:
	 *   0 .. 2^resolution - 1,
	 * for differential ones:
	 *   - 2^(resolution-1) .. 2^(resolution-1) - 1.
	 */
	uint8_t resolution;

	/**
	 * Oversampling setting.
	 * Each sample is averaged from 2^oversampling conversion results.
	 * This feature may be unsupported by a given ADC hardware, or in
	 * a specific mode (e.g. when sampling multiple channels).
	 */
	uint8_t oversampling;

	/**
	 * Perform calibration before the reading is taken if requested.
	 *
	 * The impact of channel configuration on the calibration
	 * process is specific to the underlying hardware.  ADC
	 * implementations that do not support calibration should
	 * ignore this flag.
	 */
	bool calibrate;
};


/**
 * @brief Type definition of ADC API function for configuring a channel.
 * See adc_channel_setup() for argument descriptions.
 */
typedef int (*adc_api_channel_setup)(const struct device *dev,
				     const struct adc_channel_cfg *channel_cfg);

/**
 * @brief Type definition of ADC API function for setting a read request.
 * See adc_read() for argument descriptions.
 */
typedef int (*adc_api_read)(const struct device *dev,
			    const struct adc_sequence *sequence);

/**
 * @brief Type definition of ADC API function for setting an asynchronous
 *        read request.
 * See adc_read_async() for argument descriptions.
 */
typedef int (*adc_api_read_async)(const struct device *dev,
				  const struct adc_sequence *sequence,
				  struct k_poll_signal *async);

/**
 * @brief ADC driver API
 *
 * This is the mandatory API any ADC driver needs to expose.
 */
__subsystem struct adc_driver_api {
	adc_api_channel_setup channel_setup;
	adc_api_read          read;
#ifdef CONFIG_ADC_ASYNC
	adc_api_read_async    read_async;
#endif
	uint16_t ref_internal;	/* mV */
};

/**
 * @brief Configure an ADC channel.
 *
 * It is required to call this function and configure each channel before it is
 * selected for a read request.
 *
 * @param dev          Pointer to the device structure for the driver instance.
 * @param channel_cfg  Channel configuration.
 *
 * @retval 0       On success.
 * @retval -EINVAL If a parameter with an invalid value has been provided.
 */
__syscall int adc_channel_setup(const struct device *dev,
				const struct adc_channel_cfg *channel_cfg);

static inline int z_impl_adc_channel_setup(const struct device *dev,
					   const struct adc_channel_cfg *channel_cfg)
{
	const struct adc_driver_api *api =
				(const struct adc_driver_api *)dev->api;

	return api->channel_setup(dev, channel_cfg);
}

/**
 * @brief Set a read request.
 *
 * @param dev       Pointer to the device structure for the driver instance.
 * @param sequence  Structure specifying requested sequence of samplings.
 *
 * If invoked from user mode, any sequence struct options for callback must
 * be NULL.
 *
 * @retval 0        On success.
 * @retval -EINVAL  If a parameter with an invalid value has been provided.
 * @retval -ENOMEM  If the provided buffer is to small to hold the results
 *                  of all requested samplings.
 * @retval -ENOTSUP If the requested mode of operation is not supported.
 * @retval -EBUSY   If another sampling was triggered while the previous one
 *                  was still in progress. This may occur only when samplings
 *                  are done with intervals, and it indicates that the selected
 *                  interval was too small. All requested samples are written
 *                  in the buffer, but at least some of them were taken with
 *                  an extra delay compared to what was scheduled.
 */
__syscall int adc_read(const struct device *dev,
		       const struct adc_sequence *sequence);

static inline int z_impl_adc_read(const struct device *dev,
				  const struct adc_sequence *sequence)
{
	const struct adc_driver_api *api =
				(const struct adc_driver_api *)dev->api;

	return api->read(dev, sequence);
}

/**
 * @brief Set an asynchronous read request.
 *
 * @note This function is available only if @kconfig{CONFIG_ADC_ASYNC}
 * is selected.
 *
 * If invoked from user mode, any sequence struct options for callback must
 * be NULL.
 *
 * @param dev       Pointer to the device structure for the driver instance.
 * @param sequence  Structure specifying requested sequence of samplings.
 * @param async     Pointer to a valid and ready to be signaled struct
 *                  k_poll_signal. (Note: if NULL this function will not notify
 *                  the end of the transaction, and whether it went successfully
 *                  or not).
 *
 * @returns 0 on success, negative error code otherwise.
 *          See adc_read() for a list of possible error codes.
 *
 */
__syscall int adc_read_async(const struct device *dev,
			     const struct adc_sequence *sequence,
			     struct k_poll_signal *async);


#ifdef CONFIG_ADC_ASYNC
static inline int z_impl_adc_read_async(const struct device *dev,
					const struct adc_sequence *sequence,
					struct k_poll_signal *async)
{
	const struct adc_driver_api *api =
				(const struct adc_driver_api *)dev->api;

	return api->read_async(dev, sequence, async);
}
#endif /* CONFIG_ADC_ASYNC */

/**
 * @brief Get the internal reference voltage.
 *
 * Returns the voltage corresponding to @ref ADC_REF_INTERNAL,
 * measured in millivolts.
 *
 * @return a positive value is the reference voltage value.  Returns
 * zero if reference voltage information is not available.
 */
static inline uint16_t adc_ref_internal(const struct device *dev)
{
	const struct adc_driver_api *api =
				(const struct adc_driver_api *)dev->api;

	return api->ref_internal;
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#include <syscalls/adc.h>

#endif  /* ZEPHYR_INCLUDE_DRIVERS_ADC_H_ */
