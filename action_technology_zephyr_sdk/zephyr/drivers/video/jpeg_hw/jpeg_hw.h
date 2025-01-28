/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_JPEG_HW_H_
#define ZEPHYR_DRIVERS_JPEG_HW_H_
#include <zephyr.h>
#include <device.h>
#include <string.h>
#include <board_cfg.h>

#define JPEG_INRAM1 		0x2ff17C00
#define JPEG_INRAM0 		0x2ff17E00
#define JPEG_VLCTABLE_RAM 	0x2ff17A00
#define JPEG_IQTABLE_RAM 	0x2ff17800
#define JPEG_VLCTABLE_SIZE 	(384/4)
#define JPEG_IQTABLE_SIZE 	(192/4)


#define     SDMAIP_SDMA3HFIP                                                  19
#define     SDMAIP_SDMA2HFIP                                                  18
#define     SDMAIP_SDMA1HFIP                                                  17
#define     SDMAIP_SDMA0HFIP                                                  16
#define     SDMAIP_SDMA3TCIP                                                  3
#define     SDMAIP_SDMA2TCIP                                                  2
#define     SDMAIP_SDMA1TCIP                                                  1
#define     SDMAIP_SDMA0TCIP                                                  0

#define     SDMAIE_SDMA3HFIE                                                  19
#define     SDMAIE_SDMA2HFIE                                                  18
#define     SDMAIE_SDMA1HFIE                                                  17
#define     SDMAIE_SDMA0HFIE                                                  16
#define     SDMAIE_SDMA3TCIE                                                  3
#define     SDMAIE_SDMA2TCIE                                                  2
#define     SDMAIE_SDMA1TCIE                                                  1
#define     SDMAIE_SDMA0TCIE                                                  0

#define     SDMADEBUG_DEBUGSEL_e                                              5
#define     SDMADEBUG_DEBUGSEL_SHIFT                                          0
#define     SDMADEBUG_DEBUGSEL_MASK                                           (0x3F<<0)

#define     SDMA0CTL_STRME                                                    24
#define     SDMA0CTL_RELOAD                                                   18
#define     SDMA0CTL_DSTSL_e                                                  13
#define     SDMA0CTL_DSTSL_SHIFT                                              8
#define     SDMA0CTL_DSTSL_MASK                                               (0x3F<<8)
#define     SDMA0CTL_YSAM                                                     6
#define     SDMA0CTL_SRCSL_e                                                  5
#define     SDMA0CTL_SRCSL_SHIFT                                              0
#define     SDMA0CTL_SRCSL_MASK                                               (0x3F<<0)

#define     SDMA0START_DMASTART                                               0

#define     SDMA0SADDR_DMASADDR_e                                             31
#define     SDMA0SADDR_DMASADDR_SHIFT                                         0
#define     SDMA0SADDR_DMASADDR_MASK                                          (0xFFFFFFFF<<0)

#define     SDMA0BC_DMABYTECOUNTER_e                                          19
#define     SDMA0BC_DMABYTECOUNTER_SHIFT                                      0
#define     SDMA0BC_DMABYTECOUNTER_MASK                                       (0xFFFFF<<0)

#define     SDMA0RC_DMAREMAINCOUNTER_e                                        19
#define     SDMA0RC_DMAREMAINCOUNTER_SHIFT                                    0
#define     SDMA0RC_DMAREMAINCOUNTER_MASK                                     (0xFFFFF<<0)

#define     SDMA1CTL_STRME                                                    24
#define     SDMA1CTL_JPEG_OUT_FORMAT                                          20
#define     SDMA1CTL_YDAM                                                     14
#define     SDMA1CTL_DSTSL                                                    8
#define     SDMA1CTL_YSAM                                                     6
#define     SDMA1CTL_SRCSL                                                    0

#define     SDMA1START_DMASTART                                               0

#define     SDMA1SADDR_DMASADDR_e                                             31
#define     SDMA1SADDR_DMASADDR_SHIFT                                         0
#define     SDMA1SADDR_DMASADDR_MASK                                          (0xFFFFFFFF<<0)

#define     SDMA1DADDR_DMADADDR_e                                             31
#define     SDMA1DADDR_DMADADDR_SHIFT                                         0
#define     SDMA1DADDR_DMADADDR_MASK                                          (0xFFFFFFFF<<0)

#define     SDMA1BC_DMABYTECOUNTER_e                                          19
#define     SDMA1BC_DMABYTECOUNTER_SHIFT                                      0
#define     SDMA1BC_DMABYTECOUNTER_MASK                                       (0xFFFFF<<0)

#define     SDMA1RC_DMAREMAINCOUNTER_e                                        19
#define     SDMA1RC_DMAREMAINCOUNTER_SHIFT                                    0
#define     SDMA1RC_DMAREMAINCOUNTER_MASK                                     (0xFFFFF<<0)

#define     SDMA2CTL_STRME                                                    24
#define     SDMA2CTL_JPEG_OUT_FORMAT                                          20
#define     SDMA2CTL_YDAM                                                     14
#define     SDMA2CTL_DSTSL                                                    8
#define     SDMA2CTL_YSAM                                                     6
#define     SDMA2CTL_SRCSL                                                    0

#define     SDMA2START_DMASTART                                               0

#define     SDMA2SADDR_DMASADDR_e                                             31
#define     SDMA2SADDR_DMASADDR_SHIFT                                         0
#define     SDMA2SADDR_DMASADDR_MASK                                          (0xFFFFFFFF<<0)

#define     SDMA2DADDR_DMADADDR_e                                             31
#define     SDMA2DADDR_DMADADDR_SHIFT                                         0
#define     SDMA2DADDR_DMADADDR_MASK                                          (0xFFFFFFFF<<0)

#define     SDMA2BC_DMABYTECOUNTER_e                                          19
#define     SDMA2BC_DMABYTECOUNTER_SHIFT                                      0
#define     SDMA2BC_DMABYTECOUNTER_MASK                                       (0xFFFFF<<0)

#define     SDMA2RC_DMAREMAINCOUNTER_e                                        19
#define     SDMA2RC_DMAREMAINCOUNTER_SHIFT                                    0
#define     SDMA2RC_DMAREMAINCOUNTER_MASK                                     (0xFFFFF<<0)

#define     SDMA3CTL_STRME                                                    24
#define     SDMA3CTL_JPEG_OUT_FORMAT                                          20
#define     SDMA3CTL_YDAM                                                     14
#define     SDMA3CTL_DSTSL                                                    8
#define     SDMA3CTL_YSAM                                                     6
#define     SDMA3CTL_SRCSL                                                    0

#define     SDMA3START_DMASTART                                               0

#define     SDMA3SADDR_DMASADDR_e                                             31
#define     SDMA3SADDR_DMASADDR_SHIFT                                         0
#define     SDMA3SADDR_DMASADDR_MASK                                          (0xFFFFFFFF<<0)

#define     SDMA3DADDR_DMADADDR_e                                             31
#define     SDMA3DADDR_DMADADDR_SHIFT                                         0
#define     SDMA3DADDR_DMADADDR_MASK                                          (0xFFFFFFFF<<0)

#define     SDMA3BC_DMABYTECOUNTER_e                                          19
#define     SDMA3BC_DMABYTECOUNTER_SHIFT                                      0
#define     SDMA3BC_DMABYTECOUNTER_MASK                                       (0xFFFFF<<0)

#define     SDMA3RC_DMAREMAINCOUNTER_e                                        19
#define     SDMA3RC_DMAREMAINCOUNTER_SHIFT                                    0
#define     SDMA3RC_DMAREMAINCOUNTER_MASK                                     (0xFFFFF<<0)

#define     LINE_LENGTH0_LENGTH_e                                             15
#define     LINE_LENGTH0_LENGTH_SHIFT                                         0
#define     LINE_LENGTH0_LENGTH_MASK                                          (0xFFFF<<0)

#define     LINE_COUNT0_COUNT_e                                               15
#define     LINE_COUNT0_COUNT_SHIFT                                           0
#define     LINE_COUNT0_COUNT_MASK                                            (0xFFFF<<0)

#define     LINE_SRC_STRIDE0_STRIDE_e                                         15
#define     LINE_SRC_STRIDE0_STRIDE_SHIFT                                     0
#define     LINE_SRC_STRIDE0_STRIDE_MASK                                      (0xFFFF<<0)

#define     LINE_REMAIN0_COUNTER_e                                            15
#define     LINE_REMAIN0_COUNTER_SHIFT                                        0
#define     LINE_REMAIN0_COUNTER_MASK                                         (0xFFFF<<0)

#define     BYTE_REMAIN_IN_LINE0_COUNTER_e                                    15
#define     BYTE_REMAIN_IN_LINE0_COUNTER_SHIFT                                0
#define     BYTE_REMAIN_IN_LINE0_COUNTER_MASK                                 (0xFFFF<<0)

#define     LINE_LENGTH1_LENGTH_e                                             15
#define     LINE_LENGTH1_LENGTH_SHIFT                                         0
#define     LINE_LENGTH1_LENGTH_MASK                                          (0xFFFF<<0)

#define     LINE_COUNT1_COUNT_e                                               15
#define     LINE_COUNT1_COUNT_SHIFT                                           0
#define     LINE_COUNT1_COUNT_MASK                                            (0xFFFF<<0)

#define     LINE_SRC_STRIDE1_STRIDE_e                                         15
#define     LINE_SRC_STRIDE1_STRIDE_SHIFT                                     0
#define     LINE_SRC_STRIDE1_STRIDE_MASK                                      (0xFFFF<<0)

#define     LINE_DST_STRIDE1_STRIDE_e                                         15
#define     LINE_DST_STRIDE1_STRIDE_SHIFT                                     0
#define     LINE_DST_STRIDE1_STRIDE_MASK                                      (0xFFFF<<0)

#define     LINE_REMAIN1_COUNTER_e                                            15
#define     LINE_REMAIN1_COUNTER_SHIFT                                        0
#define     LINE_REMAIN1_COUNTER_MASK                                         (0xFFFF<<0)

#define     BYTE_REMAIN_IN_LINE1_COUNTER_e                                    15
#define     BYTE_REMAIN_IN_LINE1_COUNTER_SHIFT                                0
#define     BYTE_REMAIN_IN_LINE1_COUNTER_MASK                                 (0xFFFF<<0)

#define     LINE_LENGTH2_LENGTH_e                                             15
#define     LINE_LENGTH2_LENGTH_SHIFT                                         0
#define     LINE_LENGTH2_LENGTH_MASK                                          (0xFFFF<<0)

#define     LINE_COUNT2_COUNT_e                                               15
#define     LINE_COUNT2_COUNT_SHIFT                                           0
#define     LINE_COUNT2_COUNT_MASK                                            (0xFFFF<<0)

#define     LINE_SRC_STRIDE2_STRIDE_e                                         15
#define     LINE_SRC_STRIDE2_STRIDE_SHIFT                                     0
#define     LINE_SRC_STRIDE2_STRIDE_MASK                                      (0xFFFF<<0)

#define     LINE_DST_STRIDE2_STRIDE_e                                         15
#define     LINE_DST_STRIDE2_STRIDE_SHIFT                                     0
#define     LINE_DST_STRIDE2_STRIDE_MASK                                      (0xFFFF<<0)

#define     LINE_REMAIN2_COUNTER_e                                            15
#define     LINE_REMAIN2_COUNTER_SHIFT                                        0
#define     LINE_REMAIN2_COUNTER_MASK                                         (0xFFFF<<0)

#define     BYTE_REMAIN_IN_LINE2_COUNTER_e                                    15
#define     BYTE_REMAIN_IN_LINE2_COUNTER_SHIFT                                0
#define     BYTE_REMAIN_IN_LINE2_COUNTER_MASK                                 (0xFFFF<<0)

#define     LINE_LENGTH3_LENGTH_e                                             15
#define     LINE_LENGTH3_LENGTH_SHIFT                                         0
#define     LINE_LENGTH3_LENGTH_MASK                                          (0xFFFF<<0)

#define     LINE_COUNT3_COUNT_e                                               15
#define     LINE_COUNT3_COUNT_SHIFT                                           0
#define     LINE_COUNT3_COUNT_MASK                                            (0xFFFF<<0)

#define     LINE_SRC_STRIDE3_STRIDE_e                                         15
#define     LINE_SRC_STRIDE3_STRIDE_SHIFT                                     0
#define     LINE_SRC_STRIDE3_STRIDE_MASK                                      (0xFFFF<<0)

#define     LINE_DST_STRIDE3_STRIDE_e                                         15
#define     LINE_DST_STRIDE3_STRIDE_SHIFT                                     0
#define     LINE_DST_STRIDE3_STRIDE_MASK                                      (0xFFFF<<0)

#define     LINE_REMAIN3_COUNTER_e                                            15
#define     LINE_REMAIN3_COUNTER_SHIFT                                        0
#define     LINE_REMAIN3_COUNTER_MASK                                         (0xFFFF<<0)

#define     BYTE_REMAIN_IN_LINE3_COUNTER_e                                    15
#define     BYTE_REMAIN_IN_LINE3_COUNTER_SHIFT                                0
#define     BYTE_REMAIN_IN_LINE3_COUNTER_MASK                                 (0xFFFF<<0)


#define MAX_NUMBER_OF_COMPONENTS 3
/**
  * @brief SDMA Channel Config Module (SDMA_CHAN_Type)
  */
typedef struct {                         /*!< DMA Channel Config Structure                         */
	volatile uint32_t CTL;               /*!< (@ 0x00000000) Control Register                      */
	volatile uint32_t START;             /*!< (@ 0x00000004) Start Register                        */
	volatile uint32_t SADDR;             /*!< (@ 0x00000008) Source Address 0 Register             */
	const volatile uint32_t RESERVED_1;
	volatile uint32_t DADDR;             /*!< (@ 0x00000010) Source Address 1 Register             */
	const volatile uint32_t RESERVED_2;
	volatile uint32_t BC;                /*!< (@ 0x00000018) Byte Counter Register                 */
	volatile uint32_t RC;                /*!< (@ 0x0000001C) Remain Counter Register               */
	const volatile uint32_t RESERVED_3[56];
} SDMA_CHAN_Type;                        /*!< Size = 256 (0x100)                                   */

/**
  * @brief SDMA Line Config Module (SDMA_LINE_Type)
  */
typedef struct {                           /*!< SDMA Line Config Structure                               */
	volatile uint32_t LENGTH;              /*!< (@ 0x00000000) Line Length Register                      */
	volatile uint32_t COUNT;               /*!< (@ 0x00000004) Line Count Register                       */
	volatile uint32_t SSTRIDE;             /*!< (@ 0x00000008) Line Src Stride Register                  */
	volatile uint32_t DSTRIDE;             /*!< (@ 0x0000000C) Line Dest Stride Register                 */
	volatile uint32_t REMAIN;              /*!< (@ 0x00000010) Line Remain Register                      */
	volatile uint32_t BYTE_REMAIN_IN_LINE; /*!< (@ 0x00000014) Byte Remain in Transmitting Line Register */
	const volatile uint32_t RESERVED[2];
} SDMA_LINE_Type;                          /*!< Size = 32 (0x20)                                         */

/**
  * @brief SDMA Module (SDMA)
  */
typedef struct {                             /*!< SDMA Structure                                */
	volatile uint32_t IP;                    /*!< (@ 0x00000000) Interrupt Pending Register     */
	volatile uint32_t IE;                    /*!< (@ 0x00000004) Interrupt Enable Register      */
	const volatile uint32_t RESERVED_1[26];
	volatile uint32_t PRIORITY;              /*!< (@ 0x00000070) Priority Control Register      */
	const volatile uint32_t RESERVED_2[3];
	volatile uint32_t DEBUG;                 /*!< (@ 0x00000080) Debug Register                 */
	const volatile uint32_t RESERVED_3[7];
	volatile uint32_t COUPLE_CONFIG;         /*!< (@ 0x000000A0) Debug Register                 */
	volatile uint32_t COUPLE_BUF_ADDR;       /*!< (@ 0x000000A4) Debug Register                 */
	volatile uint32_t COUPLE_BUF_SIZE;       /*!< (@ 0x000000A8) Debug Register                 */
	volatile uint32_t COUPLE_START;          /*!< (@ 0x000000AC) Debug Register                 */
	volatile uint32_t COUPLE_WRITER_POINTER; /*!< (@ 0x000000B0) Debug Register                 */
	volatile uint32_t COUPLE_READ_POINTER;   /*!< (@ 0x000000B4) Debug Register                 */
	const volatile uint32_t RESERVED_4[18];
	SDMA_CHAN_Type CHAN_CTL[5];
	const volatile uint32_t RESERVED_5[384];
	SDMA_LINE_Type LINE_CTL[5];
	const volatile uint32_t RESERVED_6[24];
	volatile uint32_t COLOR_FILL_DATA[3];    /*!< (@ 0x000000AC) Color Fill Data Register       */
} __attribute__((__packed__)) SDMA_Type;     /*!< Size = 3340 (0xD0C)                           */

/**
  * @brief JPEG_HW Module (JPEG_HW)
  */
typedef struct {                             /*!< JPEG_HW Structure                                 	*/
	volatile uint32_t JPEG_ENABLE;           /*!< (@ 0x00000000) Decoder enable register     	 	*/
	volatile uint32_t JPEG_INTERRUPT;        /*!< (@ 0x00000004) Decoder interrupt DRQ register  	*/
	volatile uint32_t JPEG_CONFIG;        	 /*!< (@ 0x00000008) Decode configure register		 	*/
	volatile uint32_t JPEG_BITSTREAM;        /*!< (@ 0x0000000C) Bitstream register     		 	*/
	volatile uint32_t JPEG_DOWNSAMPLE;       /*!< (@ 0x00000010) Downsample register			 	*/
	volatile uint32_t JPEG_STREAM_SIZE;      /*!< (@ 0x00000014) Stream size register    		 	*/
	volatile uint32_t JPEG_WINDOW_SIZE;      /*!< (@ 0x00000018) Window_wh register 			 	*/
	volatile uint32_t JPEG_OUTPUT;        	 /*!< (@ 0x0000001C) Output height register		 	 	*/
	volatile uint32_t JPEG_WINDOW_START;     /*!< (@ 0x00000020) Window start		              	*/
	volatile uint32_t JPEG_RESOLUTION;       /*!< (@ 0x00000024) Decoder picture resolution register*/
	volatile uint32_t JPEG_HUFF_TABLE;       /*!< (@ 0x00000028) Huffman table select register      */
	volatile uint32_t JPEG_AC_VLC[7];        /*!< (@ 0x0000002C) Jpeg AC VLC code part		  		*/
	volatile uint32_t JPEG_DC_VLC[4];        /*!< (@ 0x00000048) Jpeg DC VLC code part		  		*/
	const volatile uint32_t RESERVED;
	volatile uint32_t JPEG_DEBUG;        	 /*!< (@ 0x0000005C) Decoder debug register		  		*/
} __attribute__((__packed__)) JPEG_HW_Type;     /*!< Size = 3340 (0xD0C)                           */

typedef enum { /* JPEG marker codes */
	YUV100 = 0,
	YUV111,
	YUV420,
	YUV211H,
	YUV211V,
	YUV422,
	YUV_OTHER,
}jpeg_yue_mode_e;

typedef struct
{
    uint32_t Ls;
    uint32_t Ns;
    uint32_t Cs[MAX_NUMBER_OF_COMPONENTS];   /* Scan component selector */
    uint32_t Td[MAX_NUMBER_OF_COMPONENTS];   /* Selects table for DC */
    uint32_t Ta[MAX_NUMBER_OF_COMPONENTS];   /* Selects table for AC */
    uint32_t Ss;
    uint32_t Se;
    uint32_t Ah;
    uint32_t Al;
    uint32_t index;
} ScanInfo;

struct jpeg_info_t {
	uint8_t *stream_addr;
	uint32_t stream_offset;
	uint32_t stream_size;
	uint16_t image_w;
	uint16_t image_h;
	uint16_t window_x;
	uint16_t window_y;
	uint16_t window_w;
	uint16_t window_h;
	uint16_t output_stride;
	uint8_t  output_format;
	uint8_t  ds_mode;
	uint8_t *output_bmp;
	uint8_t  DC_TAB0[16];
	uint8_t  DC_TAB1[16];
	uint8_t  AC_TAB0[16];
	uint8_t  AC_TAB1[16];
	uint8_t  *HDcTable[4];
	uint8_t  *HAcTable[4];
	int32_t  amountOfQTables;
	int32_t  getQTablenum;
	jpeg_yue_mode_e yuv_mode;
	ScanInfo scan;
	uint8_t Date[20];
};


enum jpeg_hw_status {
	/* frame decode finised*/
	DECODE_FRAME_FINISHED = 0,
	/* block decoded finised */
	DECODE_BLOCK_FINISHED = 1,
	/* block decoded ERR */
	DECODE_DRROR = 2,
};

/**
 * @struct jpeg_hw_engine_capabilities
 * @brief Structure holding display engine capabilities
 *
 * @var uint8_t jpeg_hw_capabilities::num_overlays
 * Maximum number of overlays supported
 *
 * @var uint16_t jpeg_hw_capabilities::max_width
 * X Resolution at maximum
 *
 * @var uint16_t jpeg_hw_capabilities::max_height
 * Y Resolution at maximum
 *
 * @var uint8_t jpeg_hw_capabilities::support_blend_fg
 * Blending constant fg color supported
 *
 * @var uint8_t jpeg_hw_capabilities::support_blend_b
 * Blending constant bg color supported
 *
 * @var uint32_t jpeg_hw_capabilities::supported_input_pixel_formats
 * Bitwise or of input pixel formats supported by the display engine
 *
 * @var uint32_t jpeg_hw_capabilities::supported_output_pixel_formats
 * Bitwise or of output pixel formats supported by the display engine
 *
 * @var uint32_t jpeg_hw_capabilities::supported_rotate_pixel_formats
 * Bitwise or of rotation pixel formats supported by the display engine
 *
 */
struct jpeg_hw_capabilities {
	uint32_t min_width 	: 10;
	uint32_t min_height	: 10;
	uint32_t max_width 	: 10;
	uint32_t max_height : 10;
};

/**
 * @typedef jpeg_hw_engine_instance_callback_t
 * @brief Callback API executed when any display engine instance transfer complete or error
 *
 */
typedef void (*jpeg_hw_instance_callback_t)(int err_code, void *user_data);


/**
 * @typedef jpeg_hw_open_api
 * @brief Callback API to open jpeg hw
 * See jpeg_hw_open_api() for argument description
 */
typedef int (*jpeg_hw_open_api)(const struct device *dev);

/**
 * @typedef jpeg_hw_close_api
 * @brief Callback API to close jpeg hw
 * See jpeg_hw_close() for argument description
 */
typedef int (*jpeg_hw_close_api)(const struct device *dev);

/**
 * @typedef jpeg_hw_config_api
 * @brief Callback API to config jpeg hw
 * See jpeg_hw_config() for argument description
 */
typedef int (*jpeg_hw_config_api)(const struct device *dev,
											struct jpeg_info_t *cfg_info);

/**
 * @typedef jpeg_hw_config_api
 * @brief Callback API to config jpeg hw
 * See jpeg_hw_config() for argument description
 */
typedef int (*jpeg_hw_decode_api)(const struct device *dev);

/**
 * @typedef jpeg_hw_get_capabilities_api
 * @brief Callback API to get jpeg hw capabilities
 * See jpeg_hw_get_capabilities() for argument description
 */
typedef void (*jpeg_hw_get_capabilities_api)(const struct device *dev,
		struct jpeg_hw_capabilities *capabilities);

/**
 * @typedef jpeg_hw_register_callback_api
 * @brief Callback API to register instance callback
 * See jpeg_hw_register_callback() for argument description
 */
typedef int (*jpeg_hw_register_callback_api)(const struct device *dev,
		        jpeg_hw_instance_callback_t callback, void *user_data);

/**
 * @typedef jpeg_hw_poll_api
 * @brief Callback API to poll complete of jpeg hw
 * See jpeg_hw_poll_api() for argument description
 */
typedef int (*jpeg_hw_poll_api)(const struct device *dev,
		 	 int timeout_ms);

/**
 * @brief jpeg hw driver API
 * API which a jpeg hw driver should expose
 */
struct jpeg_hw_driver_api {
	jpeg_hw_open_api open;
	jpeg_hw_close_api close;
	jpeg_hw_config_api config;
	jpeg_hw_get_capabilities_api get_capabilities;
	jpeg_hw_register_callback_api register_callback;
	jpeg_hw_decode_api decode;
	jpeg_hw_poll_api poll;
};

/**
 * @brief open jpeg hw 
 *
 * @param dev Pointer to device structure
 *
 * @retval 0 on success else negative errno code.
 */
static inline int jpeg_open(
		const struct device *dev)
{
	struct jpeg_hw_driver_api *api =
		(struct jpeg_hw_driver_api *)dev->api;

	return api->open(dev);
}
/**
 * @brief config jpeg hw 
 *
 * @param dev Pointer to device structure
 *
 * @retval 0 on success else negative errno code.
 */
static inline int jpeg_config(
		const struct device *dev, struct jpeg_info_t *cfg_info)
{
	struct jpeg_hw_driver_api *api =
		(struct jpeg_hw_driver_api *)dev->api;

	return api->config(dev, cfg_info);
}


/**
 * @brief  jpeg hw start process 
 *
 * @param dev Pointer to device structure
 *
 * @retval 0 on success else negative errno code.
 */
static inline int jpeg_decode(
		const struct device *dev)
{
	struct jpeg_hw_driver_api *api =
		(struct jpeg_hw_driver_api *)dev->api;

	return api->decode(dev);
}

/**
 * @brief  jpeg hw start process 
 *
 * @param dev Pointer to device structure
 *
 * @retval 0 on success else negative errno code.
 */
static inline int jpeg_register_callback(
		const struct device *dev,
		jpeg_hw_instance_callback_t callback, void *user_data)
{
	struct jpeg_hw_driver_api *api =
		(struct jpeg_hw_driver_api *)dev->api;

	return api->register_callback(dev, callback, user_data);
}

/**
 * @brief close jpeg hw 
 *
 * @param dev Pointer to device structure
 *
 * @retval 0 on success else negative errno code.
 */
static inline int jpeg_decode_wait_finished(
		const struct device *dev, int timeout)
{
	struct jpeg_hw_driver_api *api =
		(struct jpeg_hw_driver_api *)dev->api;

	return api->poll(dev, timeout);
}

/**
 * @brief close jpeg hw 
 *
 * @param dev Pointer to device structure
 *
 * @retval 0 on success else negative errno code.
 */
static inline int jpeg_close(
		const struct device *dev)
{
	struct jpeg_hw_driver_api *api =
		(struct jpeg_hw_driver_api *)dev->api;

	return api->close(dev);
}

#endif /* ZEPHYR_DRIVERS_JPEG_HW_H_ */
