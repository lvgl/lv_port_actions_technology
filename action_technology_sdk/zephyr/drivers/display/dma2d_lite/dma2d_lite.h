/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_DISPLAY_DMA2D_LITE_H_
#define ZEPHYR_DRIVERS_DISPLAY_DMA2D_LITE_H_

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

#endif /* ZEPHYR_DRIVERS_DISPLAY_DMA2D_LITE_H_ */
