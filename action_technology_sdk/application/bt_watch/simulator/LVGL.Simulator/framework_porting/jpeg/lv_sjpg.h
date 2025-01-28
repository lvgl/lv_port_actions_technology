/**
 * @file lv_sjpg.h
 *
 */

#ifndef LV_SJPEG_H
#define LV_SJPEG_H

#ifdef __cplusplus
extern "C" {
#endif

#include <lvgl/lvgl.h>
#include "tjpgd.h"

#if LV_USE_SJPG

/**
 * @brief  decode jpeg pitcutre.
 *
 * @param buf pointer jpeg data.
 * 
 * @param buf_size pointer jpeg data size.
 *
 * @param  buf_out jpeg  decoded data.
 *
 * @param  x x coordinate of data to be decoded.
 *
 * @param  y y coordinate of data to be decoded.
 *
 * @param  win_w width of data to be decoded.
 *
 * @param  win_h height of data to be decoded.
 *
 * @param  offset next write buf_out offset.
 *
 * @return JDR_OK...
 */
lv_res_t decoder_jpeg(uint8_t* buf, uint32_t buf_size, uint8_t* buf_out, uint16_t output_stride,
                      uint32_t x, uint32_t y, uint32_t win_w, uint32_t win_h);


#endif /*LV_USE_SJPG*/

#ifdef __cplusplus
}
#endif

#endif /* LV_SJPEG_H */
