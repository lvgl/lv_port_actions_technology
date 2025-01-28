/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief API for input recorder
 */

#ifndef FRAMEWORK_DISPLAY_INCLUDE_INPUT_RECORDER_H
#define FRAMEWORK_DISPLAY_INCLUDE_INPUT_RECORDER_H

#include <stdint.h>
#include <stdbool.h>
#include <input_manager.h>
#include <stream.h>

/**
 * @brief Input Recorder
 * @defgroup input_recorder app Input Recorder
 * @ingroup system_apis
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

#define INPUTREC_DEFAULT_REPEAT_DELAY (500) /* in milliseconds */

/* Input Record Data Structure */
typedef struct input_rec_data {
	input_dev_data_t data;
	uint32_t timestamp;
} input_rec_data_t;

/*
 * Read the input data.
 * Must return 0 on success else negative code.
 */
typedef int (*input_data_read_t)(input_rec_data_t *data, void *user_data);

/*
 * Write the input data.
 * Must return 0 on success else negative code.
 */
typedef int (*input_data_write_t)(const input_rec_data_t *data, void *user_data);

/*
 * Rewind the input data stream to implement repeat function.
 */
typedef void (*input_data_rewind_t)(void *user_data);

/**
 * @brief Start the input event capture.
 *
 * The capture will really started until the press event.
 *
 * @param write_fn callback to write input data.
 * @param user_data user data passed to callback function.
 *
 * @retval 0 on success else negative code.
 */
int input_capture_start(input_data_write_t write_fn, void *user_data);

/**
 * @brief Stop the input event capture.
 *
 * @retval number of records successfully capture on success else negative code.
 */
int input_capture_stop(void);

/**
 * @brief Query the input event capture is running or not.
 *
 * @retval the query result.
 */
bool input_capture_is_running(void);

/**
 * @brief Write the input data.
 *
 * @retval 0 on success else negative code.
 */
int input_capture_write(const input_dev_data_t * data);

/**
 * @brief Start the input event playback.
 *
 * @param read_fn callback to read input data.
 * @param rewind_fn callback to rewind input data stream to repeat the events. If NULL,
 *               the repeat will be disabled
 * @param repeat_delay delay in milliseconds before repeat,
 * @param user_data user data passed to callback function.
 *
 * @retval 0 on success else negative code.
 */
int input_playback_start(input_data_read_t read_fn, input_data_rewind_t rewind_fn,
		uint16_t repeat_delay, void *user_data);

/**
 * @brief Stop the input event playback.
 *
 * @retval number of records successfully play on success else negative code.
 */
int input_playback_stop(void);

/**
 * @brief Query the input event playback is running or not.
 *
 * @retval the query result.
 */
bool input_playback_is_running(void);

/**
 * @brief Read the input data.
 *
 * @retval 0 on success else negative code.
 */
int input_playback_read(input_dev_data_t * data);

/**
 * @brief Start the input event capture to stream.
 *
 * @param stream stream to write input data.
 *
 * @retval 0 on success else negative code.
 */
int input_capture_stream_start(io_stream_t stream);

/**
 * @brief Start the input event playback from stream.
 *
 * @param stream stream to read input data.
 * @param repeat repeat the event or not
 *
 * @retval 0 on success else negative code.
 */
int input_playback_stream_start(io_stream_t stream, bool repeat);

/**
 * @brief Start the input event capture to buffer.
 *
 * @param buffer buffer address to write input data.
 * @param buffer buffer size.
 *
 * @retval 0 on success else negative code.
 */
int input_capture_buffer_start(void *buffer, uint32_t size);

/**
 * @brief Start the input event playback from buffer.
 *
 * @param buffer buffer address to write input data.
 * @param buffer buffer size.
 * @param repeat repeat the event or not
 *
 * @retval 0 on success else negative code.
 */
int input_playback_buffer_start(const void *buffer, uint32_t size, bool repeat);

/**
 * @brief Start the input event playback with fixedstep sliding forth and back.
 *
 * @param start start position.
 * @param stop stop position.
 * @param step sliding step.
 * @param is_vert vertical sliding or not
 *
 * @retval 0 on success else negative code.
 */
int input_playback_slide_fixstep_start(int16_t start, int16_t stop, int16_t step, bool is_vert);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* FRAMEWORK_DISPLAY_INCLUDE_INPUT_RECORDER_H */
