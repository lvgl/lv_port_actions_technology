/*
 * Copyright (c) 2020, Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief DMA2D Public API
 */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef FRAMEWORK_DMA2D_HAL_H_
#define FRAMEWORK_DMA2D_HAL_H_

#include <zephyr.h>
#include <sys/atomic.h>
#include <drivers/display/display_engine.h>
#include <display/display_hal.h>

/**
 * @brief Display DMA2D Interface
 * @defgroup display_dma2d_interface Display DMA2D Interface
 * @ingroup display_libraries
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

#define HAL_DMA2D_MAX_LAYER  2U  /*!< DMA2D maximum number of layers */
#define HAL_DMA2D_SCALE_NONE 256U  /*!< DMA2D maximum number of layers */

/**
 * @brief DMA2D output structure definition
 */
typedef struct {
	uint32_t color_format; /*!< Configures the color format of the output image.
	                             This parameter can be one value of @ref HAL_PIXEL_FORMAT_*. */
	uint32_t output_pitch; /*!< Specifies the output pitch value.
	                             This parameter must be a number between Min_Data = 0x0000 and Max_Data = 0x01FF. */
	uint16_t mode;          /*!< Configures the DMA2D transfer mode.
	                             This parameter can be one value of @ref DMA2D_MODE. */
} hal_dma2d_output_cfg_t;

/**
 * @brief DMA2D Layer structure definition
 */
typedef struct {
	uint32_t color_format; /*!< Configures the DMA2D foreground or background color format.
	                              This parameter can be one value of @ref HAL_PIXEL_FORMAT_*. */
	uint32_t input_width;  /*!< Configures the DMA2D foreground or background width.
	                              This parameter must be a number between Min_Data = 0x0000 and Max_Data = 0x3FFF. */
	uint32_t input_height; /*!< Configures the DMA2D foreground or background height.
	                              This parameter must be a number between Min_Data = 0x0000 and Max_Data = 0x3FFF. */
	uint32_t input_pitch;  /*!< Configures the DMA2D foreground or background pitch.
	                              This parameter must be a number between Min_Data = 0x0000 and Max_Data = 0x3FFF. */
	uint8_t input_xofs;    /*!< Configures the DMA2D foreground or background x offset per row.
	                              This parameter is only for A1/2/4 and I1/2/4 color formats, accepted values:
	                              0, 1 for A4, I4
	                              0, 1, 2, 3 for A2, I2
	                              0, 1, 2, 3, 4, 5, 6, 7 for A1, I1
	                              0 for others */
	uint8_t alpha_mode;    /*!< Configures the DMA2D foreground or background alpha mode.
	                              This parameter can be one value of @ref DMA2D_ALPHA_Mode. */
	uint32_t input_alpha;  /*!< Specifies the DMA2D foreground or background alpha value and color value both of Ax color formats and rotation fill-color.
	                              This parameter must be a number between Min_Data = 0x00 and Max_Data = 0xFF except for the color formats detailed below.
	                              @note In case of A8 or A4 color mode (ARGB), this parameter must be a number between
	                              Min_Data = 0x00000000 and Max_Data = 0xFFFFFFFF where
	                              - input_alpha[24:31] is the alpha value ALPHA[0:7]
	                              - input_alpha[16:23] is the red value RED[0:7]
	                              - input_alpha[8:15] is the green value GREEN[0:7]
	                              - input_alpha[0:7] is the blue value BLUE[0:7]. */
} hal_dma2d_layer_cfg_t;

/**
 * @brief DMA2D Transform structure definition
 */
typedef struct {
	uint16_t mode;          /*!< Transform mode, This parameter can be one value of @ref DMA2D_Transform_Mode:
	                             - HAL_DMA2D_ROT2D  Similarity transfrom, can set any scale and rotation
								 - Others  predefined transform, whose top-left corner after transformation
								   keep the same as before (image_x0, image_y0) */
	uint16_t angle;         /*!< Anti-clockwise rotation angle in 0.1 degree [0, 3600) in the display coordinate */
	int16_t image_x0;       /*!< Coord X of top-left corner of image src before rotation in the display coordinate */
	int16_t image_y0;       /*!< Coord Y of top-left corner of image src before rotation in the display coordinate */

	union {
		struct {
			uint16_t outer_diameter;  /*!< Outer Diameter in pixels of the ring area of the Source, and must equal to the source image width and height */
			uint16_t inner_diameter;  /*!< Inner Diameter in pixels of the ring area of the Source */
		} circle;

		struct {
			int16_t pivot_x; /*!< Rotate/scale pivot X offset relative to top-left corner of the Image Source */
			int16_t pivot_y; /*!< Rotate/scale pivot Y offset relative to top-left corner of the Image Source  */
			/*!< Scale factor in fixedpoint, equal to (HAL_DMA2D_SCALE_NONE * dest / src)
			 * > HAL_DMA2D_SCALE_NONE, scaling up
			 * < HAL_DMA2D_SCALE_NONE, scaling down
			 * = HAL_DMA2D_SCALE_NONE, no scaling
			 * only scaling up supported so far
			 */
			uint16_t scale_x; /*!< scale factor X */
			uint16_t scale_y; /*!< scale factor Y */
		} rect;
	};

	display_engine_transform_param_t hw_param;
} hal_dma2d_transform_cfg_t;

/**
 * @brief  HAL DMA2D State structures definition
 */
typedef enum {
	HAL_DMA2D_STATE_RESET    = 0x00U, /*!< DMA2D not yet initialized or disabled       */
	HAL_DMA2D_STATE_READY    = 0x01U, /*!< Peripheral Initialized and ready for use    */
	HAL_DMA2D_STATE_BUSY     = 0x02U, /*!< An internal process is ongoing              */
	HAL_DMA2D_STATE_TIMEOUT  = 0x03U, /*!< timeout state                               */
	HAL_DMA2D_STATE_ERROR    = 0x04U, /*!< DMA2D state error                           */
} hal_dma2d_state_e;

struct _hal_dma2d_handle;

/**
 * @brief  HAL DMA2D Callback pointer definition provided the current command sequence and the error code
 */
typedef void (* hal_dma2d_callback_t)(struct _hal_dma2d_handle * hdma2d, uint16_t cmd_seq, uint32_t error_code);

/**
 * @brief  DMA2D handle Structure definition
 */
typedef struct _hal_dma2d_handle {
	const void *device;                   /*!< DMA2D Device Handle */
	int instance;                         /*!< DMA2D Device Instance ID */

	atomic_t xfer_count;                  /*!< DMA2D pending transfer count */
	hal_dma2d_callback_t xfer_callback;   /*!< DMA2D transfer callback. */

	hal_dma2d_output_cfg_t output_cfg;                       /*!< DMA2D output parameters. */
	hal_dma2d_layer_cfg_t  layer_cfg[HAL_DMA2D_MAX_LAYER];   /*!< DMA2D Layers parameters */
	hal_dma2d_transform_cfg_t trans_cfg;                     /*!< DMA2D Transform parameters */

	uint32_t           error_code;                           /*!< DMA2D error code. */
} hal_dma2d_handle_t;

/**
 * @brief HAL DMA2D_Layers DMA2D Layers
 */
#define HAL_DMA2D_BACKGROUND_LAYER  0x0000U  /*!< DMA2D Background Layer (layer 0) */
#define HAL_DMA2D_FOREGROUND_LAYER  0x0001U  /*!< DMA2D Foreground Layer (layer 1) */

/**
 * @brief HAL DMA2D_Offset DMA2D Offset
 */
#define HAL_DMA2D_PITCH             2048U     /*!< maximum Line Offset */

/**
 * @brief HAL DMA2D_Size DMA2D Size
 */
#define HAL_DMA2D_PIXEL             UINT32_MAX /*!< DMA2D maximum number of pixels per line */
#define HAL_DMA2D_LINE              UINT16_MAX /*!< DMA2D maximum number of lines           */

/**
 * @brief HAL DMA2D_Error_Code DMA2D Error Code
 */
#define HAL_DMA2D_ERROR_NONE        0x0000U  /*!< No error             */
#define HAL_DMA2D_ERROR_TE          0x0001U  /*!< Transfer error       */
#define HAL_DMA2D_ERROR_CE          0x0002U  /*!< Configuration error  */
#define HAL_DMA2D_ERROR_TIMEOUT     0x0020U  /*!< Timeout error        */
#define HAL_DMA2D_ERROR_INVALID_CALLBACK 0x0040U  /*!< Invalid callback error  */

/**
 * @brief HAL DMA2D_MODE DMA2D Mode
 */
#define HAL_DMA2D_R2M                   0x0001U  /*!< DMA2D register to memory transfer mode */
#define HAL_DMA2D_M2M                   0x0002U  /*!< DMA2D memory to memory transfer mode, optionally with pixel format conversion */
#define HAL_DMA2D_M2M_BLEND             0x0004U  /*!< DMA2D memory to memory with blending transfer mode */
#define HAL_DMA2D_M2M_BLEND_FG          0x0008U  /*!< DMA2D memory to memory with blending transfer mode and fixed color FG */
#define HAL_DMA2D_M2M_BLEND_BG          0x0010U  /*!< DMA2D memory to memory with blending transfer mode and fixed color BG */
#define HAL_DMA2D_M2M_TRANSFORM         0x0020U  /*!< DMA2D memory to memory with transform transfer mode */
#define HAL_DMA2D_M2M_TRANSFORM_BLEND   0x0040U  /*!< DMA2D memory to memory with transform and blending transfer mode */
#define HAL_DMA2D_M2M_TRANSFORM_CIRCLE  0x0080U  /*!< DMA2D memory to memory with transform circle transfer mode */

#define HAL_DMA2D_LITE_MODES \
	(HAL_DMA2D_R2M | HAL_DMA2D_M2M)
#define HAL_DMA2D_FULL_MODES \
	(HAL_DMA2D_R2M | HAL_DMA2D_M2M | HAL_DMA2D_M2M_BLEND | HAL_DMA2D_M2M_BLEND_FG | HAL_DMA2D_M2M_BLEND_BG | \
	 HAL_DMA2D_M2M_TRANSFORM | HAL_DMA2D_M2M_TRANSFORM_BLEND | HAL_DMA2D_M2M_TRANSFORM_CIRCLE)

/**
 * @brief HAL DMA2D_Alpha_Mode DMA2D Alpha Mode
 */
#define HAL_DMA2D_NO_MODIF_ALPHA        0x0000U  /*!< No modification of the alpha channel value */
#define HAL_DMA2D_REPLACE_ALPHA         0x0001U  /*!< Replace original alpha channel value by programmed alpha value */
#define HAL_DMA2D_COMBINE_ALPHA         0x0002U  /*!< Replace original alpha channel value by programmed alpha value
                                                  with original alpha channel value                              */

/**
 * @brief HAL DMA2D_Transform_Mode DMA2D Transform Mode
 */
#define HAL_DMA2D_ROT2D          0x0000U  /*!< Similarity transfrom, can set any scale and rotation */
#define HAL_DMA2D_FLIP_H         0x0001U  /*!< Flip source image horizontally */
#define HAL_DMA2D_FLIP_V         0x0002U  /*!< Flip source image verticallye */
#define HAL_DMA2D_ROT_90         0x0004U  /*!< Rotate source image 90 degrees clock-wise */
#define HAL_DMA2D_ROT_180        0x0003U  /*!< Rotate source image 180 degrees */
#define HAL_DMA2D_ROT_270        0x0007U  /*!< Rotate source image 270 degrees clock-wise */

/* Exported functions --------------------------------------------------------*/

/* Initialization and de-initialization functions *******************************/
/**
 * @brief  Initialize the DMA2D peripheral and create the associated handle.
 * @param  hdma2d pointer to a hal_dma2d_handle_t structure that contains
 *                the configuration information for the DMA2D.
 * @param preferred_modes "bitwise or" of output modes that maybe used.
 *        Different hardware device may be chosen as accelerator according this.
 * @retval 0 on success else negative errno code.
 */
int hal_dma2d_init(hal_dma2d_handle_t *hdma2d, uint32_t preferred_modes);

/**
 * @brief  Deinitializes the DMA2D peripheral registers to their default reset
 *         values.
 * @param  hdma2d pointer to a hal_dma2d_handle_t structure that contains
 *                 the configuration information for the DMA2D.
 * @retval 0 on success else negative errno code.
 */
int hal_dma2d_deinit(hal_dma2d_handle_t *hdma2d);

/* Callbacks Register/UnRegister functions  ***********************************/
/**
 * @brief  Register a User DMA2D Callback
 *         To be used instead of the weak (surcharged) predefined callback
 * @param hdma2d DMA2D handle
 * @param callback_fn pointer to the callback function
 * @retval 0 on success else negative errno code.
 */
int hal_dma2d_register_callback(hal_dma2d_handle_t *hdma2d, hal_dma2d_callback_t callback_fn);

/**
 * @brief  Unregister a DMA2D Callback
 *         DMA2D Callback is redirected to the weak (surcharged) predefined callback
 * @param hdma2d DMA2D handle
 * @retval 0 on success else negative errno code.
 */
int hal_dma2d_unregister_callback(hal_dma2d_handle_t *hdma2d);

/* IO operation functions *******************************************************/
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
int hal_dma2d_start(hal_dma2d_handle_t *hdma2d, uint32_t pdata, uint32_t dst_address, uint32_t width, uint32_t height);

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
int hal_dma2d_blending_start(hal_dma2d_handle_t *hdma2d, uint32_t fg_address, uint32_t bg_address,
		uint32_t dst_address, uint32_t width, uint32_t height);

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
 * @param  x        X coord of top-left corner of dest area in the display coordinate.
 * @param  y        Y coord of top-left corner of dest area in the display coordinate.
 * @param  width    Width of dest area.
 * @param  height   Height of dest area.
 * @retval command sequence (uint16_t) on success else negative errno code.
 */
int hal_dma2d_transform_start(hal_dma2d_handle_t *hdma2d, uint32_t src_address, uint32_t dst_address,
		int16_t x, int16_t y, uint16_t width, uint16_t height);

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
int hal_dma2d_clut_load_start(hal_dma2d_handle_t *hdma2d, uint16_t layer_idx, uint16_t size, const uint32_t *clut);

/**
 * @brief  Polling for transfer complete.
 * @param  hdma2d Pointer to a hal_dma2d_handle_t structure that contains
 *                 the configuration information for the DMA2D.
 * @param  timeout timeout duration in milliseconds, if negative, means wait forever
 * @retval 0 on success else negative errno code.
 */
int hal_dma2d_poll_transfer(hal_dma2d_handle_t *hdma2d, int32_t timeout);

/* Peripheral Control functions *************************************************/
/**
 * @brief  Configure the DMA2D transfer mode and output according to the
 *         specified parameters in the hal_dma2d_handle_t.
 * @param  hdma2d Pointer to a hal_dma2d_handle_t structure that contains
 *                 the configuration information for the DMA2D.
 * @retval 0 on success else negative errno code.
 */
int hal_dma2d_config_output(hal_dma2d_handle_t *hdma2d);

/**
 * @brief  Configure the DMA2D Layer according to the specified
 *         parameters in the hal_dma2d_handle_t.
 *
 * hal_dma2d_config_output() must be invoked to configure the correct mode first.
 *
 * @param  hdma2d Pointer to a hal_dma2d_handle_t structure that contains
 *                 the configuration information for the DMA2D.
 * @param  layer_idx DMA2D Layer index.
 *                   This parameter can be one of the following values:
 *                   HAL_DMA2D_BACKGROUND_LAYER(0) / HAL_DMA2D_FOREGROUND_LAYER(1)
 * @retval 0 on success else negative errno code.
 */
int hal_dma2d_config_layer(hal_dma2d_handle_t *hdma2d, uint16_t layer_idx);

/**
 * @brief  Configure the DMA2D Rotation according to the specified
 *         parameters in the hal_dma2d_handle_t.
 *
 * hal_dma2d_config_output() must be invoked to configure correct mode first.
 *
 * @param  hdma2d Pointer to a hal_dma2d_handle_t structure that contains
 *                 the configuration information for the DMA2D.
 * @retval 0 on success else negative errno code.
 */
int hal_dma2d_config_transform(hal_dma2d_handle_t *hdma2d);

/* Peripheral State functions ***************************************************/
/**
 * @brief  Return the DMA2D state
 * @param  hdma2d pointer to a hal_dma2d_handle_t structure that contains
 *                 the configuration information for the DMA2D.
 * @retval HAL state
 */
hal_dma2d_state_e hal_dma2d_get_state(hal_dma2d_handle_t *hdma2d);

/**
 * @brief  Return the DMA2D error code
 * @param  hdma2d  pointer to a hal_dma2d_handle_t structure that contains
 *               the configuration information for DMA2D.
 * @retval DMA2D Error Code
 */
uint32_t hal_dma2d_get_error(hal_dma2d_handle_t *hdma2d);

/**
* @cond INTERNAL_HIDDEN
*/

/**
 * @brief  Global enable/disable DMAD functions
 * @param  enabled enable or not
 * @retval N/A
 */
void hal_dma2d_set_global_enabled(bool enabled);

/**
* INTERNAL_HIDDEN @endcond
*/

#ifdef __cplusplus
}
#endif
/**
 * @}
 */

#endif /* FRAMEWORK_DMA2D_HAL_H_ */
