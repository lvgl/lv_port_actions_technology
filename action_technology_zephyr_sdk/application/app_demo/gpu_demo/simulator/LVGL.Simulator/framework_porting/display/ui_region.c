/*
 * Copyright (c) 2019 Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 */

#include <ui_region.h>
#ifdef __ZEPHYR__
#  include <linker/linker-defs.h>
#else
#  define __ramfunc
#endif

/**
 * @brief Get the size of an region
 *
 * @param area_p pointer to an region
 *
 * @retval size of the region
 */
int32_t ui_region_get_size(const ui_region_t * region)
{
	return (int32_t)(region->x2 - region->x1 + 1) * (region->y2 - region->y1 + 1);
}

/**
 * @brief Set the x coordinate of an region
 *
 * @param region pointer to an region
 * @param x new x coordinate
 *
 * @retval N/A
 */
void ui_region_set_x(ui_region_t * region, int16_t x)
{
	region->x2 += x - region->x1;
	region->x1 = x;
}

/**
 * @brief Set the y coordinate of an region
 *
 * @param region pointer to an region
 * @param y new y coordinate
 *
 * @retval N/A
 */
void ui_region_set_y(ui_region_t * region, int16_t y)
{
	region->y2 += y - region->y1;
	region->y1 = y;
}

/**
 * @brief Set the position of an region (width and height will be kept)
 *
 * @param region pointer to an region
 * @param x new x coordinate
 * @param y new y coordinate
 *
 * @retval N/A
 */
void ui_region_set_pos(ui_region_t * region, int16_t x, int16_t y)
{
	ui_region_set_x(region, x);
	ui_region_set_y(region, y);
}

/**
 * @brief Set the width of an region
 *
 * @param region pointer to an region
 * @param w the new width of region (w == 1 makes x1 == x2)
 *
 * @retval N/A
 */
void ui_region_set_width(ui_region_t * region, int16_t w)
{
	region->x2 = region->x1 + w - 1;
}

/**
 * @brief Set the height of an region
 *
 * @param region pointer to an region
 * @param w the new height of region (h == 1 makes y1 == y2)
 *
 * @retval N/A
 */
void ui_region_set_height(ui_region_t * region, int16_t h)
{
	region->y2 = region->y1 + h - 1;
}

/**
 * @brief Move an region
 *
 * @param region pointer to an region
 * @param dx delta X to move
 * @param dy delta Y to move
 *
 * @retval N/A
 */
void ui_region_move(ui_region_t * region, int16_t dx, int16_t dy)
{
	region->x1 += dx;
	region->y1 += dy;
	region->x2 += dx;
	region->y2 += dy;
}

/**
 * @brief Get the common parts of two regions
 *
 * @param result pointer to an region, the result will be stored here
 * @param region1 pointer to the first region
 * @param region2 pointer to the second region
 *
 * @return false: the two area has NO common parts, res_p is invalid
 */
bool __ramfunc ui_region_intersect(ui_region_t * result, const ui_region_t * region1, const ui_region_t * region2)
{
	bool union_ok = true;

	/* Get the smaller region */
	result->x1 = UI_MAX(region1->x1, region2->x1);
	result->y1 = UI_MAX(region1->y1, region2->y1);
	result->x2 = UI_MIN(region1->x2, region2->x2);
	result->y2 = UI_MIN(region1->y2, region2->y2);

	/*If x1 or y1 greater then x2 or y2 then the areas union is empty*/
	if ((result->x1 > result->x2) || (result->y1 > result->y2)) {
		union_ok = false;
	}

	return union_ok;
}

/**
 * @brief merge two regions into a third which involves the other two
 *
 * @param result pointer to an region, the result will be stored here
 * @param region1 pointer to the first region
 * @param region2 pointer to the second region
 *
 * @retval N/A
 */
void ui_region_merge(ui_region_t * result, const ui_region_t * region1, const ui_region_t * region2)
{
	result->x1 = UI_MIN(region1->x1, region2->x1);
	result->y1 = UI_MIN(region1->y1, region2->y1);
	result->x2 = UI_MAX(region1->x2, region2->x2);
	result->y2 = UI_MAX(region1->y2, region2->y2);
}

/**
 * @brief Subtract the common parts of two regions
 *
 * @param result pointer to a region array (4 at maximum), the result will be stored here
 * @param region pointer to a region
 * @param exclude pointer to the region to be subtracted from 'region'
 *
 * @retval the number of result regions
 */
int ui_region_subtract(ui_region_t * result, const ui_region_t * region, const ui_region_t * exclude)
{
	int16_t cur_y = region->y1;
	int cnt = 0;

	if (region->x1 >= exclude->x1 && region->y1 >= exclude->y1 &&
		region->x2 <= exclude->x1 && region->y2 <= exclude->y1) {
		return 0;
	}

	if (ui_region_is_on(region, exclude) == false) {
		ui_region_copy(result, region);
		return 1;
	}

	/* the top part */
	if (region->y1 < exclude->y1) {
		result[cnt].x1 = region->x1;
		result[cnt].y1 = region->y1;
		result[cnt].x2 = region->x2;
		result[cnt++].y2 = exclude->y1 - 1;

		cur_y = exclude->y1;
	}

	/* the left part */
	if (region->x1 < exclude->x1) {
		result[cnt].x1 = region->x1;
		result[cnt].y1 = cur_y;
		result[cnt].x2 = exclude->x1 - 1;
		result[cnt++].y2 = UI_MIN(region->y2, exclude->y2);
	}

	/* the right part */
	if (region->x2 > exclude->x2) {
		result[cnt].x1 = exclude->x2 + 1;
		result[cnt].y1 = cur_y;
		result[cnt].x2 = region->x2;
		result[cnt++].y2 = UI_MIN(region->y2, exclude->y2);
	}

	/* the bottom part */
	if (region->y2 > exclude->y2) {
		result[cnt].x1 = region->x1;
		result[cnt].y1 = exclude->y2 + 1;
		result[cnt].x2 = region->x2;
		result[cnt++].y2 = region->y2;
	}

	return cnt;
}

/**
 * @brief Try smallest adjustment of region position to make it totally fall in the holder region
 *
 * @param region pointer to an region which should fall in 'holder'
 * @param holder pointer to an region which involves 'region'
 *
 * @retval N/A
 */
void ui_region_fit_in(ui_region_t * region, const ui_region_t * holder)
{
	int16_t dx = 0;
	int16_t dy = 0;

	if (region->x1 < holder->x1 && region->x2 < holder->x2) {
		dx = UI_MIN(holder->x1 - region->x1, holder->x2 - region->x2);
	} else if (region->x1 > holder->x1 && region->x2 > holder->x2) {
		dx = UI_MAX(holder->x1 - region->x1, holder->x2 - region->x2);
	}

	if (region->y1 < holder->y1 && region->y2 < holder->y2) {
		dy = UI_MIN(holder->y1 - region->y1, holder->y2 - region->y2);
	} else if (region->y1 > holder->y1 && region->y2 > holder->y2) {
		dy = UI_MAX(holder->y1 - region->y1, holder->y2 - region->y2);
	}

	ui_region_set_pos(region, region->x1 + dx, region->y1 + dy);
}

/**
 * @brief Check if a point is on an region
 *
 * @param region pointer to an region
 * @param point pointer to a point
 *
 * @return false: the point is out of the area
 */
bool ui_region_is_point_on(const ui_region_t * region, const ui_point_t * point)
{
	if ((point->x >= region->x1 && point->x <= region->x2) &&
		((point->y >= region->y1 && point->y <= region->y2))) {
		return true;
	} else {
		return false;
	}
}

/**
 * @brief Check if two region has common parts
 *
 * @param region1 pointer to an region.
 * @param region2 pointer to an other region
 *
 * @return false: region1 and region2 has no common parts
 */
bool ui_region_is_on(const ui_region_t * region1, const ui_region_t * region2)
{
	if ((region1->x1 <= region2->x2) && (region1->x2 >= region2->x1) &&
		(region1->y1 <= region2->y2) && (region1->y2 >= region2->y1)) {
		return true;
	} else {
		return false;
	}
}

/**
 * @brief Check if a region is fully on an other
 *
 * @param region pointer to an region which could be in 'holder'
 * @param holder pointer to an region which could involve 'region'
 *
 * @return true: `region` is fully inside `holder`
 */
bool ui_region_is_in(const ui_region_t * region, const ui_region_t * holder)
{
	if (region->x1 >= holder->x1 && region->y1 >= holder->y1 &&
		region->x2 <= holder->x2 && region->y2 <= holder->y2) {
		return true;
	} else {
		return false;
	}
}

/**
 * @brief Check if an region is valid

  A valid region has a non negative width and height.

 * @param region pointer to an region.
 *
 * @return true if region is valid
 */
bool ui_region_is_valid(const ui_region_t * region)
{
	return ui_region_get_width(region) >= 0 && ui_region_get_height(region) >= 0;
}

/**
 * @brief Check if an region is empty
 *
 * An empty region has a zero width or height, or is invalid
 *
 * @param region pointer to an region.
 *
 * @return true if region is empty
 */
bool ui_region_is_empty(const ui_region_t *region)
{
	return ui_region_get_width(region) <= 0 || ui_region_get_height(region) <= 0;
}