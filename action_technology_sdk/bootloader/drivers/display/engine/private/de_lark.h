/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_DISPLAY_ENGINE_DE_LARK_H_
#define ZEPHYR_DRIVERS_DISPLAY_ENGINE_DE_LARK_H_

#include <zephyr/types.h>
#include <sys/util.h>

/* Reg DE_CTL */
#define DE_CTL_OUT_FORMAT_SEL(x)              ((uint32_t)(x) << 2)
#define DE_CTL_OUT_FORMAT_RGB565              DE_CTL_OUT_FORMAT_SEL(0)
#define DE_CTL_OUT_FORMAT_RGB666              DE_CTL_OUT_FORMAT_SEL(1)
#define DE_CTL_OUT_FORMAT_RGB888              DE_CTL_OUT_FORMAT_SEL(2)
#define DE_CTL_OUT_FORMAT_RGB888_WB_ARGB8888  DE_CTL_OUT_FORMAT_SEL(3)
#define DE_CTL_OUT_COLOR_FILL_EN              (0x1 << 5)
#define DE_CTL_OUT_MODE_SEL(x)                ((uint32_t)(x) << 8)
#define DE_CTL_OUT_MODE_MASK                  DE_CTL_OUT_MODE_SEL(3)
#define DE_CTL_OUT_MODE_DISPLAY               DE_CTL_OUT_MODE_SEL(0)
#define DE_CTL_OUT_MODE_DISPLAY_WB            DE_CTL_OUT_MODE_SEL(1)
#define DE_CTL_OUT_MODE_WB                    DE_CTL_OUT_MODE_SEL(2)
#define DE_CTL_WB_NO_STRIDE_EN(en)            ((uint32_t)(en) << 29)
#define DE_CTL_TRANSFER_MODE_SEL(x)           ((uint32_t)(x) << 12)
#define DE_CTL_TRANSFER_MODE_MASK             DE_CTL_TRANSFER_MODE_SEL(1)
#define DE_CTL_TRANSFER_MODE_TRIGGER          DE_CTL_TRANSFER_MODE_SEL(0)
#define DE_CTL_TRANSFER_MODE_CONTINUE         DE_CTL_TRANSFER_MODE_SEL(1)

#define DE_FORMAT_RGB565          (0)
#define DE_FORMAT_RGB666          (1)
#define DE_FORMAT_RGB888          (2)
#define DE_FORMAT_ARGB            (3)
#define DE_FORMAT_BGRA            (4)
#define DE_FORMAT_AX              (5)

#define DE_CTL_L0_EN                    BIT(20)
#define DE_CTL_L0_HALFWORD_EN(en)       ((uint32_t)(en) << 0)
#define DE_CTL_L0_NO_STRIDE_EN(en)      ((uint32_t)(en) << 31)
#define DE_CTL_L0_FORMAT(de_fmt, rb_swap, g_swap, is_6666)                     \
		(((uint32_t)(de_fmt) << 15) | ((uint32_t)(rb_swap) << 18) |            \
		((uint32_t)(!(g_swap)) << 14) | ((uint32_t)(is_6666) << 19))
#define DE_CTL_L0_FORMAT_RGB565      DE_CTL_L0_FORMAT(DE_FORMAT_RGB565, 0, 0, 0)
#define DE_CTL_L0_FORMAT_BGR565      DE_CTL_L0_FORMAT(DE_FORMAT_RGB565, 1, 0, 0)
#define DE_CTL_L0_FORMAT_RGB565_SWAP DE_CTL_L0_FORMAT(DE_FORMAT_RGB565, 0, 1, 0)
#define DE_CTL_L0_FORMAT_BGR565_SWAP DE_CTL_L0_FORMAT(DE_FORMAT_RGB565, 1, 1, 0)
#define DE_CTL_L0_FORMAT_RGB666      DE_CTL_L0_FORMAT(DE_FORMAT_RGB666, 0, 0, 0)
#define DE_CTL_L0_FORMAT_BGRB666     DE_CTL_L0_FORMAT(DE_FORMAT_RGB666, 1, 0, 0)
#define DE_CTL_L0_FORMAT_RGB888      DE_CTL_L0_FORMAT(DE_FORMAT_RGB888, 0, 0, 0)
#define DE_CTL_L0_FORMAT_BGR888      DE_CTL_L0_FORMAT(DE_FORMAT_RGB888, 1, 0, 0)
#define DE_CTL_L0_FORMAT_ARGB8888    DE_CTL_L0_FORMAT(DE_FORMAT_ARGB, 0, 0, 0)
#define DE_CTL_L0_FORMAT_ABGR8888    DE_CTL_L0_FORMAT(DE_FORMAT_ARGB, 1, 0, 0)
#define DE_CTL_L0_FORMAT_BGRA8888    DE_CTL_L0_FORMAT(DE_FORMAT_BGRA, 0, 0, 0)
#define DE_CTL_L0_FORMAT_RGBA8888    DE_CTL_L0_FORMAT(DE_FORMAT_BGRA, 1, 0, 0)
#define DE_CTL_L0_FORMAT_ARGB6666    DE_CTL_L0_FORMAT(DE_FORMAT_ARGB, 0, 0, 1)
#define DE_CTL_L0_FORMAT_ABGR6666    DE_CTL_L0_FORMAT(DE_FORMAT_ARGB, 1, 0, 1)
#define DE_CTL_L0_FORMAT_BGRA6666    DE_CTL_L0_FORMAT(DE_FORMAT_BGRA, 0, 0, 1)
#define DE_CTL_L0_FORMAT_RGBA6666    DE_CTL_L0_FORMAT(DE_FORMAT_BGRA, 1, 0, 1)
#define DE_CTL_L0_FORMAT_AX          DE_CTL_L0_FORMAT(DE_FORMAT_AX, 0, 0, 0)
#define DE_CTL_L1_EN                 BIT(26)
#define DE_CTL_L1_HALFWORD_EN(en)    ((uint32_t)(en) << 1)
#define DE_CTL_L1_NO_STRIDE_EN(en)   ((uint32_t)(en) << 30)
#define DE_CTL_L1_COLOR_FILL_EN      BIT(28)
#define DE_CTL_L1_FORMAT(de_fmt, rb_swap, g_swap, is_6666)                     \
		(((uint32_t)(de_fmt) << 21) | ((uint32_t)(rb_swap) << 24) |            \
		((uint32_t)(!(g_swap)) << 27) | ((uint32_t)(is_6666) << 25))
#define DE_CTL_L1_FORMAT_RGB565      DE_CTL_L1_FORMAT(DE_FORMAT_RGB565, 0, 0, 0)
#define DE_CTL_L1_FORMAT_BGR565      DE_CTL_L1_FORMAT(DE_FORMAT_RGB565, 1, 0, 0)
#define DE_CTL_L1_FORMAT_RGB565_SWAP DE_CTL_L1_FORMAT(DE_FORMAT_RGB565, 0, 1, 0)
#define DE_CTL_L1_FORMAT_BGR565_SWAP DE_CTL_L1_FORMAT(DE_FORMAT_RGB565, 1, 1, 0)
#define DE_CTL_L1_FORMAT_RGB666      DE_CTL_L1_FORMAT(DE_FORMAT_RGB666, 0, 0, 0)
#define DE_CTL_L1_FORMAT_BGR666      DE_CTL_L1_FORMAT(DE_FORMAT_RGB666, 1, 0, 0)
#define DE_CTL_L1_FORMAT_RGB888      DE_CTL_L1_FORMAT(DE_FORMAT_RGB888, 0, 0, 0)
#define DE_CTL_L1_FORMAT_BGR888      DE_CTL_L1_FORMAT(DE_FORMAT_RGB888, 1, 0, 0)
#define DE_CTL_L1_FORMAT_ARGB8888    DE_CTL_L1_FORMAT(DE_FORMAT_ARGB, 0, 0, 0)
#define DE_CTL_L1_FORMAT_ABGR8888    DE_CTL_L1_FORMAT(DE_FORMAT_ARGB, 1, 0, 0)
#define DE_CTL_L1_FORMAT_BGRA8888    DE_CTL_L1_FORMAT(DE_FORMAT_BGRA, 0, 0, 0)
#define DE_CTL_L1_FORMAT_RGBA8888    DE_CTL_L1_FORMAT(DE_FORMAT_BGRA, 1, 0, 0)
#define DE_CTL_L1_FORMAT_ARGB6666    DE_CTL_L1_FORMAT(DE_FORMAT_ARGB, 0, 0, 1)
#define DE_CTL_L1_FORMAT_ABGR6666    DE_CTL_L1_FORMAT(DE_FORMAT_ARGB, 1, 0, 1)
#define DE_CTL_L1_FORMAT_BGRA6666    DE_CTL_L1_FORMAT(DE_FORMAT_BGRA, 0, 0, 1)
#define DE_CTL_L1_FORMAT_RGBA6666    DE_CTL_L1_FORMAT(DE_FORMAT_BGRA, 1, 0, 1)
#define DE_CTL_L1_FORMAT_AX          DE_CTL_L1_FORMAT(DE_FORMAT_AX, 0, 0, 0)

#define DE_L_POS(x, y)             (((uint32_t)(y) << 16) | (x))
#define DE_L_SIZE(w, h)            ((((uint32_t)(h) - 1) << 16) | ((w) - 1))
#define DE_L_COLOR_GAIN(r, g, b)   (((uint32_t)(r) << 16) | ((uint32_t)(g) << 8) | (b))
#define DE_L_COLOR_OFFSET(r, g, b) (((uint32_t)(r) << 16) | ((uint32_t)(g) << 8) | (b))
#define DE_L_DEF_COLOR(a, r, g, b)                                             \
	(((uint32_t)(a) << 24) | ((uint32_t)(r) << 16) | ((uint32_t)(g) << 8) | (b))

/* Reg DE_GATE_CTL */
#define DE_LAYER_GATING_EN      BIT(0)
#define DE_OUTPUT_GATING_EN     BIT(1)
#define DE_GAMMA_AHB_GATING_EN  BIT(2)
#define DE_PATH_GATING_EN       BIT(3)
#define DE_ROTATE_GATING_EN     BIT(4)

/* Reg DE_CTL2 */
#define DE_CTL2_WB_START           BIT(0)
#define DE_CTL2_OUT_FIFO_RESET     BIT(1)
#define DE_CTL2_WB_FIFO_RESET      BIT(2)
#define DE_CTL2_L0_FIFO_RESET      BIT(3)
#define DE_CTL2_L1_FIFO_RESET      BIT(4)

/* Reg DE_MEM_OPT */
#define DE_MEM_BURST_LEN_SEL(x)  ((uint32_t)(x) << 8)
#define DE_MEM_BURST_LEN_MASK    DE_MEM_BURST_LEN_SEL(3)
#define DE_MEM_BURST_8           DE_MEM_BURST_LEN_SEL(0)
#define DE_MEM_BURST_16          DE_MEM_BURST_LEN_SEL(1)
#define DE_MEM_BURST_32          DE_MEM_BURST_LEN_SEL(2)
#define DE_MEM_SWITCH_EN         BIT(6)
#define DE_MEM_CMD_NUM_SEL(x)    (((uint32_t)(x) & 0x3f) << 0)
#define DE_MEM_CMD_NUM_MASK      DE_MEM_CMD_NUM_SEL(0x3f)
#define DE_SW_FRAME_RST_LEN(x)    (((uint32_t)(x) & 0x3f) << 10)
#define DE_SW_FRAME_RST_MASK      DE_MEM_CMD_NUM_SEL(0x3f)

/* Reg DE_IRQ_CTL */
#define DE_IRQ_PRELINE          BIT(0)
#define DE_IRQ_L0_FIFO_UDF      BIT(1) /* under flow*/
#define DE_IRQ_L0_FIFO_OVF      BIT(2)  /* over flow*/
#define DE_IRQ_L1_FIFO_UDF      BIT(3)
#define DE_IRQ_L1_FIFO_OVF      BIT(4)
#define DE_IRQ_L0_FTC           BIT(5) /* frame transfer complete */
#define DE_IRQ_L1_FTC           BIT(6)
#define DE_IRQ_WB_FIFO_UDF      BIT(8)
#define DE_IRQ_WB_FIFO_OVF      BIT(9)
#define DE_IRQ_DEV_FIFO_UDF     BIT(10)
#define DE_IRQ_DEV_FIFO_OVF     BIT(11)
#define DE_IRQ_VSYNC            BIT(16)
#define DE_IRQ_DEV_FIFO_HF      BIT(29) /* half full */
#define DE_IRQ_WB_FTC           BIT(30)
#define DE_IRQ_DEV_FTC          BIT(31)

/* Reg DE_STA */
#define DE_STAT_L0_MEM_ERR        BIT(0)
#define DE_STAT_L1_MEM_ERR        BIT(1)
#define DE_STAT_L0_FIFO_EMPTY     BIT(7)
#define DE_STAT_L0_FIFO_FULL      BIT(8)
#define DE_STAT_L0_FIFO_UDF       BIT(9)
#define DE_STAT_L0_FIFO_OVF       BIT(10)
#define DE_STAT_L0_FTC            BIT(11)
#define DE_STAT_WB_FIFO_UDF       BIT(12)
#define DE_STAT_WB_FIFO_OVF       BIT(13)
#define DE_STAT_DEV_FIFO_UDF      BIT(14)
#define DE_STAT_DEV_FIFO_OVF      BIT(15)
#define DE_STAT_DEV_VSYNC         BIT(16)
#define DE_STAT_L1_FIFO_EMPTY     BIT(23)
#define DE_STAT_L1_FIFO_FULL      BIT(24)
#define DE_STAT_L1_FIFO_UDF       BIT(25)
#define DE_STAT_L1_FIFO_OVF       BIT(26)
#define DE_STAT_L1_FTC            BIT(27)
#define DE_STAT_PRELINE           BIT(28)
#define DE_STAT_DEV_FIFO_HF       BIT(29)
#define DE_STAT_WB_FTC            BIT(30)
#define DE_STAT_DEV_FTC           BIT(31)

/* Reg DE_ALPHA_CTL */
#define DE_ALPHA_EN               BIT(31)
#define DE_ALPHA_COVERAGE         (0x0 << 10)
#define DE_ALPHA_PREMULTIPLIED    (0x1 << 10)
#define DE_ALPHA_PLANE_ALPHA(x)   ((x) & 0xff)

#define DE_ALPHA_POS(x, y)   (((uint32_t)(y) << 16) | (x))
#define DE_ALPHA_SIZE(w, h)  ((((uint32_t)(h) - 1) << 16) | ((w) - 1))

/* Reg DE_BG_SIZE */
#define DE_BG_SIZE(w, h)      ((((uint32_t)(h) - 1) << 16) | ((w) - 1))
/* Reg DE_BG_COLOR */
#define DE_BG_COLOR(r, g, b)  (((uint32_t)(r) << 16) | ((uint32_t)(g) << 8) | (b))

/* Reg DE_FILL_COLOR */
#define DE_FILL_COLOR(a, r, g, b)                                              \
	(((uint32_t)(a) << 24) | ((uint32_t)(r) << 16) | ((uint32_t)(g) << 8) | (b))
/* Reg DE_COLOR_FILL_POS */
#define DE_COLOR_FILL_POS(x, y)   (((uint32_t)(y) << 16) | (x))
/* Reg DE_COLOR_FILL_SIZE */
#define DE_COLOR_FILL_SIZE(w, h)  ((((uint32_t)(h) - 1) << 16) | ((w) - 1))

/* Reg DE_A148_COLOR */
#define DE_AX_TYPE_SEL(x)     ((uint32_t)(x) << 24)
#define DE_TYPE_A8            DE_AX_TYPE_SEL(0)
#define DE_TYPE_A4            DE_AX_TYPE_SEL(1)
#define DE_TYPE_A1            DE_AX_TYPE_SEL(2)
#define DE_AX_COLOR(r, g, b)  (((uint32_t)(r) << 16) | ((uint32_t)(g) << 8) | (b))

/* Reg DE_A14_CTL */
#define DE_A14_FONT(size, stride, total)                                      \
	(((uint32_t)((size) - 1) << 20) | ((uint32_t)((stride) - 1) << 10) | ((total) - 1))

/* Reg DE_DITHER_CTL */
/*
 * internal dither table as follows:
 *
 * int table4x4_5bit[16]={
 * 	0, 4, 1, 5,
 * 	6, 2, 7, 3,
 * 	2, 6, 1, 5,
 * 	8, 4, 7, 3,
 * };
 *
 * int table4x4_6bit[16]={
 * 	0, 2, 1, 3,
 * 	3, 1, 4, 2,
 * 	1, 3, 0, 2,
 * 	4, 2, 3, 1,
 * };
 *
 * int table8x8_5bit[64]={
 * 	0, 12,  3, 15,  1, 13,  4, 16,
 * 	8,  4, 11,  7,  9,  5, 12,  8,
 * 	2, 14,  1, 13,  3, 15,  2, 14,
 * 	10,  6,  9,  5, 11,  7, 10,  6,
 * 	1, 13,  4, 16,  0, 12,  3, 15,
 * 	9,  5, 12,  8,  8,  4, 11,  7,
 * 	3, 15,  2, 14,  2, 14,  1, 13,
 * 	11,  7, 10,  6, 10,  6,  9,  5,
 * };
 *
 * int table8x8_6bit[64]={
 * 	0, 6, 2, 8, 0, 6, 2, 8,
 * 	4, 2, 6, 4, 4, 2, 6, 4,
 * 	1, 7, 1, 7, 1, 7, 1, 7,
 * 	5, 3, 5, 3, 5, 3, 5, 3,
 * 	0, 6, 2, 8, 0, 6, 2, 8,
 * 	4, 2, 6, 4, 4, 2, 6, 4,
 * 	1, 7, 1, 7, 1, 7, 1, 7,
 * 	5, 3, 5, 3, 5, 3, 5, 3,
 * };
 */
#define DE_DITHER_EN               BIT(0)
#define DE_DITHER_STRENGTH_SEL(x)  ((uint32_t)(x) << 4)
#define DE_DITHER_STRENGTH_4X4     DE_DITHER_STRENGTH_SEL(0)
#define DE_DITHER_STRENGTH_8X8     DE_DITHER_STRENGTH_SEL(1)
#define DE_DITHER_POS_EN           BIT(5)
#define DE_DITHER_POS(x, y)        ((((uint32_t)(x) & 0x7) << 6) | (((uint32_t)(y) & 0x7) << 9))

/* Reg DE_GAMMA_CTL */
#define DE_GAMMA_EN            BIT(16)
#define DE_GAMMA_RAM_INDEX(x)  ((x) & 0xff)

/* Reg RT_CTL */
#define RT_EN                   BIT(31)
#define RT_FORMAT_SEL(x)        ((uint32_t)(x) << 28)
#define RT_FORMAT_RGB565_SWAP   RT_FORMAT_SEL(0)
#define RT_FORMAT_RGB565        RT_FORMAT_SEL(1)
#define RT_FORMAT_ARGB8888      RT_FORMAT_SEL(2)
#define RT_FILTER_SEL(x)        ((uint32_t)(x) << 8)
#define RT_STAT_COMPLETE        BIT(7)
#define RT_FILTER_LINEAR        RT_FILTER_SEL(0)
#define RT_FILTER_BILINEAR      RT_FILTER_SEL(1)
#define RT_IRQ_EN               BIT(4)
#define RT_COLOR_FILL_EN        BIT(2)
#define RT_AUTOLOAD_EN          BIT(1)

/* Reg RT_IMG_SIZE */
#define RT_END_HEIGHT(x)        ((uint32_t)(x) << 16)
#define RT_IMG_WIDTH(x)         ((x) & 0x1ff)

/* Reg RT_IMG_START_HEIGHT */
#define RT_START_HEIGHT(x)      ((x) & 0x1ff)

/* Reg RT_SW_X_XY & RT_SW_Y_XY */
#define RT_SW_DELTA_XY(dx, dy)                                                \
	(((uint32_t)(dy) & 0x3fff) << 16) | (((uint32_t)(dx) & 0x3fff))

/* Reg RT_FILL_COLOR */
#define RT_COLOR_ARGB_8888(a, r, g, b)                                              \
	(((uint32_t)(a) << 24) | ((uint32_t)(r) << 16) | ((uint32_t)(g) << 8) | (b))
#define RT_COLOR_RGB_565(r, g, b)                                              \
	((((uint32_t)(r) >> 3) << 11) | (((uint32_t)(g) >> 2) << 5) | ((b) >> 3))

/**
  * @brief DE Module (DE)
  */
typedef struct {
	volatile uint32_t POS;          /*!< (@ 0x00000100) Layer Start Position Register                                */
	volatile uint32_t SIZE;         /*!< (@ 0x00000104) Layer Size Register                                          */
	volatile uint32_t ADDR;         /*!< (@ 0x00000108) Layer Mem Start Address Register                             */
	volatile uint32_t STRIDE;       /*!< (@ 0x0000010C) Layer Mem Stride Register                                    */
	volatile uint32_t LENGTH;       /*!< (@ 0x00000110) Layer Mem Transfer Length Register                           */
	volatile uint32_t COLOR_GAIN;   /*!< (@ 0x00000114) Layer Color Gain Register                                    */
	volatile uint32_t COLOR_OFFSET; /*!< (@ 0x00000118) Layer Color Offset Register                                  */
	volatile uint32_t DEF_COLOR;    /*!< (@ 0x0000011C) Layer Default Color Register                                 */
	const volatile uint32_t RESERVED[56];
} DE_LAYER_CTL_Type;

typedef struct {                            /*!< (@ 0x4006C000) DE Structure                                         */
	volatile uint32_t CTL;                  /*!< (@ 0x00000000) DE Control Register                                  */
	volatile uint32_t GAT_CTL;              /*!< (@ 0x00000004) DE Gatting Control Register                          */
	volatile uint32_t REG_UD;               /*!< (@ 0x00000008) DE Layer0 Size Register                              */
	volatile uint32_t IRQ_CTL;              /*!< (@ 0x0000000C) DE IRQ Control Register                              */
	volatile uint32_t BG_SIZE;              /*!< (@ 0x00000010) DE BG Size Register                                  */
	volatile uint32_t BG_COLOR;             /*!< (@ 0x00000014) DE default color Register                            */
	volatile uint32_t MEM_OPT;              /*!< (@ 0x00000018) DE Mem Opt Register                                  */
	volatile uint32_t EN;                   /*!< (@ 0x0000001C) DE Mem Enable Register                               */
	volatile uint32_t CTL2;                 /*!< (@ 0x00000020) DE Control2 Register                                 */
	const volatile uint32_t RESERVED1[55];
	DE_LAYER_CTL_Type LAYER_CTL[2];         /*!< (@ 0x00000100) DE Layer Control Register                            */
	volatile uint32_t ALPHA_CTL;            /*!< (@ 0x00000300) DE Alpha Control Register                            */
	volatile uint32_t ALPHA_POS;            /*!< (@ 0x00000304) Alpha Position Register                              */
	volatile uint32_t ALPHA_SIZE;           /*!< (@ 0x00000308) Alpha Size Register                                  */
	volatile uint32_t STA;                  /*!< (@ 0x0000030C) DE Status Register                                   */
	volatile uint32_t GAMMA_CTL;            /*!< (@ 0x00000310) DE Gamma Control Register                            */
	volatile uint32_t PATH_GAMMA_RAM;       /*!< (@ 0x00000314) DE Gamma Data Path Register                          */
	volatile uint32_t DITHER_CTL;           /*!< (@ 0x00000318) DE Dither Control Register                           */
	volatile uint32_t WB_MEM_ADR;           /*!< (@ 0x0000031C) WriteBack Mem Start Address Register                 */
	volatile uint32_t WB_MEM_STRIDE;        /*!< (@ 0x00000320) WriteBack Mem Stride Register                        */
	volatile uint32_t COLOR_FILL_POS;       /*!< (@ 0x00000324) Color Fill Position Register                         */
	volatile uint32_t COLOR_FILL_SIZE;      /*!< (@ 0x00000328) Fill Size Register                                   */
	volatile uint32_t FILL_COLOR;           /*!< (@ 0x0000032C) Fill Color Register                                  */
	volatile uint32_t A148_COLOR;           /*!< (@ 0x00000330) Ax Color Register                                    */
	volatile uint32_t A14_CTL;              /*!< (@ 0x00000334) A14 Control Register                                 */
	const volatile uint32_t RESERVED2[50];
	volatile uint32_t RT_CTL;               /*!< (@ 0x00000400) Rotate Control Register                              */
	volatile uint32_t RT_IMG_SIZE;          /*!< (@ 0x00000404) Rotate Image Size Register                           */
	volatile uint32_t RT_SRC_ADDR;          /*!< (@ 0x00000408) Rotate Source Mem Start Register                     */
	volatile uint32_t RT_SRC_STRIDE;        /*!< (@ 0x0000040C) Rotate Source Mem Stride Register                    */
	volatile uint32_t RT_DST_ADDR;          /*!< (@ 0x00000410) Rotate Dest Mem Start Register                       */
	volatile uint32_t RT_DST_STRIDE;        /*!< (@ 0x00000414) Rotate Dest Mem Stride Register                      */
	volatile uint32_t RT_START_HEIGHT;      /*!< (@ 0x00000418) Rotate Start Height Register                         */
	volatile uint32_t RT_SW_X_XY;           /*!< (@ 0x0000041C) Rotate Source Delta X/Y along Dest X Register        */
	volatile uint32_t RT_SW_Y_XY;           /*!< (@ 0x00000420) Rotate Source Delta X/Y along Dest Y Register        */
	volatile uint32_t RT_SW_X0;             /*!< (@ 0x00000424) Rotate Source Start X mapping to Dest (0,0) Register */
	volatile uint32_t RT_SW_Y0;             /*!< (@ 0x00000428) Rotate Source Start Y mapping to Dest (0,0) Register */
	volatile uint32_t RT_SW_FIRST_DIST;     /*!< (@ 0x0000042C) Rotate Source First Distance Register                */
	volatile uint32_t RT_R1M2;              /*!< (@ 0x00000430) Rotate Source Outer Radius Square Register           */
	volatile uint32_t RT_R0M2;              /*!< (@ 0x00000430) Rotate Source Inner Radius Square Register           */
	volatile uint32_t RT_FILL_COLOR;        /*!< (@ 0x00000438) Rotate Fill Color Register                           */
	const volatile uint32_t RESERVED3[49];
	volatile uint32_t RT_RESULT_X0;         /*!< (@ 0x00000500) Rotate Result of RT_SW_X0 Register                   */
	volatile uint32_t RT_RESULT_Y0;         /*!< (@ 0x00000504) Rotate Result of RT_SW_Y0 Register                   */
	volatile uint32_t RT_RESULT_FIRST_DIST; /*!< (@ 0x00000508) Rotate Result of RT_SW_FIRST_DIST Register           */
	volatile uint32_t RT_RESULT_SRC_ADDR;   /*!< (@ 0x0000050C) Rotate Result of RT_SRC_ADDR Register                */
} __attribute__((__packed__)) DE_Type;      /*!< Size = 1048 (0x418)                                                 */

/**
  * @brief DE Command Configuration
  */
typedef struct de_overlay_cfg {
	uint32_t num_ovls : 2;
	display_layer_t ovls[2];
	display_buffer_t bufs[2];
	display_buffer_t target;
	display_rect_t target_rect;
} de_overlay_cfg_t;

typedef struct {
	uint32_t ctl;
	uint32_t img_size;
	uint32_t src_addr;
	uint32_t src_stride;
	uint32_t dst_addr;
	uint32_t dst_stride;
	uint32_t start_height;
	uint32_t sw_x_xy;
	uint32_t sw_y_xy;
	int32_t sw_x0;
	int32_t sw_y0;
	uint32_t sw_first_dist;
	uint32_t r1m2;
	uint32_t r0m2;
	uint32_t fill_color;
} __attribute__((__packed__)) de_rotate_cfg_t;

#endif /* ZEPHYR_DRIVERS_DISPLAY_ENGINE_DE_LARK_H_ */
