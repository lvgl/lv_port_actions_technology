/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <drivers/display/display_graphics.h>

/*
 * example:
 *
 * 1) declare a buffer whose address in SRAM
 * static uint16_t dst_buf_mem[360 * 360] __in_section_unique(ram.noinit.test);
 * static display_buffer_t buffer = {
 *	.desc = {
 *		.pixel_format = PIXEL_FORMAT_BGR_565,
 *		.pitch = 360 * 2,
 *		.width = 360,
 *		.height = 360,
 *	},
 *	.addr = (uint32_t)dst_buf_mem,
 *};
 *
 * 2) configure MMU LUT (round shape)
 * 	display_mmu_config_lut(dst_buf.desc.width, dst_buf.desc.height,
 *			dst_buf.desc.pixel_format, display_mmu_get_round_range);
 *
 * 3) map buffer to MMU slot 0
 *	display_mmu_map_buf(0, &dst_buf);
 *
 * 4) enable MMU
 *	display_mmu_set_enabled(true);
 */

/* Number of slots supported by display MMU */
#define NUM_DISPLAY_MMU_SLOTS 4

/**
 * @typedef display_mmu_get_range_t
 * @brief Callback API executed when computing x pixels range [x1, x2] at every given line y
 *
 */
typedef void (*display_mmu_get_range_t)(uint16_t * x1, uint16_t * x2, uint16_t y, uint16_t x_res, uint16_t y_res);

/**
 * @brief Control display MMU enabled or not
 *
 * @param en enable flag
 *
 * @retval 0 on success else negative errno code.
 */
int display_mmu_set_enabled(bool en);

/**
 * @brief Configure the Display MMU look-up table
 *
 * @param x_res horizontal resolution of frame buffer
 * @param y_res vertical resolution of frame buffer
 * @param pixel_format pixel format of frame buffer
 * @param range_cb callback to compute x pixels range [x1, x2] at every given line y
 *
 * @retval the physical size of frame buffer required on success else negative errno code.
 */
int display_mmu_config_lut(uint16_t x_res, uint16_t y_res, uint32_t pixel_format, display_mmu_get_range_t range_cb);

/**
 * @brief Map the frame buffer to a Display MMU slot
 *
 * On success, the addr, pitch and buf_size of buffer will be updated with the
 * mapping address and pitch by Display MMU.
 *
 * @param slot Y display MMU slot index
 * @param buffer pointer to structure display_buffer whose addr must be in the SRAM
 *               address range and will be replaced with virtual addr after mapping.
 *
 * @retval 0 on success else negative errno code.
 */
int display_mmu_map_buf(uint8_t slot, display_buffer_t *buffer);

/**
 * @brief Query buffer is mapped by DIisplay MMU or not
 *
 * @param buffer pointer to structure display_buffer
 *
 * @retval the query result.
 */
bool display_mmu_is_buf_mapped(const display_buffer_t *buffer);

/**
 * @brief Get the frame buffer descriptor after configuration.
 *
 * @retval the frame buffer descriptor.
 */
const struct display_buffer_descriptor * display_mmu_get_buf_desc(void);

/**
 * @brief Callback for round shape range
 *
 * @param x1 address to store min x coord of pixel at line y
 * @param x1 address to store max x coord of pixel at line y
 * @param y y coord of pixel
 * @param x_res x resolution
 * @param y_res y resolution
 *
 * @retval N/A.
 */
void display_mmu_get_round_range(uint16_t * x1, uint16_t * x2, uint16_t y, uint16_t x_res, uint16_t y_res);

/**
 * @brief Callback for rectangle shape range
 *
 * @param x1 address to store min x coord of pixel at line y
 * @param x1 address to store max x coord of pixel at line y
 * @param y y coord of pixel
 * @param x_res x resolution
 * @param y_res y resolution
 *
 * @retval N/A.
 */
void display_mmu_get_rectangle_range(uint16_t * x1, uint16_t * x2, uint16_t y, uint16_t x_res, uint16_t y_res);
