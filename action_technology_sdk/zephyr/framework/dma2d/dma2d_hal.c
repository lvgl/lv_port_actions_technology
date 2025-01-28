/*
 * Copyright (c) 2020, Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 ******************************************************************************
 * @file    dma2d_hal.c
 * @brief   DMA2D HAL module driver.
 *          This file provides firmware functions to manage the following
 *          functionalities of the DMA2D peripheral:
 *           + Initialization and de-initialization functions
 *           + IO operation functions
 *           + Peripheral Control functions
 *           + Peripheral State and Errors functions
 *
 @verbatim
 ==============================================================================
                       ##### How to use this driver #####
 ==============================================================================
   [..]
     (#) Initialize the DMA2D module using hal_dma2d_init() function.
     (#) Program the required configuration through the following parameters:
         the transfer mode, the output color mode and the output offset using
         hal_dma2d_config_output() function.
     (#) Program the required configuration through the following parameters:
        -@-   the input color mode, the input color, the input alpha value, the alpha mode,
             the red/blue swap mode, the inverted alpha mode and the input offset using
             hal_dma2d_config_layer() function for foreground or/and background layer.
        -@-   the rotation configuration using hal_dma2d_config_transform.
    *** Polling mode IO operation ***
    =================================
   [..]
      (#) Configure pdata parameter (explained hereafter), destination and data length
          and enable the transfer using hal_dma2d_start().
      (#) Wait for end of transfer using hal_dma2d_poll_transfer(), at this stage
          user can specify the value of timeout according to his end application.
    *** Interrupt mode IO operation ***
    ===================================
    [..]
      (#) Use function @ref hal_dma2d_register_callback() to register user callbacks.
      (#) Configure pdata parameter, destination and data length and enable
          the transfer using hal_dma2d_start().
      (#) At the end of data transfer dma2d_device_handler() function is executed and user can
          add his own function by customization of function pointer xfer_callback (member
          of DMA2D handle structure).
      (#) In case of error, the dma2d_device_handler() function calls the callback
          xfer_error_callback.
        -@-   In Register-to-Memory transfer mode, pdata parameter is the register
              color, in Memory-to-memory or Memory-to-Memory with pixel format
              conversion pdata is the source address.
        -@-   Configure the foreground source address, the background source address,
              the destination and data length then Enable the transfer using
              hal_dma2d_blending_start() either in polling mode or interrupt mode.
        -@-   hal_dma2d_blending_start() function is used if the memory to memory
              with blending transfer mode is selected.
        -@-   hal_dma2d_transform_start() function is used if the memory to memory
              with rotation transfer mode is selected.
        -@-   hal_dma2d_clut_load_start() function is used if the layer's color lookup table
              is required to update.
     (#) To control the DMA2D state, use the following function: hal_dma2d_get_state().
     (#) To read the DMA2D error code, use the following function: hal_dma2d_get_error().
    *** Callback registration ***
    ===================================
    [..]
     (#) Use function @ref hal_dma2d_register_callback() to register a user callback.
     (#) Function @ref hal_dma2d_register_callback() allows to register the callbacks:
         This function takes as parameters the HAL peripheral handle
         and a pointer to the user callback function.
     (#) Use function @ref hal_dma2d_unregister_callback() to reset a callback to the default
         weak (surcharged) function.
    [..]
     (@) You can refer to the DMA2D HAL driver header file for more useful macros
 @endverbatim
 */

/* Includes ------------------------------------------------------------------*/
#include <string.h>
#include <assert.h>
#include <display/sw_math.h>
#include <dma2d_hal.h>
#include <board_cfg.h>
#include <linker/linker-defs.h>
#include <tracing/tracing.h>

/** @defgroup DMA2D  DMA2D
 * @brief DMA2D HAL module driver
 * @{
 */

/* Private types -------------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/** @defgroup DMA2D_Private_Constants DMA2D Private Constants
 * @{
 */
#define FULL_DEV_IDX  0
#define LITE_DEV_IDX  1

/**
 * @}
 */

/* Private variables ---------------------------------------------------------*/
/* DMA2D global enable bit */
static bool global_en = true;
/* DMA2D device initialize or not ? */
static bool dma2d_dev_initialized = false;
/* DMA2D device handle */
static const struct device *dma2d_dev[2];
static struct display_engine_capabilities dma2d_cap[2];

/* Private constants ---------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
#define IS_DMA2D_LAYER(layer)             (((layer) == HAL_DMA2D_BACKGROUND_LAYER) || ((layer) == HAL_DMA2D_FOREGROUND_LAYER))

#define IS_DMA2D_MODE(mode)               (((mode) == HAL_DMA2D_R2M)          || ((mode) == HAL_DMA2D_M2M) || \
                                           ((mode) == HAL_DMA2D_M2M_BLEND)    || ((mode) == HAL_DMA2D_M2M_BLEND_FG) || \
                                           ((mode) == HAL_DMA2D_M2M_BLEND_BG) || ((mode) == HAL_DMA2D_M2M_TRANSFORM))

#define IS_DMA2D_LINE(line)               ((line) <= HAL_DMA2D_LINE)
#define IS_DMA2D_PIXEL(pixel)             ((pixel) <= HAL_DMA2D_PIXEL)
#define IS_DMA2D_PITCH(pitch)             ((pitch) <= HAL_DMA2D_PITCH)

#define IS_DMA2D_ALPHA_MODE(alpha_mode)   (((alpha_mode) == HAL_DMA2D_NO_MODIF_ALPHA) || \
                                           ((alpha_mode) == HAL_DMA2D_REPLACE_ALPHA)  || \
                                           ((alpha_mode) == HAL_DMA2D_COMBINE_ALPHA))

/* Private function prototypes -----------------------------------------------*/
/** @addtogroup DMA2D_Private_Functions DMA2D Private Functions
 * @{
 */
__de_func static int dma2d_set_config(hal_dma2d_handle_t *hdma2d, uint32_t fg_address,
				     uint32_t bg_address, uint32_t dst_address, uint32_t width,
				     uint32_t height);
__de_func static void dma2d_set_dst_buffer(hal_dma2d_handle_t *hdma2d, display_buffer_t *dst,
					  uint32_t dst_address, uint32_t width, uint32_t height);
__de_func static void dma2d_set_src_buffer(hal_dma2d_handle_t *hdma2d, display_buffer_t *src,
					  uint16_t layer_idx, uint32_t src_address);
/**
 * @}
 */

/* Private functions ---------------------------------------------------------*/
/**
 * @brief  Get the DMA2D device
 * @retval Pointer to the DMA2D device structure
 */
static const struct device *dma2d_get_device(uint32_t preferred_modes)
{
	const uint32_t FULL_FEATURE_MODES = HAL_DMA2D_M2M_BLEND |
			HAL_DMA2D_M2M_BLEND_FG | HAL_DMA2D_M2M_BLEND_BG |
			HAL_DMA2D_M2M_TRANSFORM | HAL_DMA2D_M2M_TRANSFORM_BLEND |
			HAL_DMA2D_M2M_TRANSFORM_CIRCLE;
	const struct device *device = NULL;

	if (dma2d_dev_initialized == false) {
		dma2d_dev_initialized = true;

		dma2d_dev[FULL_DEV_IDX] = device_get_binding(CONFIG_DISPLAY_ENGINE_DEV_NAME);
		if (dma2d_dev[FULL_DEV_IDX])
			display_engine_get_capabilities(dma2d_dev[FULL_DEV_IDX], &dma2d_cap[FULL_DEV_IDX]);

		dma2d_dev[LITE_DEV_IDX] = device_get_binding(CONFIG_DMA2D_LITE_DEV_NAME);
		if (dma2d_dev[LITE_DEV_IDX])
			display_engine_get_capabilities(dma2d_dev[LITE_DEV_IDX], &dma2d_cap[LITE_DEV_IDX]);
	}

	if (!(preferred_modes & FULL_FEATURE_MODES) && dma2d_dev[LITE_DEV_IDX]) {
		if (!(preferred_modes & HAL_DMA2D_R2M) || dma2d_cap[LITE_DEV_IDX].support_fill) {
			device = dma2d_dev[LITE_DEV_IDX];
		}
	}

	if (device == NULL) {
		device = dma2d_dev[FULL_DEV_IDX];
	}

	assert(device != NULL);

	return device;
}

__de_func static bool is_dma2d_device_ready(const struct device *device)
{
	if (device == NULL) {
		return false;
	}

	if (!global_en && device == dma2d_dev[FULL_DEV_IDX]) {
		return false;
	}

	return true;
}

/* Exported functions --------------------------------------------------------*/
/** @defgroup DMA2D_Exported_Functions DMA2D Exported Functions
 * @{
 */

/** @defgroup DMA2D_Exported_Functions_Group1 Initialization and de-initialization functions
 *  @brief   Initialization and Configuration functions
 *
@verbatim
 ===============================================================================
                ##### Initialization and Configuration functions #####
 ===============================================================================
    [..]  This section provides functions allowing to:
      (+) Initialize and configure the DMA2D
      (+) De-initialize the DMA2D

@endverbatim
 * @{
 */

/**
 * @brief  DMA2D callback from device driver, may called in interrupt routine
 * @param err_code DMA2D device error code
 * @param cmd_seq Current DMA2D device command sequence
 * @param user_data Address of structure hal_dma2d_handle_t
 */
__de_func void dma2d_device_handler(int err_code, uint16_t cmd_seq, void *user_data)
{
	hal_dma2d_handle_t *hdma2d = user_data;
	uint32_t error_code;

	/* Update error code */
	error_code = (err_code != 0) ? HAL_DMA2D_ERROR_TIMEOUT : HAL_DMA2D_ERROR_NONE;
	hdma2d->error_code |= error_code;

	if (hdma2d->xfer_callback != NULL) {
		/* Transfer error Callback */
		hdma2d->xfer_callback(hdma2d, cmd_seq, error_code);
	}

	atomic_dec(&hdma2d->xfer_count);
}

/**
 * @brief  Initialize the DMA2D peripheral and create the associated handle.
 * @param  hdma2d pointer to a hal_dma2d_handle_t structure that contains
 *                 the configuration information for the DMA2D.
 * @param preferred_modes "bitwise or" of output modes that maybe used.
 * @retval 0 on success else negative errno code.
 */
int hal_dma2d_init(hal_dma2d_handle_t *hdma2d, uint32_t preferred_modes)
{
	/* Check the Parameter */
	if (hdma2d == NULL) {
		return -EINVAL;
	}

	/* Initialize the handle */
	memset(hdma2d, 0, sizeof(*hdma2d));

	/* Check the DMA2D peripheral existence */
	hdma2d->device = dma2d_get_device(preferred_modes);
	if (hdma2d->device == NULL) {
		return -ENODEV;
	}

	/* Open the DMA2D device instance */
	hdma2d->instance = display_engine_open(hdma2d->device, 0);
	if (hdma2d->instance < 0) {
		return -EBUSY;
	}

	/* Register the DMA2D device instance callback */
	display_engine_register_callback(hdma2d->device, hdma2d->instance, dma2d_device_handler, hdma2d);

	atomic_set(&hdma2d->xfer_count, 0);

	/* Update error code */
	hdma2d->error_code = HAL_DMA2D_ERROR_NONE;

	return 0;
}

/**
 * @brief  Deinitializes the DMA2D peripheral registers to their default reset
 *         values.
 * @param  hdma2d pointer to a hal_dma2d_handle_t structure that contains
 *                 the configuration information for the DMA2D.
 * @retval 0 on success else negative errno code.
 */
int hal_dma2d_deinit(hal_dma2d_handle_t *hdma2d)
{
	/* Check the DMA2D peripheral State */
	if (hdma2d == NULL || hdma2d->device == NULL || hdma2d->instance < 0) {
		return -EINVAL;
	}

	/* Close the DMA2D device instance */
	display_engine_close(hdma2d->device, hdma2d->instance);

	/* Assign invald value */
	hdma2d->device = NULL;
	hdma2d->instance = -1;

	return 0;
}

/**
 * @brief  Register a User DMA2D Callback
 *         To be used instead of the weak (surcharged) predefined callback
 * @param hdma2d DMA2D handle
 * @param callback_fn pointer to the callback function
 * @retval 0 on success else negative errno code.
 */
int hal_dma2d_register_callback(hal_dma2d_handle_t *hdma2d, hal_dma2d_callback_t callback_fn)
{
	int status = 0;

	if (callback_fn == NULL) {
		/* Update the error code */
		hdma2d->error_code |= HAL_DMA2D_ERROR_INVALID_CALLBACK;
		return -EINVAL;
	}

	if (HAL_DMA2D_STATE_READY == hal_dma2d_get_state(hdma2d)) {
		hdma2d->xfer_callback = callback_fn;
	} else {
		/* Update the error code */
		hdma2d->error_code |= HAL_DMA2D_ERROR_INVALID_CALLBACK;
		/* update return status */
		status =  -EBUSY;
	}

	return status;
}

/**
 * @brief  Unregister a DMA2D Callback
 *         DMA2D Callback is redirected to the weak (surcharged) predefined callback
 * @param hdma2d DMA2D handle
 * @retval 0 on success else negative errno code.
 */
int hal_dma2d_unregister_callback(hal_dma2d_handle_t *hdma2d)
{
	int status = 0;

	if (HAL_DMA2D_STATE_READY == hal_dma2d_get_state(hdma2d)) {
		hdma2d->xfer_callback = NULL;
	} else {
		/* Update the error code */
		hdma2d->error_code |= HAL_DMA2D_ERROR_INVALID_CALLBACK;
		/* update return status */
		status =  -EBUSY;
	}

	return status;
}

/**
 * @}
 */


/** @defgroup DMA2D_Exported_Functions_Group2 IO operation functions
 *  @brief   IO operation functions
 *
@verbatim
 ===============================================================================
                      #####  IO operation functions  #####
 ===============================================================================
    [..]  This section provides functions allowing to:
      (+) Configure the pdata, destination address and data size then
          start the DMA2D transfer.
      (+) Configure the source for foreground and background, destination address
          and data size then start a MultiBuffer DMA2D transfer.
      (+) Configure the pdata, destination address and data size then
          start the DMA2D transfer with interrupt.
      (+) Configure the source for foreground and background, destination address
          and data size then start a MultiBuffer DMA2D transfer with interrupt.
      (+) Poll for transfer complete.
      (+) handle DMA2D interrupt request.


@endverbatim
 * @{
 */

/**
 * @brief  Start the DMA2D Transfer.
 * @param  hdma2d     Pointer to a hal_dma2d_handle_t structure that contains
 *                     the configuration information for the DMA2D.
 * @param  pdata      Configure the source memory Buffer address if
 *                     Memory-to-Memory or Memory-to-Memory with pixel format
 *                     conversion mode is selected, or configure
 *                     the color value if Register-to-Memory mode is selected.
 * @param  dst_address The destination memory Buffer address.
 * @param  width      The width of data to be transferred from source to destination (expressed in number of pixels per line).
 * @param  height     The height of data to be transferred from source to destination (expressed in number of lines).
 * @retval command sequence (uint16_t) on success else negative errno code.
 */
__de_func int hal_dma2d_start(hal_dma2d_handle_t *hdma2d, uint32_t pdata, uint32_t dst_address,
			      uint32_t width, uint32_t height)
{
	return dma2d_set_config(hdma2d, pdata, 0, dst_address, width, height);
}

/**
 * @brief  Start the multi-source DMA2D Transfer.
 * @param  hdma2d      Pointer to a hal_dma2d_handle_t structure that contains
 *                      the configuration information for the DMA2D.
 * @param  fg_address The source memory Buffer address for the foreground layer.
 * @param  bg_address The source memory Buffer address for the background layer.
 * @param  dst_address  The destination memory Buffer address.
 * @param  width       The width of data to be transferred from source to destination (expressed in number of pixels per line).
 * @param  height      The height of data to be transferred from source to destination (expressed in number of lines).
 * @retval command sequence (uint16_t) on success else negative errno code.
 */
__de_func int hal_dma2d_blending_start(hal_dma2d_handle_t *hdma2d, uint32_t fg_address,
				       uint32_t bg_address, uint32_t dst_address, uint32_t width,
				       uint32_t height)
{
	return dma2d_set_config(hdma2d, fg_address, bg_address, dst_address, width, height);
}

/**
 * @brief  Start the DMA2D Rotation Transfer with interrupt enabled.
 *
 * The source size must be square, and only the ring area defined in hal_dma2d_rotation_cfg_t
 * is rotated, and the pixels inside the inner ring can be filled with constant
 * color defined in hal_dma2d_rotation_cfg_t.
 *
 * @param  hdma2d     Pointer to a hal_dma2d_handle_t structure that contains
 *                     the configuration information for the DMA2D.
 * @param  src_address The source memory Buffer start address.
 * @param  dst_address The destination memory Buffer address.
 * @param  x        X coord of top-left corner of dest area.
 * @param  y        Y coord of top-left corner of dest area.
 * @param  width    Width of dest area.
 * @param  height   Height of dest area.
 * @retval command sequence (uint16_t) on success else negative errno code.
 */
int hal_dma2d_transform_start(hal_dma2d_handle_t *hdma2d, uint32_t src_address, uint32_t dst_address,
		int16_t x, int16_t y, uint16_t width, uint16_t height)
{
	hal_dma2d_transform_cfg_t *cfg = &hdma2d->trans_cfg;
	display_engine_transform_param_t *hw_param = &cfg->hw_param;
	display_buffer_t dst, src;
	int32_t sw_x0, sw_y0;
	int res = 0;

	/* Check the peripheral existence */
	if (!is_dma2d_device_ready(hdma2d->device) || hdma2d->instance < 0) {
		return -ENODEV;
	}

	dma2d_set_dst_buffer(hdma2d, &dst, dst_address, width, height);
	dma2d_set_src_buffer(hdma2d, &src, HAL_DMA2D_FOREGROUND_LAYER, src_address);

	if (hdma2d->output_cfg.mode == HAL_DMA2D_M2M_TRANSFORM_CIRCLE) {
		if (x != cfg->image_x0 || width != cfg->circle.outer_diameter + 1)
			return -EINVAL;

		if (y < cfg->image_y0 || y + height > cfg->image_x0 + cfg->circle.outer_diameter + 1)
			return -EINVAL;

		if (src.desc.pixel_format != dst.desc.pixel_format ||
			src.desc.width != dst.desc.width ||
			src.desc.width != src.desc.height)
			return -EINVAL;

		hw_param->circle.line_start = y;
	}

	x -= cfg->image_x0;
	y -= cfg->image_y0;

	sw_x0 = hw_param->matrix.tx;
	sw_y0 = hw_param->matrix.ty;
	hw_param->matrix.tx += y * hw_param->matrix.shx + x * hw_param->matrix.sx;
	hw_param->matrix.ty += y * hw_param->matrix.sy + x * hw_param->matrix.shy;
	hw_param->color.full = hdma2d->layer_cfg[HAL_DMA2D_FOREGROUND_LAYER].input_alpha;

	res = display_engine_transform(hdma2d->device, hdma2d->instance, &dst, &src, hw_param);
	if (res < 0) {
		/* Update error code */
		hdma2d->error_code |= HAL_DMA2D_ERROR_CE;
	} else {
		atomic_inc(&hdma2d->xfer_count);
	}

	hw_param->matrix.tx = sw_x0;
	hw_param->matrix.ty = sw_y0;

	return res;
}

/**
 * @brief  Start DMA2D CLUT Loading.
 * @param  hdma2d   Pointer to a DMA2D_HandleTypeDef structure that contains
 *                   the configuration information for the DMA2D.
 * @param  layer_idx DMA2D Layer index.
 *                   This parameter can be one of the following values:
 *                   DMA2D_BACKGROUND_LAYER(0) / DMA2D_FOREGROUND_LAYER(1)
 * @param  size  Number of colors in the color look up table
 * @param  clut  Pointer to the color look up table.
 * @retval command sequence (uint16_t) on success else negative errno code.
 */
__de_func int hal_dma2d_clut_load_start(hal_dma2d_handle_t *hdma2d, uint16_t layer_idx, uint16_t size, const uint32_t *clut)
{
	int res = 0;

	/* Check the peripheral existence */
	if (!is_dma2d_device_ready(hdma2d->device) || hdma2d->instance < 0) {
		return -ENODEV;
	}

	if (layer_idx > HAL_DMA2D_FOREGROUND_LAYER) {
		return -EINVAL;
	}

	res = display_engine_set_clut(hdma2d->device, hdma2d->instance, layer_idx + 1, size, clut);
	if (res < 0) {
		/* Update error code */
		hdma2d->error_code |= HAL_DMA2D_ERROR_CE;
	} else {
		atomic_inc(&hdma2d->xfer_count);
	}

	return res;
}

/**
 * @brief  Polling for transfer complete.
 * @param  hdma2d Pointer to a hal_dma2d_handle_t structure that contains
 *                 the configuration information for the DMA2D.
 * @param  timeout timeout duration in milliseconds, if negative, means wait forever
 * @retval 0 on success else negative errno code.
 */
int hal_dma2d_poll_transfer(hal_dma2d_handle_t *hdma2d, int32_t timeout)
{
 	int status = 0;

	/* Check the peripheral existence */
	if (hdma2d->device == NULL || hdma2d->instance < 0) {
		return -ENODEV;
	}

	if (HAL_DMA2D_STATE_BUSY != hal_dma2d_get_state(hdma2d)) {
		return 0;
	}

	if (display_engine_poll(hdma2d->device, hdma2d->instance, timeout)) {
		/* Update error code */
		hdma2d->error_code |= HAL_DMA2D_ERROR_TIMEOUT;
		/* update return status */
		status = -ETIME;
	}

	return status;
}

/**
 * @}
 */

/** @defgroup DMA2D_Exported_Functions_Group3 Peripheral Control functions
 *  @brief    Peripheral Control functions
 *
@verbatim
 ===============================================================================
                    ##### Peripheral Control functions #####
 ===============================================================================
    [..]  This section provides functions allowing to:
      (+) Configure the DMA2D transfer mode and output parameters.
      (+) Configure the DMA2D foreground or background layer parameters.
      (+) Configure the DMA2D rotation parameters.

@endverbatim
 * @{
 */

/**
 * @brief  Configure the DMA2D transfer mode and output according to the
 *         specified parameters in the hal_dma2d_handle_t.
 * @param  hdma2d Pointer to a hal_dma2d_handle_t structure that contains
 *                 the configuration information for the DMA2D.
 * @retval 0 on success else negative errno code.
 */
__de_func int hal_dma2d_config_output(hal_dma2d_handle_t *hdma2d)
{
	return 0;
}

/**
 * @brief  Configure the DMA2D Layer according to the specified
 *         parameters in the hal_dma2d_handle_t.
 * @param  hdma2d Pointer to a hal_dma2d_handle_t structure that contains
 *                 the configuration information for the DMA2D.
 * @param  layer_idx DMA2D Layer index.
 *                   This parameter can be one of the following values:
 *                   HAL_DMA2D_BACKGROUND_LAYER(0) / HAL_DMA2D_FOREGROUND_LAYER(1)
 * @retval 0 on success else negative errno code.
 */
__de_func int hal_dma2d_config_layer(hal_dma2d_handle_t *hdma2d, uint16_t layer_idx)
{
	hal_dma2d_layer_cfg_t *cfg = &hdma2d->layer_cfg[layer_idx];

	if (layer_idx == HAL_DMA2D_FOREGROUND_LAYER) {
		if (cfg->alpha_mode == HAL_DMA2D_NO_MODIF_ALPHA) {
			cfg->input_alpha |= 0xff000000;
		} else if (cfg->alpha_mode == HAL_DMA2D_REPLACE_ALPHA) {
			return -ENOTSUP;
		}
	}

	return 0;
}

/**
 * @brief  Configure the DMA2D Rotation according to the specified
 *         parameters in the hal_dma2d_handle_t.
 * @param  hdma2d Pointer to a hal_dma2d_handle_t structure that contains
 *                 the configuration information for the DMA2D.
 * @retval 0 on success else negative errno code.
 */
int hal_dma2d_config_transform(hal_dma2d_handle_t *hdma2d)
{
	hal_dma2d_transform_cfg_t *cfg = &hdma2d->trans_cfg;
	display_engine_transform_param_t *hw_param = &cfg->hw_param;
	uint16_t revert_angle, revert_scale_x, revert_scale_y;
	int32_t pivot_x, pivot_y;
	int32_t width = hdma2d->layer_cfg[HAL_DMA2D_FOREGROUND_LAYER].input_width;
	int32_t height = hdma2d->layer_cfg[HAL_DMA2D_FOREGROUND_LAYER].input_height;

	/* Just assume the top-left coordinate of the source before transformation
	 * is (0, 0) in the display coordinate system.
	 */
	switch (cfg->mode) {
	case HAL_DMA2D_ROT_90:
		hw_param->matrix = (display_matrix_t) {
			.sx = 0, .shx = FIXEDPOINT12(1), .tx = 0,
			.shy = FIXEDPOINT12(-1), .sy = 0, .ty = FIXEDPOINT12(height - 1),
		};
		return 0;

	case HAL_DMA2D_ROT_180:
		hw_param->matrix = (display_matrix_t) {
			.sx = FIXEDPOINT12(-1), .shx = 0, .tx = FIXEDPOINT12(width - 1),
			.shy = 0, .sy = FIXEDPOINT12(-1), .ty = FIXEDPOINT12(height - 1),
		};
		return 0;

	case HAL_DMA2D_ROT_270:
		hw_param->matrix = (display_matrix_t) {
			.sx = 0, .shx = FIXEDPOINT12(-1), .tx = FIXEDPOINT12(width - 1),
			.shy = FIXEDPOINT12(1), .sy = 0, .ty = 0,
		};
		return 0;

	case HAL_DMA2D_FLIP_H:
		hw_param->matrix = (display_matrix_t) {
			.sx = FIXEDPOINT12(-1), .shx = 0, .tx = FIXEDPOINT12(width - 1),
			.shy = 0, .sy = FIXEDPOINT12(1), .ty = 0,
		};
		return 0;

	case HAL_DMA2D_FLIP_V:
		hw_param->matrix = (display_matrix_t) {
			.sx = FIXEDPOINT12(1), .shx = 0, .tx = 0,
			.shy = 0, .sy = FIXEDPOINT12(-1), .ty = FIXEDPOINT12(height - 1),
		};
		return 0;

	case HAL_DMA2D_ROT2D:
	default:
		break;
	}

	/* Check the parameters */
	if (cfg->angle >= 3600) {
		return -EINVAL;
	}

	hw_param->blend_en = (hdma2d->output_cfg.mode == HAL_DMA2D_M2M_TRANSFORM_BLEND);

	if (hdma2d->output_cfg.mode== HAL_DMA2D_M2M_TRANSFORM_CIRCLE) {
		if (cfg->circle.outer_diameter <= 0 ||
			cfg->circle.inner_diameter >= cfg->circle.outer_diameter) {
			return -EINVAL;
		}

		hw_param->is_circle = 1;
		hw_param->circle.outer_radius_sq = cfg->circle.outer_diameter * cfg->circle.outer_diameter;
		hw_param->circle.inner_radius_sq = cfg->circle.inner_diameter * cfg->circle.inner_diameter;
		hw_param->circle.line_start = 0;

		pivot_x = FIXEDPOINT12(cfg->circle.outer_diameter + 1) / 2 + PX_FIXEDPOINT12(0);
		pivot_y = FIXEDPOINT12(cfg->circle.outer_diameter + 1) / 2 + PX_FIXEDPOINT12(0);
		revert_scale_x = revert_scale_y = HAL_DMA2D_SCALE_NONE; /* no scale */
	} else {
		/* only support scaling up */
		if (cfg->rect.scale_x < HAL_DMA2D_SCALE_NONE ||
			cfg->rect.scale_y < HAL_DMA2D_SCALE_NONE) {
			return -EINVAL;
		}

		hw_param->is_circle = 0;
		pivot_x = FIXEDPOINT12(cfg->rect.pivot_x);
		pivot_y = FIXEDPOINT12(cfg->rect.pivot_y);
		revert_scale_x = HAL_DMA2D_SCALE_NONE * HAL_DMA2D_SCALE_NONE / cfg->rect.scale_x;
		revert_scale_y = HAL_DMA2D_SCALE_NONE * HAL_DMA2D_SCALE_NONE / cfg->rect.scale_y;
	}

	revert_angle = 3600 - cfg->angle;

	/* coordinates in the destination coordinate system */
	hw_param->matrix.tx = PX_FIXEDPOINT12(0);
	hw_param->matrix.ty = PX_FIXEDPOINT12(0);
	hw_param->matrix.sx = FIXEDPOINT12(1);
	hw_param->matrix.shy = FIXEDPOINT12(0);
	hw_param->matrix.shx = FIXEDPOINT12(0);
	hw_param->matrix.sy = FIXEDPOINT12(1);

	/* rotate back to the source coordinate system */
	sw_transform_point32_rot_first(&hw_param->matrix.tx, &hw_param->matrix.ty,
			hw_param->matrix.tx, hw_param->matrix.ty,
			pivot_x, pivot_y, revert_angle, revert_scale_x, revert_scale_y, 8);
	sw_transform_point32_rot_first(&hw_param->matrix.sx, &hw_param->matrix.shy,
			hw_param->matrix.sx, hw_param->matrix.shy,
			FIXEDPOINT12(0), FIXEDPOINT12(0), revert_angle, revert_scale_x, revert_scale_y, 8);
	sw_transform_point32_rot_first(&hw_param->matrix.shx, &hw_param->matrix.sy,
			hw_param->matrix.shx, hw_param->matrix.sy,
			FIXEDPOINT12(0), FIXEDPOINT12(0), revert_angle, revert_scale_x, revert_scale_y, 8);

	/* move to the source pixel coordinate system */
	hw_param->matrix.tx -= PX_FIXEDPOINT12(0);
	hw_param->matrix.ty -= PX_FIXEDPOINT12(0);

	return 0;
}

/**
 * @}
 */


/** @defgroup DMA2D_Exported_Functions_Group4 Peripheral State and Error functions
 *  @brief    Peripheral State functions
 *
@verbatim
 ===============================================================================
                  ##### Peripheral State and Errors functions #####
 ===============================================================================
    [..]
    This subsection provides functions allowing to:
      (+) Get the DMA2D state
      (+) Get the DMA2D error code

@endverbatim
 * @{
 */

/**
 * @brief  Return the DMA2D state
 * @param  hdma2d pointer to a hal_dma2d_handle_t structure that contains
 *                 the configuration information for the DMA2D.
 * @retval HAL state
 */
hal_dma2d_state_e hal_dma2d_get_state(hal_dma2d_handle_t *hdma2d)
{
	return (atomic_get(&hdma2d->xfer_count) > 0) ?
			HAL_DMA2D_STATE_BUSY : HAL_DMA2D_STATE_READY;
}

/**
 * @brief  Return the DMA2D error code
 * @param  hdma2d  pointer to a hal_dma2d_handle_t structure that contains
 *               the configuration information for DMA2D.
 * @retval DMA2D Error Code
 */
uint32_t hal_dma2d_get_error(hal_dma2d_handle_t *hdma2d)
{
	uint32_t error_code = hdma2d->error_code;

	/* clear the error code */
	hdma2d->error_code = 0;

	return error_code;
}

/**
 * @}
 */

/**
 * @}
 */


/** @defgroup DMA2D_Private_Functions DMA2D Private Functions
 * @{
 */

__de_func static void dma2d_set_dst_buffer(hal_dma2d_handle_t *hdma2d, display_buffer_t *dst,
					   uint32_t dst_address, uint32_t width, uint32_t height)
{
	dst->desc.width = width;
	dst->desc.height = height;
	dst->desc.pixel_format = hdma2d->output_cfg.color_format;
	dst->desc.pitch = hdma2d->output_cfg.output_pitch;
	dst->addr = dst_address;
}

__de_func static void dma2d_set_src_buffer(hal_dma2d_handle_t *hdma2d, display_buffer_t *src,
					   uint16_t layer_idx, uint32_t src_address)
{
	src->desc.pixel_format = hdma2d->layer_cfg[layer_idx].color_format,
	src->desc.height = hdma2d->layer_cfg[layer_idx].input_height;
	src->desc.width = hdma2d->layer_cfg[layer_idx].input_width;
	src->desc.pitch = hdma2d->layer_cfg[layer_idx].input_pitch;
	src->px_ofs = hdma2d->layer_cfg[layer_idx].input_xofs;
	src->addr = src_address;
}

/**
 * @brief  Set the DMA2D transfer parameters.
 * @param  hdma2d     Pointer to a hal_dma2d_handle_t structure that contains
 *                     the configuration information for the specified DMA2D.
 * @param  fg_address The source memory Buffer address for the foreground layer.
 * @param  bg_address The source memory Buffer address for the background layer.
 * @param  dst_address The destination memory Buffer address.
 * @param  width      The width of data to be transferred from source to destination.
 * @param  height     The height of data to be transferred from source to destination.
 * @retval command sequence (uint16_t) on success else negative errno code.
 */
__de_func static int dma2d_set_config(hal_dma2d_handle_t *hdma2d, uint32_t fg_address,
				     uint32_t bg_address, uint32_t dst_address, uint32_t width,
				     uint32_t height)
{
	display_buffer_t dst, fg, bg;
	display_color_t fg_color, bg_color;
	int res = 0;

	/* Check the peripheral existence */
	if (!is_dma2d_device_ready(hdma2d->device) || hdma2d->instance < 0) {
		return -ENODEV;
	}

	/* Check the parameters */
	if (!IS_DMA2D_PIXEL(width) || !IS_DMA2D_LINE(height)) {
		return -EINVAL;
	}

	dma2d_set_dst_buffer(hdma2d, &dst, dst_address, width, height);

	fg_color.full = hdma2d->layer_cfg[HAL_DMA2D_FOREGROUND_LAYER].input_alpha; /* ARGB8888 */
	bg_color.full = hdma2d->layer_cfg[HAL_DMA2D_BACKGROUND_LAYER].input_alpha; /* ARGB8888 */

	switch (hdma2d->output_cfg.mode) {
	case HAL_DMA2D_R2M:
		fg_color.full = fg_address;
		res = display_engine_fill(hdma2d->device, hdma2d->instance, &dst, fg_color);
		break;
	case HAL_DMA2D_M2M:
		dma2d_set_src_buffer(hdma2d, &fg, HAL_DMA2D_FOREGROUND_LAYER, fg_address);
		res = display_engine_blit(hdma2d->device, hdma2d->instance, &dst, &fg);
		break;
	case HAL_DMA2D_M2M_BLEND:
		dma2d_set_src_buffer(hdma2d, &fg, HAL_DMA2D_FOREGROUND_LAYER, fg_address);
		dma2d_set_src_buffer(hdma2d, &bg, HAL_DMA2D_BACKGROUND_LAYER, bg_address);
		res = display_engine_blend(hdma2d->device, hdma2d->instance, &dst, &fg, fg_color, &bg, bg_color);
		break;
	case HAL_DMA2D_M2M_BLEND_FG:
		fg_color.full = fg_address;
		dma2d_set_src_buffer(hdma2d, &bg, HAL_DMA2D_BACKGROUND_LAYER, bg_address);
		res = display_engine_blend(hdma2d->device, hdma2d->instance, &dst, NULL, fg_color, &bg, bg_color);
		break;
	case HAL_DMA2D_M2M_BLEND_BG:
		bg_color.full = bg_address;
		dma2d_set_src_buffer(hdma2d, &fg, HAL_DMA2D_FOREGROUND_LAYER, fg_address);
		res = display_engine_blend(hdma2d->device, hdma2d->instance, &dst, &fg, fg_color, NULL, bg_color);
		break;
	default:
		res = -EINVAL;
		break;
	}

	if (res < 0) {
		/* Update error code */
		hdma2d->error_code |= HAL_DMA2D_ERROR_CE;
	} else {
		atomic_inc(&hdma2d->xfer_count);
	}

	return res;
}

/**
 * @brief  Global enable/disable DMAD functions
 * @param  enabled enable or not
 * @retval N/A
 */
void hal_dma2d_set_global_enabled(bool enabled)
{
	const struct device *dma2d_dev = dma2d_get_device(HAL_DMA2D_M2M_BLEND);

	if (dma2d_dev == NULL || enabled == global_en) {
		return;
	}

	sys_trace_u32(SYS_TRACE_ID_DMA2D_EN, enabled);

	if (!enabled) {
		global_en = false;
		display_engine_control(dma2d_dev, DISPLAY_ENGINE_CTRL_WORK_MODE,
				(void *)DISPLAY_ENGINE_MODE_DISPLAY_ONLY, NULL);
	} else {
		global_en = true;
		display_engine_control(dma2d_dev, DISPLAY_ENGINE_CTRL_WORK_MODE,
				(void *)DISPLAY_ENGINE_MODE_DEFAULT, NULL);
	}

	sys_trace_end_call(SYS_TRACE_ID_DMA2D_EN);
}

/**
 * @}
 */

/**
 * @}
 */
