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
        -@-   the rotation configuration using hal_dma2d_config_rotation.
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
      (#) Use function @ref hal_dma2d_register_callback() to register user callbacks,
       xfer_cplt_callback and xfer_error_callback.
      (#) Configure pdata parameter, destination and data length and enable
          the transfer using hal_dma2d_start().
      (#) At the end of data transfer dma2d_device_handler() function is executed and user can
          add his own function by customization of function pointer xfer_cplt_callback (member
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
        -@-   hal_dma2d_rotation_start() function is used if the memory to memory
              with rotation transfer mode is selected.
     (#) To control the DMA2D state, use the following function: hal_dma2d_get_state().
     (#) To read the DMA2D error code, use the following function: hal_dma2d_get_error().
    *** Callback registration ***
    ===================================
    [..]
     (#) Use function @ref hal_dma2d_register_callback() to register a user callback.
     (#) Function @ref hal_dma2d_register_callback() allows to register following callbacks:
           (+) xfer_cplt_callback : callback for transfer complete.
           (+) xfer_error_callback : callback for transfer error.
         This function takes as parameters the HAL peripheral handle, the Callback ID
         and a pointer to the user callback function.
     (#) Use function @ref hal_dma2d_unregister_callback() to reset a callback to the default
         weak (surcharged) function.
         @ref hal_dma2d_unregister_callback() takes as parameters the HAL peripheral handle,
         and the Callback ID.
         This function allows to reset following callbacks:
           (+) xfer_cplt_callback : callback for transfer complete.
           (+) xfer_error_callback : callback for transfer error.
    [..]
     (@) You can refer to the DMA2D HAL driver header file for more useful macros
 @endverbatim
 */

/* Includes ------------------------------------------------------------------*/
#include <string.h>
#include <assert.h>
#include <spicache.h>
#include <display/sw_math.h>
#include <dma2d_hal.h>

/** @defgroup DMA2D  DMA2D
 * @brief DMA2D HAL module driver
 * @{
 */

/* Private types -------------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/** @defgroup DMA2D_Private_Constants DMA2D Private Constants
 * @{
 */

/**
 * @}
 */

/* Private variables ---------------------------------------------------------*/
/* DMA2D global enable bit */
static bool global_en = true;

/* Private constants ---------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
#define IS_DMA2D_LAYER(layer)             (((layer) == HAL_DMA2D_BACKGROUND_LAYER) || ((layer) == HAL_DMA2D_FOREGROUND_LAYER))

#define IS_DMA2D_MODE(mode)               (((mode) == HAL_DMA2D_R2M)          || ((mode) == HAL_DMA2D_M2M) || \
                                           ((mode) == HAL_DMA2D_M2M_BLEND)    || ((mode) == HAL_DMA2D_M2M_BLEND_FG) || \
                                           ((mode) == HAL_DMA2D_M2M_BLEND_BG) || ((mode) == HAL_DMA2D_M2M_ROTATE))

#define IS_DMA2D_LINE(line)               ((line) <= HAL_DMA2D_LINE)
#define IS_DMA2D_PIXEL(pixel)             ((pixel) <= HAL_DMA2D_PIXEL)
#define IS_DMA2D_OFFSET(ooffset)          ((ooffset) <= HAL_DMA2D_OFFSET)

#define IS_DMA2D_ALPHA_MODE(alpha_mode)   (((alpha_mode) == HAL_DMA2D_NO_MODIF_ALPHA) || \
                                           ((alpha_mode) == HAL_DMA2D_REPLACE_ALPHA)  || \
                                           ((alpha_mode) == HAL_DMA2D_COMBINE_ALPHA))

#define IS_DMA2D_RB_SWAP(rb_swap)         (((rb_swap) == HAL_DMA2D_RB_REGULAR) || \
                                           ((rb_swap) == HAL_DMA2D_RB_SWAP))

#define IS_DMA2D_COLOR_MODE(color_mode)    (((color_mode) == HAL_DMA2D_ARGB8888) || \
                                            ((color_mode) == HAL_DMA2D_ARGB6666) || \
                                            ((color_mode) == HAL_DMA2D_RGB888) || \
                                            ((color_mode) == HAL_DMA2D_RGB565) || \
                                            ((color_mode) == HAL_DMA2D_A8) || \
                                            ((color_mode) == HAL_DMA2D_A4) || \
                                            ((color_mode) == HAL_DMA2D_A1) || \
                                            ((color_mode) == HAL_DMA2D_A4_LE) || \
                                            ((color_mode) == HAL_DMA2D_A1_LE) || \
                                            ((color_mode) == HAL_DMA2D_RGB565_LE))

/* Private function prototypes -----------------------------------------------*/
/** @addtogroup DMA2D_Private_Functions DMA2D Private Functions
 * @{
 */
static int dma2d_set_config(hal_dma2d_handle_t *hdma2d, uint32_t src_address1, uint32_t src_address2, uint32_t dst_address, uint16_t width, uint16_t height);
/**
 * @}
 */

/* Private functions ---------------------------------------------------------*/
/**
 * @brief  Get the DMA2D device
 * @retval Pointer to the DMA2D device structure
 */
static const struct device *dma2d_get_device(void)
{
	static const struct device *dma2d_dev;

	if (!dma2d_dev) {
		dma2d_dev = device_get_binding(CONFIG_DISPLAY_ENGINE_DEV_NAME);
	}

	assert(dma2d_dev != NULL);

	return dma2d_dev;
}

/**
 * @brief  Covert the DMA2D ColorMode and rb_swap to display pixel format
 * @retval Pointer to the DMA2D device structure
 */
static int32_t dma2d_get_display_format(uint16_t color_mode, uint16_t rb_swap)
{
	switch (color_mode) {
	case HAL_DMA2D_RGB565:
		return (rb_swap == HAL_DMA2D_RB_REGULAR) ?
				PIXEL_FORMAT_RGB_565 : PIXEL_FORMAT_BGR_565;
	case HAL_DMA2D_ARGB8888:
		return (rb_swap == HAL_DMA2D_RB_REGULAR) ?
				PIXEL_FORMAT_ARGB_8888 : -1;
	case HAL_DMA2D_ARGB6666:
		return (rb_swap == HAL_DMA2D_RB_REGULAR) ?
				PIXEL_FORMAT_ARGB_6666 : PIXEL_FORMAT_ABGR_6666;
	case HAL_DMA2D_RGB565_LE:
		return (rb_swap == HAL_DMA2D_RB_REGULAR) ?
				PIXEL_FORMAT_RGB_565_LE : -1;
	case HAL_DMA2D_RGB888:
		return (rb_swap == HAL_DMA2D_RB_REGULAR) ?
				PIXEL_FORMAT_RGB_888 : -1;
	case HAL_DMA2D_A8:
		return PIXEL_FORMAT_A8;
	case HAL_DMA2D_A4:
		return PIXEL_FORMAT_A4;
	case HAL_DMA2D_A1:
		return PIXEL_FORMAT_A1;
	case HAL_DMA2D_A4_LE:
		return PIXEL_FORMAT_A4_LE;
	case HAL_DMA2D_A1_LE:
		return PIXEL_FORMAT_A1_LE;
	default:
		return -1;
	}
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
 * @param status DMA2D device status
 * @param cmd_seq Current DMA2D device command sequence
 * @param user_data Address of structure hal_dma2d_handle_t
 */
static void dma2d_device_handler(int status, uint16_t cmd_seq, void *user_data)
{
	hal_dma2d_handle_t *hdma2d = user_data;

	if (status != 0) {
		/* Update error code */
		hdma2d->error_code |= HAL_DMA2D_ERROR_TIMEOUT;

		if (hdma2d->xfer_error_callback != NULL) {
			/* Transfer error Callback */
			hdma2d->xfer_error_callback(hdma2d, cmd_seq);
		}
	} else {
		/* Update error code */
		hdma2d->error_code |= HAL_DMA2D_ERROR_NONE;

		if (hdma2d->xfer_cplt_callback != NULL) {
			/* Transfer error Callback */
			hdma2d->xfer_cplt_callback(hdma2d, cmd_seq);
		}
	}

	atomic_dec(&hdma2d->xfer_count);
}

/**
 * @brief  Initialize the DMA2D peripheral and create the associated handle.
 * @param  hdma2d pointer to a hal_dma2d_handle_t structure that contains
 *                 the configuration information for the DMA2D.
 * @retval 0 on success else negative errno code.
 */
int hal_dma2d_init(hal_dma2d_handle_t *hdma2d)
{
	const struct device *dma2d_dev = dma2d_get_device();

	/* Check the DMA2D peripheral existence */
	if (dma2d_dev == NULL) {
		return -ENODEV;
	}

	/* Check the DMA2D peripheral State */
	if (hdma2d == NULL) {
		return -EINVAL;
	}

	/* Open the DMA2D device instance */
	hdma2d->instance = display_engine_open(dma2d_dev, 0);
	if (hdma2d->instance < 0) {
		return -EBUSY;
	}

	/* Register the DMA2D device instance callback */
	display_engine_register_callback(dma2d_dev, hdma2d->instance, dma2d_device_handler, hdma2d);

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
	const struct device *dma2d_dev = dma2d_get_device();

	/* Check the DMA2D peripheral existence */
	if (dma2d_dev == NULL) {
		return -ENODEV;
	}

	/* Check the DMA2D peripheral State */
	if (hdma2d == NULL || hdma2d->instance < 0) {
		return -EINVAL;
	}

	/* Close the DMA2D device instance */
	display_engine_close(dma2d_dev, hdma2d->instance);
	/* Assign invald value */
	hdma2d->instance = -1;

	/* Update error code */
	hdma2d->error_code = HAL_DMA2D_ERROR_NONE;

	return 0;
}

/**
 * @brief  Register a User DMA2D Callback
 *         To be used instead of the weak (surcharged) predefined callback
 * @param hdma2d DMA2D handle
 * @param callback_id ID of the callback to be registered
 *        This parameter can be one of the following values:
 *          @arg @ref HAL_DMA2D_TRANSFERCOMPLETE_CB_ID DMA2D transfer complete Callback ID
 *          @arg @ref HAL_DMA2D_TRANSFERERROR_CB_ID DMA2D transfer error Callback ID
 * @param callback_fn pointer to the callback function
 * @retval 0 on success else negative errno code.
 */
int hal_dma2d_register_callback(hal_dma2d_handle_t *hdma2d, hal_dma2d_callback_e callback_id, hal_dma2d_callback_t callback_fn)
{
	int status = 0;

	if (callback_fn == NULL) {
		/* Update the error code */
		hdma2d->error_code |= HAL_DMA2D_ERROR_INVALID_CALLBACK;
		return -EINVAL;
	}

	if (HAL_DMA2D_STATE_READY == hal_dma2d_get_state(hdma2d)) {
		switch (callback_id) {
		case HAL_DMA2D_TRANSFERCOMPLETE_CB_ID :
			hdma2d->xfer_cplt_callback = callback_fn;
			break;
		case HAL_DMA2D_TRANSFERERROR_CB_ID :
			hdma2d->xfer_error_callback = callback_fn;
			break;
		default :
			/* Update the error code */
			hdma2d->error_code |= HAL_DMA2D_ERROR_INVALID_CALLBACK;
			/* update return status */
			status =  -EINVAL;
			break;
		}
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
 * @param callback_id ID of the callback to be unregistered
 *        This parameter can be one of the following values:
 *          @arg @ref HAL_DMA2D_TRANSFERCOMPLETE_CB_ID DMA2D transfer complete Callback ID
 *          @arg @ref HAL_DMA2D_TRANSFERERROR_CB_ID DMA2D transfer error Callback ID
 * @retval 0 on success else negative errno code.
 */
int hal_dma2d_unregister_callback(hal_dma2d_handle_t *hdma2d, hal_dma2d_callback_e callback_id)
{
	int status = 0;

	if (HAL_DMA2D_STATE_READY == hal_dma2d_get_state(hdma2d)) {
		switch (callback_id) {
		case HAL_DMA2D_TRANSFERCOMPLETE_CB_ID :
			hdma2d->xfer_cplt_callback = NULL;
			break;
		case HAL_DMA2D_TRANSFERERROR_CB_ID :
			hdma2d->xfer_error_callback = NULL;
			break;
		default :
			/* Update the error code */
			hdma2d->error_code |= HAL_DMA2D_ERROR_INVALID_CALLBACK;
			/* update return status */
			status =  -EINVAL;
			break;
		}
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
int hal_dma2d_start(hal_dma2d_handle_t *hdma2d, uint32_t pdata, uint32_t dst_address, uint16_t width,  uint16_t height)
{
	return dma2d_set_config(hdma2d, pdata, 0, dst_address, width, height);
}

/**
 * @brief  Start the multi-source DMA2D Transfer.
 * @param  hdma2d      Pointer to a hal_dma2d_handle_t structure that contains
 *                      the configuration information for the DMA2D.
 * @param  src_address1 The source memory Buffer address for the foreground layer.
 * @param  src_address2 The source memory Buffer address for the background layer.
 * @param  dst_address  The destination memory Buffer address.
 * @param  width       The width of data to be transferred from source to destination (expressed in number of pixels per line).
 * @param  height      The height of data to be transferred from source to destination (expressed in number of lines).
 * @retval command sequence (uint16_t) on success else negative errno code.
 */
int hal_dma2d_blending_start(hal_dma2d_handle_t *hdma2d, uint32_t src_address1, uint32_t  src_address2, uint32_t dst_address, uint16_t width,  uint16_t height)
{
	return dma2d_set_config(hdma2d, src_address1, src_address2, dst_address, width, height);
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
 * @param  start_line  The start line to rotate.
 * @param  num_lines   Number of lines to rotate.
 * @retval command sequence (uint16_t) on success else negative errno code.
 */
int hal_dma2d_rotation_start(hal_dma2d_handle_t *hdma2d, uint32_t src_address, uint32_t dst_address, uint16_t start_line, uint16_t num_lines)
{
	const struct device *dma2d_dev = dma2d_get_device();
	hal_dma2d_output_cfg_t *ouput_cfg = &hdma2d->output_cfg;
	hal_dma2d_rotation_cfg_t *rot_cfg = &hdma2d->rotation_cfg;
	display_engine_rotation_t *hw_cfg = &rot_cfg->hw_cfg;
	uint8_t bytes_per_pixel;
	int res = 0;

	/* Check DMAD enabled */
	if (global_en == false) {
		return -EBUSY;
	}

	/* Check the peripheral existence */
	if (dma2d_dev == NULL) {
		return -ENODEV;
	}

	if (rot_cfg->color_mode != ouput_cfg->color_mode ||
		rot_cfg->rb_swap != ouput_cfg->rb_swap) {
		return -EINVAL;
	}

	if (start_line + num_lines > hw_cfg->outer_diameter) {
		return -EINVAL;
	}

	if (buf_is_psram(src_address)) {
		src_address = (uint32_t)cache_to_uncache((void *)src_address);
	}

	if (buf_is_psram(dst_address)) {
		dst_address = (uint32_t)cache_to_uncache((void *)dst_address);
	}

	bytes_per_pixel = display_format_get_bits_per_pixel(hw_cfg->pixel_format) / 8;
	hw_cfg->src_pitch = (hw_cfg->outer_diameter + rot_cfg->input_offset) * bytes_per_pixel;
	hw_cfg->dst_pitch = (hw_cfg->outer_diameter + ouput_cfg->output_offset) * bytes_per_pixel;
	hw_cfg->dst_address = dst_address;

	/* Always compute the variables for hardware parallelism */
	/* if (start_line == 0 || start_line != hw_cfg->line_end) */ {
		hw_cfg->dst_dist_sq = (hw_cfg->outer_diameter - 1) * (hw_cfg->outer_diameter - 1) +
				(hw_cfg->outer_diameter - 1 - 2 * start_line) * (hw_cfg->outer_diameter - 1 - 2 * start_line);
		hw_cfg->src_coord_x = rot_cfg->src_coord_x0 + start_line * hw_cfg->src_coord_dx_ay;
		hw_cfg->src_coord_y = rot_cfg->src_coord_y0 + start_line * hw_cfg->src_coord_dy_ay;
		hw_cfg->src_address = src_address +
				FLOOR_FIXEDPOINT12(hw_cfg->src_coord_y) * hw_cfg->src_pitch +
				FLOOR_FIXEDPOINT12(hw_cfg->src_coord_x) * bytes_per_pixel;
	}

	hw_cfg->line_start = start_line;
	hw_cfg->line_end = start_line + num_lines;

	res = display_engine_rotate(dma2d_dev, hdma2d->instance, hw_cfg);
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
 	const struct device *dma2d_dev = dma2d_get_device();
 	int status = 0;

	/* Check the peripheral existence */
	if (dma2d_dev == NULL) {
		return -ENODEV;
	}

	if (HAL_DMA2D_STATE_BUSY != hal_dma2d_get_state(hdma2d)) {
		return 0;
	}

	if (display_engine_poll(dma2d_dev, hdma2d->instance, timeout)) {
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
int hal_dma2d_config_output(hal_dma2d_handle_t *hdma2d)
{
#if 0 /* These will be checked in dma2d_set_config(), so skip it here */
	const struct device *dma2d_dev = dma2d_get_device();

	/* Check the peripheral existence */
	if (dma2d_dev == NULL) {
		return -ENODEV;
	}

	/* Check the parameters */
	assert(IS_DMA2D_MODE(hdma2d->output_cfg.mode));
	assert(IS_DMA2D_OFFSET(hdma2d->output_cfg.output_offset));
	assert(IS_DMA2D_RB_SWAP(hdma2d->output_cfg.rb_swap));
	assert(IS_DMA2D_COLOR_MODE(hdma2d->output_cfg.color_mode));

	int32_t display_format = dma2d_get_display_format(hdma2d->output_cfg.color_mode, hdma2d->output_cfg.rb_swap);
	if (display_format < 0) {
		return -EINVAL;
	}

	struct display_engine_capabilities capabilities;
	display_engine_get_capabilities(dma2d_dev, &capabilities);
	if (hdma2d->output_cfg.mode == HAL_DMA2D_M2M_ROTATE) {
		if ((capabilities.supported_rotate_pixel_formats & display_format) == 0) {
			return -ENOTSUP;
		}
	} else {
		if ((capabilities.supported_output_pixel_formats & display_format) == 0) {
			return -ENOTSUP;
		}
		if (hdma2d->output_cfg.mode == HAL_DMA2D_M2M_BLEND_FG && capabilities.support_blend_fg == 0) {
			return -ENOTSUP;
		}
		if (hdma2d->output_cfg.mode == HAL_DMA2D_M2M_BLEND_BG && capabilities.support_blend_bg == 0) {
			return -ENOTSUP;
		}
	}
#endif

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
int hal_dma2d_config_layer(hal_dma2d_handle_t *hdma2d, uint16_t layer_idx)
{
	hal_dma2d_layer_cfg_t *cfg = &hdma2d->layer_cfg[layer_idx];

#if 0 /* These will be checked in dma2d_set_config(), so skip it here */
	const struct device *dma2d_dev = dma2d_get_device();

	/* Check the peripheral existence */
	if (dma2d_dev == NULL) {
		return -ENODEV;
	}

	/* Check the parameters */
	assert(IS_DMA2D_LAYER(layer_idx));
	assert(IS_DMA2D_OFFSET(cfg->input_offset));
	assert(IS_DMA2D_COLOR_MODE(cfg->color_mode));

	int32_t display_format = dma2d_get_display_format(cfg->color_mode, cfg->rb_swap);
	if (display_format < 0) {
		return -EINVAL;
	}

	struct display_engine_capabilities capabilities;
	display_engine_get_capabilities(dma2d_dev, &capabilities);
	if ((capabilities.supported_input_pixel_formats & display_format) == 0) {
		return -ENOTSUP;
	}
#endif

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
int hal_dma2d_config_rotation(hal_dma2d_handle_t *hdma2d)
{
	hal_dma2d_rotation_cfg_t *cfg = &hdma2d->rotation_cfg;
	display_engine_rotation_t *hw_cfg = &cfg->hw_cfg;
	uint16_t revert_angle;
	int32_t center;

	/* Check the parameters */
	assert(IS_DMA2D_OFFSET(cfg->input_offset));

	if (cfg->angle >= 3600 || cfg->outer_diameter <= 0 ||
		cfg->inner_diameter >= cfg->outer_diameter) {
		return -EINVAL;
	}

	hw_cfg->pixel_format = dma2d_get_display_format(cfg->color_mode, cfg->rb_swap);
	hw_cfg->outer_diameter = cfg->outer_diameter;
	hw_cfg->outer_radius_sq = (cfg->outer_diameter - 1) * (cfg->outer_diameter - 1);
	hw_cfg->inner_radius_sq = cfg->inner_diameter > 0 ?
			(cfg->inner_diameter - 1) * (cfg->inner_diameter - 1) : 0;
	hw_cfg->fill_color.full = cfg->fill_color;
	hw_cfg->fill_enable = cfg->fill_enable;
	hw_cfg->line_start = 0;
	hw_cfg->line_end = 0;

	center = FIXEDPOINT12(hw_cfg->outer_diameter) / 2;
	revert_angle = 3600 - cfg->angle;

	/* coordinates in the destination coordinate system */
	cfg->src_coord_x0 = PX_FIXEDPOINT12(0);
	cfg->src_coord_y0 = PX_FIXEDPOINT12(0);
	hw_cfg->src_coord_dx_ax = FIXEDPOINT12(1);
	hw_cfg->src_coord_dy_ax = FIXEDPOINT12(0);
	hw_cfg->src_coord_dx_ay = FIXEDPOINT12(0);
	hw_cfg->src_coord_dy_ay = FIXEDPOINT12(1);

	/* rotate back to the source coordinate system */
	sw_rotate_point32(&cfg->src_coord_x0, &cfg->src_coord_y0,
			cfg->src_coord_x0, cfg->src_coord_y0, center,
			center, revert_angle);
	sw_rotate_point32(&hw_cfg->src_coord_dx_ax, &hw_cfg->src_coord_dy_ax,
			hw_cfg->src_coord_dx_ax, hw_cfg->src_coord_dy_ax,
			FIXEDPOINT12(0), FIXEDPOINT12(0), revert_angle);
	sw_rotate_point32(&hw_cfg->src_coord_dx_ay, &hw_cfg->src_coord_dy_ay,
			hw_cfg->src_coord_dx_ay, hw_cfg->src_coord_dy_ay,
			FIXEDPOINT12(0), FIXEDPOINT12(0), revert_angle);

	/* move to the source pixel coordinate system */
	cfg->src_coord_x0 -= PX_FIXEDPOINT12(0);
	cfg->src_coord_y0 -= PX_FIXEDPOINT12(0);

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

static void dma2d_set_dst_buffer(hal_dma2d_handle_t *hdma2d, display_buffer_t *dst, uint32_t dst_address, uint16_t width, uint16_t height)
{
	dst->desc.width = width;
	dst->desc.height = height;
	dst->desc.pixel_format = dma2d_get_display_format(hdma2d->output_cfg.color_mode, hdma2d->output_cfg.rb_swap);
	dst->desc.pitch = width + hdma2d->output_cfg.output_offset;
	dst->addr = dst_address;
}

static void dma2d_set_src_buffer(hal_dma2d_handle_t *hdma2d, display_buffer_t *src, uint16_t layer_idx, uint32_t src_address, uint16_t width, uint16_t height)
{
	src->desc.width = width;
	src->desc.height = height;
	src->desc.pixel_format = dma2d_get_display_format(hdma2d->layer_cfg[layer_idx].color_mode, hdma2d->layer_cfg[layer_idx].rb_swap),
	src->desc.pitch = width + hdma2d->layer_cfg[layer_idx].input_offset;
	src->addr = src_address;
}

/**
 * @brief  Set the DMA2D transfer parameters.
 * @param  hdma2d     Pointer to a hal_dma2d_handle_t structure that contains
 *                     the configuration information for the specified DMA2D.
 * @param  src_address1 The source memory Buffer address for the foreground layer.
 * @param  src_address2 The source memory Buffer address for the background layer.
 * @param  dst_address The destination memory Buffer address.
 * @param  width      The width of data to be transferred from source to destination.
 * @param  height     The height of data to be transferred from source to destination.
 * @retval command sequence (uint16_t) on success else negative errno code.
 */
static int dma2d_set_config(hal_dma2d_handle_t *hdma2d, uint32_t src_address1, uint32_t src_address2, uint32_t dst_address, uint16_t width, uint16_t height)
{
	const struct device *dma2d_dev = dma2d_get_device();
	display_buffer_t dst;
	display_buffer_t fg;
	display_color_t fg_color;
	display_buffer_t bg;
	display_color_t bg_color;
	int res = 0;

	/* Check DMAD enabled */
	if (global_en == false) {
		return -EBUSY;
	}

	/* Check the peripheral existence */
	if (dma2d_dev == NULL) {
		return -ENODEV;
	}

	/* Check the parameters */
	assert(IS_DMA2D_LINE(height));
	assert(IS_DMA2D_PIXEL(width));

	dma2d_set_dst_buffer(hdma2d, &dst, dst_address, width, height);

	fg_color.full = hdma2d->layer_cfg[HAL_DMA2D_FOREGROUND_LAYER].input_alpha; /* ARGB8888 */
	bg_color.full = hdma2d->layer_cfg[HAL_DMA2D_BACKGROUND_LAYER].input_alpha; /* ARGB8888 */

	switch (hdma2d->output_cfg.mode) {
	case HAL_DMA2D_R2M:
		fg_color.full = src_address1;
		res = display_engine_fill(dma2d_dev, hdma2d->instance, &dst, fg_color);
		break;
	case HAL_DMA2D_M2M:
		dma2d_set_src_buffer(hdma2d, &fg, HAL_DMA2D_FOREGROUND_LAYER, src_address1, width, height);
		res = display_engine_blit(dma2d_dev, hdma2d->instance, &dst, &fg, fg_color);
		break;
	case HAL_DMA2D_M2M_BLEND:
		dma2d_set_src_buffer(hdma2d, &fg, HAL_DMA2D_FOREGROUND_LAYER, src_address1, width, height);
		dma2d_set_src_buffer(hdma2d, &bg, HAL_DMA2D_BACKGROUND_LAYER, src_address2, width, height);
		res = display_engine_blend(dma2d_dev, hdma2d->instance, &dst, &fg, fg_color, &bg, bg_color);
		break;
	case HAL_DMA2D_M2M_BLEND_FG:
		fg_color.full = src_address1;
		dma2d_set_src_buffer(hdma2d, &bg, HAL_DMA2D_BACKGROUND_LAYER, src_address2, width, height);
		res = display_engine_blend(dma2d_dev, hdma2d->instance, &dst, NULL, fg_color, &bg, bg_color);
		break;
	case HAL_DMA2D_M2M_BLEND_BG:
		bg_color.full = src_address2;
		dma2d_set_src_buffer(hdma2d, &fg, HAL_DMA2D_FOREGROUND_LAYER, src_address1, width, height);
		res = display_engine_blend(dma2d_dev, hdma2d->instance, &dst, &fg, fg_color, NULL, bg_color);
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
	global_en = enabled;
}

/**
 * @}
 */

/**
 * @}
 */
