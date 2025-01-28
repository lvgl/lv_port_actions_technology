/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file view manager interface
 */

#ifndef FRAMEWORK_DISPLAY_INCLUDE_UI_REGION_H_
#define FRAMEWORK_DISPLAY_INCLUDE_UI_REGION_H_

/**
 * @defgroup ui_manager_apis app ui Manager APIs
 * @ingroup system_apis
 * @{
 */

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <ui_math.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @struct ui_point
 * @brief Structure holding point
 *
 */
typedef struct ui_point {
	int16_t x;
	int16_t y;
} ui_point_t;

/**
 * @struct ui_region
 * @brief Structure holding region [x1,x2] x [y1,y2]
 *
 */
typedef struct ui_region {
	int16_t x1;
	int16_t y1;
	int16_t x2;
	int16_t y2;
} ui_region_t;

/**
 * @brief Initialize a point
 *
 * @param point pointer to a point
 * @param x new x coordinate
 * @param y new y coordinate
 *
 * @retval N/A
 */
static inline void ui_point_set(ui_point_t * point, int16_t x, int16_t y)
{
	point->x = x;
	point->y = y;
}

/**
 * @brief Move a point
 *
 * @param point pointer to a point
 * @param dx delta X to move
 * @param dy delta Y to move
 *
 * @retval N/A
 */
static inline void ui_point_move(ui_point_t * point, int16_t dx, int16_t dy)
{
	point->x += dx;
	point->y += dy;
}

/**
 * @brief Initialize an region
 *
 * @param region pointer to an region
 * @param x1 left coordinate of the region
 * @param y1 top coordinate of the region
 * @param x2 right coordinate of the region
 * @param y2 bottom coordinate of the region
 *
 * @retval N/A
 */
static inline void ui_region_set(ui_region_t * region,
		int16_t x1, int16_t y1, int16_t x2, int16_t y2)
{
	region->x1 = x1;
	region->y1 = y1;
	region->x2 = x2;
	region->y2 = y2;
}

/**
 * @brief Copy an region
 *
 * @param dest pointer to the destination region
 * @param src pointer to the source region
 *
 * @retval N/A
 */
static inline void ui_region_copy(ui_region_t * dest, const ui_region_t * src)
{
	dest->x1 = src->x1;
	dest->y1 = src->y1;
	dest->x2 = src->x2;
	dest->y2 = src->y2;
}

/**
 * @brief Get the width of an region
 *
 * @param region pointer to an region
 *
 * @return the width of the region (if x1 == x2 -> width = 1)
 */
static inline int16_t ui_region_get_width(const ui_region_t * region)
{
	return (region->x2 - region->x1 + 1);
}

/**
 * @brief Get the height of an region
 *
 * @param region pointer to an region
 *
 * @return the height of the region (if y1 == y2 -> height = 1)
 */
static inline int16_t ui_region_get_height(const ui_region_t * region)
{
	return (region->y2 - region->y1 + 1);
}

/**
 * @brief Get the size of an region
 *
 * @param region pointer to an region
 *
 * @retval size of the region
 */
int32_t ui_region_get_size(const ui_region_t * region);

/**
 * @brief Set the x coordinate of an region
 *
 * @param region pointer to an region
 * @param x new x coordinate
 *
 * @retval N/A
 */
void ui_region_set_x(ui_region_t * region, int16_t x);

/**
 * @brief Set the y coordinate of an region
 *
 * @param region pointer to an region
 * @param y new y coordinate
 *
 * @retval N/A
 */
void ui_region_set_y(ui_region_t * region, int16_t y);

/**
 * @brief Set the position of an region (width and height will be kept)
 *
 * @param region pointer to an region
 * @param x new x coordinate
 * @param y new y coordinate
 *
 * @retval N/A
 */
void ui_region_set_pos(ui_region_t * region, int16_t x, int16_t y);

/**
 * @brief Set the width of an region
 *
 * @param region pointer to an region
 * @param w the new width of region (w == 1 makes x1 == x2)
 *
 * @retval N/A
 */
void ui_region_set_width(ui_region_t * region, int16_t w);

/**
 * @brief Set the height of an region
 *
 * @param region pointer to an region
 * @param w the new height of region (h == 1 makes y1 == y2)
 *
 * @retval N/A
 */
void ui_region_set_height(ui_region_t * region, int16_t h);

/**
 * @brief Move an region
 *
 * @param region pointer to an region
 * @param dx delta X to move
 * @param dy delta Y to move
 *
 * @retval N/A
 */
void ui_region_move(ui_region_t * region, int16_t dx, int16_t dy);

/**
 * @brief Get the common parts of two regions
 *
 * @param result pointer to an region, the result will be stored here
 * @param region1 pointer to the first region
 * @param region2 pointer to the second region
 *
 * @return false: the two area has NO common parts, res_p is invalid
 */
bool ui_region_intersect(ui_region_t * result, const ui_region_t * region1, const ui_region_t * region2);

/**
 * @brief merge two regions into a third which involves the other two
 *
 * @param result pointer to an region, the result will be stored here
 * @param region1 pointer to the first region
 * @param region2 pointer to the second region
 *
 * @retval N/A
 */
void ui_region_merge(ui_region_t * result, const ui_region_t * region1, const ui_region_t * region2);

/**
 * @brief Subtract the common parts of two regions
 *
 * @param result pointer to a region array (4 at maximum), the result will be stored here
 * @param region pointer to a region
 * @param exclude pointer to the region to be subtracted from 'region'
 *
 * @retval the number of result regions
 */
int ui_region_subtract(ui_region_t * result, const ui_region_t * region, const ui_region_t * exclude);

/**
 * @brief Try smallest adjustment of region position to make it totally fall in the holder region
 *
 * @param region pointer to an region which should fall in 'holder'
 * @param holder pointer to an region which involves 'region'
 *
 * @retval N/A
 */
void ui_region_fit_in(ui_region_t  *region, const ui_region_t * holder);

/**
 * @brief Check if a point is on an region
 *
 * @param region pointer to an region
 * @param point pointer to a point
 *
 * @return false: the point is out of the area
 */
bool ui_region_is_point_on(const ui_region_t * region, const ui_point_t * point);

/**
 * @brief Check if two region has common parts
 *
 * @param region1 pointer to an region.
 * @param region2 pointer to an other region
 *
 * @return false: region1 and region2 has no common parts
 */
bool ui_region_is_on(const ui_region_t * region1, const ui_region_t * region2);

/**
 * @brief Check if a region is fully on an other
 *
 * @param region pointer to an region which could be in 'holder'
 * @param holder pointer to an region which could involve 'region'
 *
 * @return true: `region` is fully inside `holder`
 */
bool ui_region_is_in(const ui_region_t * region, const ui_region_t * holder);

/**
 * @brief Check if an region is valid

  A valid region has a non negative width and height.

 * @param region pointer to an region.
 *
 * @return true if region is valid
 */
bool ui_region_is_valid(const ui_region_t * region);

/**
 * @brief Check if an region is empty
 *
 * An empty region has a zero width or height, or is invalid
 *
 * @param region pointer to an region.
 *
 * @return true if region is empty
 */
bool ui_region_is_empty(const ui_region_t *region);

#ifdef __cplusplus
}
#endif

/**
 * @} end defgroup system_apis
 */
#endif /* FRAMEWORK_DISPLAY_INCLUDE_UI_REGION_H_ */
