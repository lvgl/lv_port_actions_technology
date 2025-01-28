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

/**
 * @brief DMA2D output structure definition
 */
typedef struct {
	uint16_t mode;          /*!< Configures the DMA2D transfer mode.
	                             This parameter can be one value of @ref DMA2D_MODE. */
	uint16_t output_offset; /*!< Specifies the output offset value.
	                             This parameter must be a number between Min_Data = 0x0000 and Max_Data = 0x01FF. */
	uint16_t color_mode;    /*!< Configures the color format of the output image.
	                             This parameter can be one value of @ref DMA2D_COLOR_MODE. */
	uint16_t rb_swap;       /*!< Select regular mode (RGB or ARGB) or swap mode (BGR or ABGR)
	                             for the output pixel format converter.
	                             his parameter can be one value of @ref HAL_DMA2D_RB_SWAP. */
} hal_dma2d_output_cfg_t;


/**
 * @brief DMA2D Layer structure definition
 */
typedef struct {
	uint16_t input_offset;  /*!< Configures the DMA2D foreground or background offset.
	                              This parameter must be a number between Min_Data = 0x0000 and Max_Data = 0x3FFF. */
	uint16_t color_mode;    /*!< Configures the DMA2D foreground or background color mode.
	                              This parameter can be one value of @ref DMA2D_COLOR_MODE. */
	uint16_t rb_swap;       /*!< Select regular mode (RGB or ARGB) or swap mode (BGR or ABGR).
	                              This parameter can be one value of @ref HAL_DMA2D_RB_SWAP. */
	uint16_t alpha_mode;    /*!< Configures the DMA2D foreground or background alpha mode.
	                              This parameter can be one value of @ref DMA2D_ALPHA_Mode. */
	uint32_t input_alpha;   /*!< Specifies the DMA2D foreground or background alpha value and color value in case of A8 or A4 color mode.
	                              This parameter must be a number between Min_Data = 0x00 and Max_Data = 0xFF except for the color modes detailed below.
	                              @note In case of A8 or A4 color mode (ARGB), this parameter must be a number between
	                              Min_Data = 0x00000000 and Max_Data = 0xFFFFFFFF where
	                              - input_alpha[24:31] is the alpha value ALPHA[0:7]
	                              - input_alpha[16:23] is the red value RED[0:7]
	                              - input_alpha[8:15] is the green value GREEN[0:7]
	                              - input_alpha[0:7] is the blue value BLUE[0:7]. */
} hal_dma2d_layer_cfg_t;

/**
 * @brief DMA2D Rotation structure definition
 */
typedef struct {
	uint16_t input_offset;    /*!< Configures the DMA2D rotation input offset.
	                              This parameter must be a number between Min_Data = 0x0000 and Max_Data = 0x3FFF. */
	uint16_t color_mode;      /*!< Configures the DMA2D rotation input color mode.
	                              This parameter can be one value of @ref DMA2D_COLOR_MODE. */
	uint16_t rb_swap;         /*!< Select regular mode (RGB or ARGB) or swap mode (BGR or ABGR).
	                              This parameter can be one value of @ref HAL_DMA2D_RB_SWAP. */

	uint16_t outer_diameter;  /*!< Outer Diameter in pixels of the ring area of the Source, and must equal to the source image width and height */
	uint16_t inner_diameter;  /*!< Inner Diameter in pixels of the ring area of the Source */
	uint16_t angle;           /*!< Rotate angle in 0.1 degree [0, 3600] */

	uint16_t fill_enable;     /*!< Enable filling color outside the ring area */
	uint32_t fill_color;      /*!< Fill color (ARGB8888) outside the ring area */

	/* Private parameters computed in HAL implementation and passed to device driver */
	int32_t src_coord_x0;     /* src X coord in .12 fixedpoint mapping to dest coord (0, 0) */
	int32_t src_coord_y0;     /* src Y coord in .12 fixedpoint mapping to dest coord (0, 0) */
	display_engine_rotation_t hw_cfg;
} hal_dma2d_rotation_cfg_t;

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
 * @brief  HAL DMA2D Callback pointer definition provided the current command sequence
 */
typedef void (* hal_dma2d_callback_t)(struct _hal_dma2d_handle * hdma2d, uint16_t cmd_seq);   /*!< Pointer to a DMA2D common callback function */

/**
 * @brief  DMA2D handle Structure definition
 */
typedef struct _hal_dma2d_handle {
	int instance;                         /*!< DMA2D instance ID */

	atomic_t xfer_count;                  /*!< DMA2D pending transfer count */
	hal_dma2d_callback_t xfer_cplt_callback;   /*!< DMA2D transfer complete callback. */
	hal_dma2d_callback_t xfer_error_callback;  /*!< DMA2D transfer error callback. */

	hal_dma2d_output_cfg_t output_cfg;                       /*!< DMA2D output parameters. */
	hal_dma2d_layer_cfg_t  layer_cfg[HAL_DMA2D_MAX_LAYER];   /*!< DMA2D Layers parameters */
	hal_dma2d_rotation_cfg_t rotation_cfg;                   /*!< DMA2D Rotation parameters */

	uint32_t           error_code;                                 /*!< DMA2D error code. */
} hal_dma2d_handle_t;

/**
 * @brief HAL DMA2D_Layers DMA2D Layers
 */
#define HAL_DMA2D_BACKGROUND_LAYER  0x0000U  /*!< DMA2D Background Layer (layer 0) */
#define HAL_DMA2D_FOREGROUND_LAYER  0x0001U  /*!< DMA2D Foreground Layer (layer 1) */

/**
 * @brief HAL DMA2D_Offset DMA2D Offset
 */
#define HAL_DMA2D_OFFSET            511U     /*!< maximum Line Offset */

/**
 * @brief HAL DMA2D_Size DMA2D Size
 */
#define HAL_DMA2D_PIXEL             512U     /*!< DMA2D maximum number of pixels per line */
#define HAL_DMA2D_LINE              512U     /*!< DMA2D maximum number of lines           */

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
#define HAL_DMA2D_R2M                   0x0000U  /*!< DMA2D register to memory transfer mode */
#define HAL_DMA2D_M2M                   0x0001U  /*!< DMA2D memory to memory transfer mode, optionally with pixel format conversion */
#define HAL_DMA2D_M2M_BLEND             0x0002U  /*!< DMA2D memory to memory with blending transfer mode */
#define HAL_DMA2D_M2M_BLEND_FG          0x0004U  /*!< DMA2D memory to memory with blending transfer mode and fixed color FG */
#define HAL_DMA2D_M2M_BLEND_BG          0x0008U  /*!< DMA2D memory to memory with blending transfer mode and fixed color BG */
#define HAL_DMA2D_M2M_ROTATE            0x0010U  /*!< DMA2D memory to memory with rotation transfer mode */

/**
 * @brief HAL DMA2D_COLOR_MODE DMA2D Color Mode
 */
#define HAL_DMA2D_ARGB8888              0x0000U  /*!< ARGB8888 color mode */
#define HAL_DMA2D_ARGB6666              0x0001U  /*!< ARGB6666 color mode */
#define HAL_DMA2D_RGB888                0x0002U  /*!< RGB888 color mode   */
#define HAL_DMA2D_RGB565                0x0004U  /*!< RGB565 color mode   */
#define HAL_DMA2D_A8                    0x0008U  /*!< A8 color mode       */
#define HAL_DMA2D_A4                    0x0010U  /*!< A4 color mode       */
#define HAL_DMA2D_A1                    0x0020U  /*!< A1 color mode       */
#define HAL_DMA2D_A4_LE                 0x0040U  /*!< A4 (little endian) color mode */
#define HAL_DMA2D_A1_LE                 0x0080U  /*!< A1 (little endian) color mode */
#define HAL_DMA2D_RGB565_LE             0x0100U  /*!< RGB565 (little endian) color mode   */

/**
 * @brief HAL DMA2D_Alpha_Mode DMA2D Alpha Mode
 */
#define HAL_DMA2D_NO_MODIF_ALPHA        0x0000U  /*!< No modification of the alpha channel value */
#define HAL_DMA2D_REPLACE_ALPHA         0x0001U  /*!< Replace original alpha channel value by programmed alpha value */
#define HAL_DMA2D_COMBINE_ALPHA         0x0002U  /*!< Replace original alpha channel value by programmed alpha value
                                                  with original alpha channel value                              */

/**
 * @brief HAL DMA2D_RB_Swap DMA2D Red and Blue Swap
 */
#define HAL_DMA2D_RB_REGULAR            0x0000U  /*!< Select regular mode (RGB or ARGB) */
#define HAL_DMA2D_RB_SWAP               0x0001U  /*!< Select swap mode (BGR or ABGR) */

/**
 * @brief  HAL DMA2D common Callback ID enumeration definition
 */
typedef enum {
	HAL_DMA2D_TRANSFERCOMPLETE_CB_ID  = 0x00U,    /*!< DMA2D transfer complete callback ID       */
	HAL_DMA2D_TRANSFERERROR_CB_ID     = 0x01U,    /*!< DMA2D transfer error callback ID          */
} hal_dma2d_callback_e;

/* Exported functions --------------------------------------------------------*/

/* Initialization and de-initialization functions *******************************/
/**
 * @brief  Initialize the DMA2D peripheral and create the associated handle.
 * @param  hdma2d pointer to a hal_dma2d_handle_t structure that contains
 *                 the configuration information for the DMA2D.
 * @retval 0 on success else negative errno code.
 */
int hal_dma2d_init(hal_dma2d_handle_t *hdma2d);

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
 * @param callback_id ID of the callback to be registered
 *        This parameter can be one of the following values:
 *          @arg @ref HAL_DMA2D_TRANSFERCOMPLETE_CB_ID DMA2D transfer complete Callback ID
 *          @arg @ref HAL_DMA2D_TRANSFERERROR_CB_ID DMA2D transfer error Callback ID
 * @param callback_fn pointer to the callback function
 * @retval 0 on success else negative errno code.
 */
int hal_dma2d_register_callback(hal_dma2d_handle_t *hdma2d, hal_dma2d_callback_e callback_id, hal_dma2d_callback_t callback_fn);

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
int hal_dma2d_unregister_callback(hal_dma2d_handle_t *hdma2d, hal_dma2d_callback_e callback_id);

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
int hal_dma2d_start(hal_dma2d_handle_t *hdma2d, uint32_t pdata, uint32_t dst_address, uint16_t width, uint16_t height);

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
int hal_dma2d_blending_start(hal_dma2d_handle_t *hdma2d, uint32_t src_address1, uint32_t src_address2, uint32_t dst_address, uint16_t width,  uint16_t height);

/**
 * @brief  Start the DMA2D Rotation Transfer with interrupt enabled.
 *
 * The source size must be square, and only the ring area defined in hal_dma2d_rotation_cfg_t
 * is rotated, and the pixels inside the inner ring can be filled with constant
 * color defined in hal_dma2d_rotation_cfg_t.
 *
 * @param  hdma2d     Pointer to a hal_dma2d_handle_t structure that contains
 *                     the configuration information for the DMA2D.
 * @param  src_address The source memory Buffer start address
 * @param  dst_address The destination memory Buffer address.
 * @param  start_line  The start line to rotate.
 * @param  num_lines   Number of lines to rotate.
 * @retval command sequence (uint16_t) on success else negative errno code.
 */
int hal_dma2d_rotation_start(hal_dma2d_handle_t *hdma2d, uint32_t src_address, uint32_t dst_address, uint16_t start_line, uint16_t num_lines);

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
 * @param  hdma2d Pointer to a hal_dma2d_handle_t structure that contains
 *                 the configuration information for the DMA2D.
 * @retval 0 on success else negative errno code.
 */
int hal_dma2d_config_rotation(hal_dma2d_handle_t *hdma2d);

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
