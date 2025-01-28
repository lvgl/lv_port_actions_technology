/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief display surface interface
 */

#ifndef ZEPHYR_FRAMEWORK_INCLUDE_DISPLAY_UI_SURFACE_H_
#define ZEPHYR_FRAMEWORK_INCLUDE_DISPLAY_UI_SURFACE_H_

#include <os_common_api.h>
#include <display/graphic_buffer.h>

/**
 * @defgroup ui_surface_apis UI Surface APIs
 * @ingroup system_apis
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CONFIG_SURFACE_DOUBLE_BUFFER
#  define CONFIG_SURFACE_MAX_BUFFER_COUNT (2)
#elif defined(CONFIG_SURFACE_SINGLE_BUFFER)
#  define CONFIG_SURFACE_MAX_BUFFER_COUNT (1)
#else
#  define CONFIG_SURFACE_MAX_BUFFER_COUNT (0)
#endif

/**
 * @enum surface_event_id
 * @brief Enumeration with possible surface event
 *
 */
enum surface_event_id {
	/* ready to accept next draw */
	SURFACE_EVT_DRAW_READY = 0,
	/* check if current frame drawn dirty regions fully covers an area,
	 * required to optimize the swapbuf copy in double buffer case.
	 */
	SURFACE_EVT_DRAW_COVER_CHECK,

	SURFACE_EVT_POST_START,
};

/**
 * @enum surface_callback_id
 * @brief Enumeration with possible surface callback
 *
 */
enum surface_callback_id {
	SURFACE_CB_DRAW = 0,
	SURFACE_CB_POST,

	SURFACE_NUM_CBS,
};

/**
 * @brief Enumeration with possible surface flags
 *
 */
enum {
	/* both for orientation and update flags */
	SURFACE_ROTATED_90   = 0x01,
	SURFACE_ROTATED_180  = 0x02,
	SURFACE_ROTATED_270  = 0x03,
	SURFACE_ROTATED_MASK = 0x03,

	/* draw/update flags */
	SURFACE_FIRST_DRAW = 0x10, /* first draw in one frame */
	SURFACE_LAST_DRAW  = 0x20, /* last draw in one frame */

	/* post flags */
	SURFACE_POST_IN_SYNC_MODE = 0x40, /* surface post in sync mode. */
};

/**
 * @struct surface_post_data
 * @brief Structure holding surface event data in callback SURFACE_CB_POST
 *
 */
typedef struct surface_post_data {
	uint8_t flags;
	const ui_region_t *area;
} surface_post_data_t;

/**
 * @struct surface_cover_check_data
 * @brief Structure holding surface event data in callback SURFACE_EVT_DRAW_COVER_CHECK
 *
 */
typedef struct surface_cover_check_data {
	const ui_region_t *area;
	bool covered; /* store the cover result with 'area' */
} surface_cover_check_data_t;

/**
 * @typedef surface_callback_t
 * @brief Callback API executed when surface changed
 *
 */
typedef void (*surface_callback_t)(uint32_t event, void *data, void *user_data);

/**
 * @struct surface
 * @brief Structure holding surface
 *
 */
typedef struct surface {
	uint16_t width;
	uint16_t height;
	uint32_t pixel_format;

#if CONFIG_SURFACE_MAX_BUFFER_COUNT > 0
	ui_region_t dirty_area;

	graphic_buffer_t *buffers[CONFIG_SURFACE_MAX_BUFFER_COUNT];
	uint8_t buf_count;
	uint8_t draw_idx;
	uint8_t post_idx;

	uint8_t swapping : 1;
#else
	graphic_buffer_t *buffers;
	uint8_t buf_count;
	uint8_t draw_idx;
#endif /* CONFIG_SURFACE_MAX_BUFFER_COUNT > 0 */

	uint8_t in_frame;
	os_sem frame_sem;

	/* passed in surface_create() */
	uint8_t create_flags;
	/* passed in surface_begin_draw() */
	uint8_t draw_flags;

	atomic_t post_cnt;
	os_sem post_sem;

	surface_callback_t callback[SURFACE_NUM_CBS];
	void *user_data[SURFACE_NUM_CBS];

	/* reference count (considering alloc/free and post pending count) */
	atomic_t refcount;
} surface_t;

/**
 * @brief Create surface buffer
 *
 * @param w width in pixels
 * @param h height in pixels
 * @param pixel_format display pixel format, see enum display_pixel_format
 * @param buffer usage
 *
 * @return pointer to graphic buffer structure; NULL if failed.
 */
graphic_buffer_t *surface_buffer_create(uint16_t w, uint16_t h,
		uint32_t pixel_format, uint32_t usage);

/**
 * @brief Create surface
 *
 * Must be called in the draw thread context.
 *
 * @param w width in pixels
 * @param h height in pixels
 * @param pixel_format display pixel format, see enum display_pixel_format
 * @param buf_count number of buffers to allocate
 * @param flags surface create flags
 *
 * @return pointer to surface structure; NULL if failed.
 */
surface_t *surface_create(uint16_t w, uint16_t h,
		uint32_t pixel_format, uint8_t buf_count, uint8_t flags);

/**
 * @brief Destroy surface
 *
 * Must be called in the draw thread context.
 *
 * @param surface pointer to surface structure
 *
 * @retval N/A.
 */
void surface_destroy(surface_t *surface);

/**
 * @brief Register surface callback
 *
 * Must be called in the draw thread context.
 *
 * @param surface pointer to surface structure
 * @param callback event callback
 * @param user_data user data passed in callback function
 *
 * @return N/A.
 */
void surface_register_callback(surface_t *surface,
		int callback_id, surface_callback_t callback_fn, void *user_data);

/**
 * @brief Set continuous draw count without waiting next draw ready.
 *
 * This is just a hint to surface.
 * Must call before the first draw.
 *
 * Must be called in the draw thread context.
 *
 * @param surface pointer to surface structure
 * @param count continuous draw count
 *
 * @retval 0 on success else negative error code.
 */
void surface_set_continuous_draw_count(surface_t *surface, uint8_t count);

/**
 * @brief surface begin a new frame to draw.
 *
 * Must call when current frame dirty regions are computed, and before
 * any surface_begin_draw().
 *
 * Must be called in the draw thread context.
 *
 * @param surface pointer to surface structure
 *
 * @return 0 on success else negetive errno code.
 */
int surface_begin_frame(surface_t *surface);

/**
 * @brief surface end current frame to draw
 *
 * @param surface pointer to surface structure
 *
 * @return 0 on success else negetive errno code.
 */
int surface_end_frame(surface_t *surface);

/**
 * @brief surface begin to draw the current frame
 *
 * Must be called in the draw thread context.
 *
 * @param surface pointer to surface structure
 * @param flags draw flags
 * @param drawbuf store the pointer of draw buffer.
 *
 * @retval 0 on success else negative error code.
 */
int surface_begin_draw(surface_t *surface, uint8_t flags, graphic_buffer_t **drawbuf);

/**
 * @brief surface end to draw the current frame
 *
 * Must be called in the draw thread context.
 *
 * @param surface pointer to surface structure
 * @param area updated surface area
 *
 * @retval 0 on success else negative error code.
 */
int surface_end_draw(surface_t *surface, const ui_region_t *area);

/**
 * @brief surface update with the external buffer
 *
 * @param surface pointer to surface structure
 * @param flags draw flags, can contain SURFACE_ROTATED_? if enabled.
 * @param area updated surface area after rotation if set
 * @param buf pointer to the data buffer before rotation if set
 * @param stride stride in pixels of the data buffer
 * @param pixel_format pixel format of the data buffer
 *
 * @retval 0 on success else negative error code.
 */
int surface_update(surface_t *surface, uint8_t flags,
				   const ui_region_t *area, const void *buf,
				   uint16_t stride, uint32_t pixel_format);

/**
 * @brief wait the surface update content completed
 *
 * Wait the previous surface_update() completed
 * Must be called in the draw thread context.
 *
 * @param surface pointer to surface structure
 * @parem timeout wait timeout in milliseconds
 *
 * @retval 0 on success else negative error code.
 */
int surface_wait_for_update(surface_t *surface, int timeout);

/**
 * @brief wait the surface refresh to display completed
 *
 * Must be called in the draw thread context.
 *
 * @param surface pointer to surface structure
 * @parem timeout wait timeout in milliseconds
 *
 * @retval 0 on success else negative error code.
 */
int surface_wait_for_refresh(surface_t *surface, int timeout);

/**
 * @brief notify surface that one post has completed
 *
 * This must be called once corresponding to every SURFACE_EVT_POST_START.
 *
 * @param surface pointer to surface structure
 *
 * @return N/A.
 */
void surface_complete_one_post(surface_t *surface);

/**
 * @brief Get pixel format of surface
 *
 * @param surface pointer to surface structure
 *
 * @return pixel format.
 */
static inline uint32_t surface_get_pixel_format(surface_t *surface)
{
	return surface->pixel_format;
}

/**
 * @brief Get width of surface
 *
 * @param surface pointer to surface structure
 *
 * @return width in pixels.
 */
static inline uint16_t surface_get_width(surface_t *surface)
{
	return surface->width;
}

/**
 * @brief Get height of surface
 *
 * @param surface pointer to surface structure
 *
 * @return height in pixels.
 */
static inline uint16_t surface_get_height(surface_t *surface)
{
	return surface->height;
}

/**
 * @brief Get stride of surface
 *
 * @param surface pointer to surface structure
 *
 * @return stride in pixels.
 */
static inline uint32_t surface_get_stride(surface_t *surface)
{
#if CONFIG_SURFACE_MAX_BUFFER_COUNT > 0
	return surface->buffers[0] ? graphic_buffer_get_stride(surface->buffers[0]) : surface->width;
#else
	return surface->width;
#endif
}

/**
 * @brief Get orientation of surface
 *
 * @param surface pointer to surface structure
 *
 * @return rotation (CW) in degrees
 */
static inline uint16_t surface_get_orientation(surface_t *surface)
{
	return ((surface->create_flags & SURFACE_ROTATED_90) ? 90 : 0) +
			((surface->create_flags & SURFACE_ROTATED_180) ? 180 : 0);
}

/**
 * @brief Get allocated buffer count of surface
 *
 * @param surface pointer to surface structure
 *
 * @return buffer count of surface.
 */
uint8_t surface_get_buffer_count(surface_t *surface);

/**
 * @brief Get maximum supported surface buffer count
 *
 * @return buffer count.
 */
uint8_t surface_get_max_possible_buffer_count(void);

/**
* @cond INTERNAL_HIDDEN
*/

/**
 * @brief Set minumum buffer count of surface
 *
 * It will check current buffer count, if it is less than min_count,
 * buffers will be allocated.
 *
 * Must be called in the draw thread context.
 *
 * @param surface pointer to surface structure
 * @param min_count min buffer count
 *
 * @retval 0 on success else negative code.
 */
int surface_set_min_buffer_count(surface_t *surface, uint8_t min_count);

/**
 * @brief Set maximum buffer count of surface
 *
 * It will check current buffer count, if it is less than min_count,
 * buffers will be allocated.
 *
 * Must be called in the draw thread context.
 *
 * @param surface pointer to surface structure
 * @param max_count max buffer count
 *
 * @retval 0 on success else negative code.
 */
int surface_set_max_buffer_count(surface_t *surface, uint8_t max_count);

/**
 * @brief Set current buffer count of surface
 *
 * Set buffer count, it will change the min/max buffer count if required.
 *
 * Must be called in the draw thread context.
 *
 * @param surface pointer to surface structure
 * @param buf_count buffer count
 *
 * @retval 0 on success else negative code.
 */
int surface_set_buffer_count(surface_t *surface, uint8_t buf_count);

/**
 * @brief Get draw buffer of surface
 *
 * @param surface pointer to surface structure
 *
 * @return pointer of graphic buffer structure; NULL if failed.
 */
graphic_buffer_t *surface_get_draw_buffer(surface_t *surface);

/**
 * @brief Get post buffer of surface
 *
 * @param surface pointer to surface structure
 *
 * @return pointer of graphic buffer structure; NULL if failed.
 */
graphic_buffer_t *surface_get_post_buffer(surface_t *surface);

/**
* INTERNAL_HIDDEN @endcond
*/

#ifdef __cplusplus
}
#endif
/**
 * @}
 */

#endif /* ZEPHYR_FRAMEWORK_INCLUDE_DISPLAY_UI_SURFACE_H_ */
