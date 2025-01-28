/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_DISPLAY_ENGINE_DE_LEOPARD_H_
#define ZEPHYR_DRIVERS_DISPLAY_ENGINE_DE_LEOPARD_H_

#include <zephyr/types.h>
#include <sys/util.h>

/* Reg DE_CTL */
#define DE_CTL_OUT_FORMAT_SEL(x)              ((uint32_t)(x) << 2)
#define DE_CTL_OUT_FORMAT_RGB565              DE_CTL_OUT_FORMAT_SEL(0)
#define DE_CTL_OUT_FORMAT_RGB666              DE_CTL_OUT_FORMAT_SEL(1)
#define DE_CTL_OUT_FORMAT_RGB888              DE_CTL_OUT_FORMAT_SEL(2)
#define DE_CTL_OUT_FORMAT_RGB888_WB_ARGB8888  DE_CTL_OUT_FORMAT_SEL(3)
#define DE_CTL_OUT_FORMAT_RGB888_WB_XRGB8888  (DE_CTL_OUT_FORMAT_SEL(3) | BIT(13))

#define DE_CTL_OUT_COLOR_FILL_EN              (0x1 << 5)
#define DE_CTL_OUT_DISPLY_YFLIP_EN(en)        ((uint32_t)(en) << 6)
#define DE_CTL_OUT_WB_YFLIP_EN(en)            ((uint32_t)(en) << 7)

#define DE_CTL_OUT_MODE_SEL(x)                ((uint32_t)(x) << 8)
#define DE_CTL_OUT_MODE_MASK                  DE_CTL_OUT_MODE_SEL(3)
#define DE_CTL_OUT_MODE_DISPLAY               DE_CTL_OUT_MODE_SEL(0)
#define DE_CTL_OUT_MODE_DISPLAY_WB            DE_CTL_OUT_MODE_SEL(1)
#define DE_CTL_OUT_MODE_WB                    DE_CTL_OUT_MODE_SEL(2)

#define DE_CTL_TRANSFER_MODE_SEL(x)           ((uint32_t)(x) << 12)
#define DE_CTL_TRANSFER_MODE_MASK             DE_CTL_TRANSFER_MODE_SEL(1)
#define DE_CTL_TRANSFER_MODE_TRIGGER          DE_CTL_TRANSFER_MODE_SEL(0)
#define DE_CTL_TRANSFER_MODE_CONTINUE         DE_CTL_TRANSFER_MODE_SEL(1)

#define DE_CTL_WB_NO_STRIDE_EN(en)            ((uint32_t)(en) << 29)

#define DE_FORMAT_RGB565          (0)
#define DE_FORMAT_RGB666          (1)
#define DE_FORMAT_RGB888          (2)
#define DE_FORMAT_ARGB            (3)
#define DE_FORMAT_BGRA            (4)
#define DE_FORMAT_AX              (5)
#define DE_FORMAT_ARGB8565        (6)
#define DE_FORMAT_ABMP_INDEX      (7)

/* L0 layer */
#define DE_L0_EN                    BIT(20)
#define DE_L0_COLOR_FILL_EN(en)     (0)
#define DE_L0_NO_STRIDE_EN(en)      ((uint32_t)(en) << 31)
#define DE_L0_FORMAT(de_fmt, rb_swap, byte_swap, is_6666, alpha_off)       \
		((de_fmt << 15) | (rb_swap << 18) | (!byte_swap << 14) | (is_6666 << 19) | (alpha_off << 10))

#define DE_L0_FORMAT_ARGB_1555    DE_L0_FORMAT(DE_FORMAT_RGB565, 0, 0, 1, 0)
#define DE_L0_FORMAT_RGB_565      DE_L0_FORMAT(DE_FORMAT_RGB565, 0, 0, 0, 0)
#define DE_L0_FORMAT_BGR_565      DE_L0_FORMAT(DE_FORMAT_RGB565, 1, 0, 0, 0)
#define DE_L0_FORMAT_RGB_565_SWAP DE_L0_FORMAT(DE_FORMAT_RGB565, 0, 1, 0, 0)
#define DE_L0_FORMAT_BGR_565_SWAP DE_L0_FORMAT(DE_FORMAT_RGB565, 1, 1, 0, 0)
#define DE_L0_FORMAT_RGB_666      DE_L0_FORMAT(DE_FORMAT_RGB666, 0, 0, 0, 0)
#define DE_L0_FORMAT_BGR_666      DE_L0_FORMAT(DE_FORMAT_RGB666, 1, 0, 0, 0)
#define DE_L0_FORMAT_RGB_888      DE_L0_FORMAT(DE_FORMAT_RGB888, 0, 0, 0, 0)
#define DE_L0_FORMAT_BGR_888      DE_L0_FORMAT(DE_FORMAT_RGB888, 1, 0, 0, 0)

#define DE_L0_FORMAT_ARGB_8888    DE_L0_FORMAT(DE_FORMAT_ARGB, 0, 0, 0, 0)
#define DE_L0_FORMAT_ABGR_8888    DE_L0_FORMAT(DE_FORMAT_ARGB, 1, 0, 0, 0)
#define DE_L0_FORMAT_BGRA_8888    DE_L0_FORMAT(DE_FORMAT_BGRA, 0, 0, 0, 0)
#define DE_L0_FORMAT_RGBA_8888    DE_L0_FORMAT(DE_FORMAT_BGRA, 1, 0, 0, 0)
#define DE_L0_FORMAT_XRGB_8888    DE_L0_FORMAT(DE_FORMAT_ARGB, 0, 0, 0, 1)
#define DE_L0_FORMAT_XBGR_8888    DE_L0_FORMAT(DE_FORMAT_ARGB, 1, 0, 0, 1)
#define DE_L0_FORMAT_BGRX_8888    DE_L0_FORMAT(DE_FORMAT_BGRA, 0, 0, 0, 1)
#define DE_L0_FORMAT_RGBX_8888    DE_L0_FORMAT(DE_FORMAT_BGRA, 1, 0, 0, 1)

#define DE_L0_FORMAT_ARGB_6666    DE_L0_FORMAT(DE_FORMAT_ARGB, 0, 0, 1, 0)
#define DE_L0_FORMAT_ABGR_6666    DE_L0_FORMAT(DE_FORMAT_ARGB, 1, 0, 1, 0)
#define DE_L0_FORMAT_BGRA_6666    DE_L0_FORMAT(DE_FORMAT_BGRA, 0, 0, 1, 0)
#define DE_L0_FORMAT_RGBA_6666    DE_L0_FORMAT(DE_FORMAT_BGRA, 1, 0, 1, 0)
#define DE_L0_FORMAT_XRGB_6666    DE_L0_FORMAT(DE_FORMAT_ARGB, 0, 0, 1, 1)
#define DE_L0_FORMAT_XBGR_6666    DE_L0_FORMAT(DE_FORMAT_ARGB, 1, 0, 1, 1)
#define DE_L0_FORMAT_BGRX_6666    DE_L0_FORMAT(DE_FORMAT_BGRA, 0, 0, 1, 1)
#define DE_L0_FORMAT_RGBX_6666    DE_L0_FORMAT(DE_FORMAT_BGRA, 1, 0, 1, 1)

#define DE_L0_FORMAT_ARGB_8565    DE_L0_FORMAT(DE_FORMAT_ARGB8565, 0, 0, 0, 0)
#define DE_L0_FORMAT_ABGR_8565    DE_L0_FORMAT(DE_FORMAT_ARGB8565, 1, 0, 0, 0)
#define DE_L0_FORMAT_XRGB_8565    DE_L0_FORMAT(DE_FORMAT_ARGB8565, 0, 0, 0, 1)
#define DE_L0_FORMAT_XBGR_8565    DE_L0_FORMAT(DE_FORMAT_ARGB8565, 1, 0, 0, 1)

#define DE_L0_FORMAT_AX          DE_L0_FORMAT(DE_FORMAT_AX, 0, 0, 0, 0)
#define DE_L0_FORMAT_ABMP_INDEX  DE_L0_FORMAT(DE_FORMAT_ABMP_INDEX, 0, 0, 0, 0)

/* L1 layer */
#define DE_L1_EN                 BIT(26)
#define DE_L1_SC_EN              (0x1)
#define DE_L1_RT_EN              (0x2)
#define DE_L1_COLOR_FILL_EN(en)  ((uint32_t)(en) << 28)
#define DE_L1_NO_STRIDE_EN(en)   ((uint32_t)(en) << 30)
#define DE_L1_FORMAT(de_fmt, rb_swap, byte_swap, is_6666, alpha_off)       \
		((de_fmt << 21) | (rb_swap << 24) | (!byte_swap << 27) | (is_6666 << 25) | (alpha_off << 11))

#define DE_L1_FORMAT_ARGB_1555    DE_L1_FORMAT(DE_FORMAT_RGB565, 0, 0, 1, 0)
#define DE_L1_FORMAT_RGB_565      DE_L1_FORMAT(DE_FORMAT_RGB565, 0, 0, 0, 0)
#define DE_L1_FORMAT_BGR_565      DE_L1_FORMAT(DE_FORMAT_RGB565, 1, 0, 0, 0)
#define DE_L1_FORMAT_RGB_565_SWAP DE_L1_FORMAT(DE_FORMAT_RGB565, 0, 1, 0, 0)
#define DE_L1_FORMAT_BGR_565_SWAP DE_L1_FORMAT(DE_FORMAT_RGB565, 1, 1, 0, 0)
#define DE_L1_FORMAT_RGB_666      DE_L1_FORMAT(DE_FORMAT_RGB666, 0, 0, 0, 0)
#define DE_L1_FORMAT_BGR_666      DE_L1_FORMAT(DE_FORMAT_RGB666, 1, 0, 0, 0)
#define DE_L1_FORMAT_RGB_888      DE_L1_FORMAT(DE_FORMAT_RGB888, 0, 0, 0, 0)
#define DE_L1_FORMAT_BGR_888      DE_L1_FORMAT(DE_FORMAT_RGB888, 1, 0, 0, 0)

#define DE_L1_FORMAT_ARGB_8888    DE_L1_FORMAT(DE_FORMAT_ARGB, 0, 0, 0, 0)
#define DE_L1_FORMAT_ABGR_8888    DE_L1_FORMAT(DE_FORMAT_ARGB, 1, 0, 0, 0)
#define DE_L1_FORMAT_BGRA_8888    DE_L1_FORMAT(DE_FORMAT_BGRA, 0, 0, 0, 0)
#define DE_L1_FORMAT_RGBA_8888    DE_L1_FORMAT(DE_FORMAT_BGRA, 1, 0, 0, 0)
#define DE_L1_FORMAT_XRGB_8888    DE_L1_FORMAT(DE_FORMAT_ARGB, 0, 0, 0, 1)
#define DE_L1_FORMAT_XBGR_8888    DE_L1_FORMAT(DE_FORMAT_ARGB, 1, 0, 0, 1)
#define DE_L1_FORMAT_BGRX_8888    DE_L1_FORMAT(DE_FORMAT_BGRA, 0, 0, 0, 1)
#define DE_L1_FORMAT_RGBX_8888    DE_L1_FORMAT(DE_FORMAT_BGRA, 1, 0, 0, 1)

#define DE_L1_FORMAT_ARGB_6666    DE_L1_FORMAT(DE_FORMAT_ARGB, 0, 0, 1, 0)
#define DE_L1_FORMAT_ABGR_6666    DE_L1_FORMAT(DE_FORMAT_ARGB, 1, 0, 1, 0)
#define DE_L1_FORMAT_BGRA_6666    DE_L1_FORMAT(DE_FORMAT_BGRA, 0, 0, 1, 0)
#define DE_L1_FORMAT_RGBA_6666    DE_L1_FORMAT(DE_FORMAT_BGRA, 1, 0, 1, 0)
#define DE_L1_FORMAT_XRGB_6666    DE_L1_FORMAT(DE_FORMAT_ARGB, 0, 0, 1, 1)
#define DE_L1_FORMAT_XBGR_6666    DE_L1_FORMAT(DE_FORMAT_ARGB, 1, 0, 1, 1)
#define DE_L1_FORMAT_BGRX_6666    DE_L1_FORMAT(DE_FORMAT_BGRA, 0, 0, 1, 1)
#define DE_L1_FORMAT_RGBX_6666    DE_L1_FORMAT(DE_FORMAT_BGRA, 1, 0, 1, 1)

#define DE_L1_FORMAT_ARGB_8565    DE_L1_FORMAT(DE_FORMAT_ARGB8565, 0, 0, 0, 0)
#define DE_L1_FORMAT_ABGR_8565    DE_L1_FORMAT(DE_FORMAT_ARGB8565, 1, 0, 0, 0)
#define DE_L1_FORMAT_XRGB_8565    DE_L1_FORMAT(DE_FORMAT_ARGB8565, 0, 0, 0, 1)
#define DE_L1_FORMAT_XBGR_8565    DE_L1_FORMAT(DE_FORMAT_ARGB8565, 1, 0, 0, 1)

#define DE_L1_FORMAT_AX          DE_L1_FORMAT(DE_FORMAT_AX, 0, 0, 0, 0)
#define DE_L1_FORMAT_ABMP_INDEX  DE_L1_FORMAT(DE_FORMAT_ABMP_INDEX, 0, 0, 0, 0)

/* Reg DE_CTL1 */
/* L2 layer */
#define DE_L2_EN                 BIT(26)
#define DE_L2_SC_EN              (0x1)
#define DE_L2_RT_EN              (0x2)
#define DE_L2_COLOR_FILL_EN(en)  ((uint32_t)(en) << 28)
#define DE_L2_NO_STRIDE_EN(en)   ((uint32_t)(en) << 30)
#define DE_L2_FORMAT(de_fmt, rb_swap, byte_swap, is_6666, alpha_off)       \
		((de_fmt << 21) | (rb_swap << 24) | (!byte_swap << 27) | (is_6666 << 25) | (alpha_off << 11))

#define DE_L2_FORMAT_ARGB_1555    DE_L1_FORMAT(DE_FORMAT_RGB565, 0, 0, 1, 0)
#define DE_L2_FORMAT_RGB_565      DE_L2_FORMAT(DE_FORMAT_RGB565, 0, 0, 0, 0)
#define DE_L2_FORMAT_BGR_565      DE_L2_FORMAT(DE_FORMAT_RGB565, 1, 0, 0, 0)
#define DE_L2_FORMAT_RGB_565_SWAP DE_L2_FORMAT(DE_FORMAT_RGB565, 0, 1, 0, 0)
#define DE_L2_FORMAT_BGR_565_SWAP DE_L2_FORMAT(DE_FORMAT_RGB565, 1, 1, 0, 0)
#define DE_L2_FORMAT_RGB_666      DE_L2_FORMAT(DE_FORMAT_RGB666, 0, 0, 0, 0)
#define DE_L2_FORMAT_BGR_666      DE_L2_FORMAT(DE_FORMAT_RGB666, 1, 0, 0, 0)
#define DE_L2_FORMAT_RGB_888      DE_L2_FORMAT(DE_FORMAT_RGB888, 0, 0, 0, 0)
#define DE_L2_FORMAT_BGR_888      DE_L2_FORMAT(DE_FORMAT_RGB888, 1, 0, 0, 0)

#define DE_L2_FORMAT_ARGB_8888    DE_L2_FORMAT(DE_FORMAT_ARGB, 0, 0, 0, 0)
#define DE_L2_FORMAT_ABGR_8888    DE_L2_FORMAT(DE_FORMAT_ARGB, 1, 0, 0, 0)
#define DE_L2_FORMAT_BGRA_8888    DE_L2_FORMAT(DE_FORMAT_BGRA, 0, 0, 0, 0)
#define DE_L2_FORMAT_RGBA_8888    DE_L2_FORMAT(DE_FORMAT_BGRA, 1, 0, 0, 0)
#define DE_L2_FORMAT_XRGB_8888    DE_L2_FORMAT(DE_FORMAT_ARGB, 0, 0, 0, 1)
#define DE_L2_FORMAT_XBGR_8888    DE_L2_FORMAT(DE_FORMAT_ARGB, 1, 0, 0, 1)
#define DE_L2_FORMAT_BGRX_8888    DE_L2_FORMAT(DE_FORMAT_BGRA, 0, 0, 0, 1)
#define DE_L2_FORMAT_RGBX_8888    DE_L2_FORMAT(DE_FORMAT_BGRA, 1, 0, 0, 1)

#define DE_L2_FORMAT_ARGB_6666    DE_L2_FORMAT(DE_FORMAT_ARGB, 0, 0, 1, 0)
#define DE_L2_FORMAT_ABGR_6666    DE_L2_FORMAT(DE_FORMAT_ARGB, 1, 0, 1, 0)
#define DE_L2_FORMAT_BGRA_6666    DE_L2_FORMAT(DE_FORMAT_BGRA, 0, 0, 1, 0)
#define DE_L2_FORMAT_RGBA_6666    DE_L2_FORMAT(DE_FORMAT_BGRA, 1, 0, 1, 0)
#define DE_L2_FORMAT_XRGB_6666    DE_L2_FORMAT(DE_FORMAT_ARGB, 0, 0, 1, 1)
#define DE_L2_FORMAT_XBGR_6666    DE_L2_FORMAT(DE_FORMAT_ARGB, 1, 0, 1, 1)
#define DE_L2_FORMAT_BGRX_6666    DE_L2_FORMAT(DE_FORMAT_BGRA, 0, 0, 1, 1)
#define DE_L2_FORMAT_RGBX_6666    DE_L2_FORMAT(DE_FORMAT_BGRA, 1, 0, 1, 1)

#define DE_L2_FORMAT_ARGB_8565    DE_L2_FORMAT(DE_FORMAT_ARGB8565, 0, 0, 0, 0)
#define DE_L2_FORMAT_ABGR_8565    DE_L2_FORMAT(DE_FORMAT_ARGB8565, 1, 0, 0, 0)
#define DE_L2_FORMAT_XRGB_8565    DE_L2_FORMAT(DE_FORMAT_ARGB8565, 0, 0, 0, 1)
#define DE_L2_FORMAT_XBGR_8565    DE_L2_FORMAT(DE_FORMAT_ARGB8565, 1, 0, 0, 1)

#define DE_L2_FORMAT_AX          DE_L2_FORMAT(DE_FORMAT_AX, 0, 0, 0, 0)
#define DE_L2_FORMAT_ABMP_INDEX  DE_L2_FORMAT(DE_FORMAT_ABMP_INDEX, 0, 0, 0, 0)

/* Reg DE_CTL2 */
#define DE_CTL2_START              BIT(0)
#define DE_CTL2_OUT_FIFO_RESET     BIT(1)
#define DE_CTL2_WB_FIFO_RESET      BIT(2)
#define DE_CTL2_L0_FIFO_RESET      BIT(3)
#define DE_CTL2_L1_FIFO_RESET      BIT(4)
#define DE_CTL2_L2_FIFO_RESET      BIT(5)
#define DE_CTL2_RGB_CVT_SEL(x)    ((uint32_t)(x) << 8)
#define DE_CTL2_RGB_CVT_MASK      DE_CTL2_RGB_CVT_SEL(3)
#define DE_CTL2_RGB_CVT_LOW       DE_CTL2_RGB_CVT_SEL(0) /* Low bits compensation */
#define DE_CTL2_RGB_CVT_ZERO      DE_CTL2_RGB_CVT_SEL(1) /* Zero bits compensation */
#define DE_CTL2_RGB_CVT_HIGH      DE_CTL2_RGB_CVT_SEL(2) /* High bits compensation */
#define DE_CTL2_RGB_CVT_CUT       DE_CTL2_RGB_CVT_SEL(3) /* Cut to the same bits, then zero compensation */

/* Reg DE_GAT_CTL */
#define DE_LX_GAT_EN      BIT(0)
#define DE_OUT_GAT_EN     BIT(1)
#define DE_PATH_GAT_EN    BIT(3)
#define DE_OVERLAY_GAT_EN (DE_LX_GAT_EN | DE_OUT_GAT_EN | DE_PATH_GAT_EN)
#define DE_ROTATE_GAT_EN  BIT(4)

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
#define DE_IRQ_L0_FIFO_OVF      BIT(2) /* over flow*/
#define DE_IRQ_L1_FIFO_UDF      BIT(3)
#define DE_IRQ_L1_FIFO_OVF      BIT(4)
#define DE_IRQ_L0_FTC           BIT(5) /* frame transfer complete */
#define DE_IRQ_L1_FTC           BIT(6)
#define DE_IRQ_L2_FTC           BIT(6)
#define DE_IRQ_WB_FIFO_UDF      BIT(8)
#define DE_IRQ_WB_FIFO_OVF      BIT(9)
#define DE_IRQ_DEV_FIFO_UDF     BIT(10)
#define DE_IRQ_DEV_FIFO_OVF     BIT(11)
#define DE_IRQ_L2_FIFO_UDF      BIT(12)
#define DE_IRQ_L2_FIFO_OVF      BIT(13)
#define DE_IRQ_VSYNC            BIT(16)
#define DE_IRQ_DEV_FIFO_HF      BIT(29) /* half full */
#define DE_IRQ_WB_FTC           BIT(30)
#define DE_IRQ_DEV_FTC          BIT(31)

/* Reg DE_STA */
#define DE_STAT_L0_MEM_ERR        BIT(0)
#define DE_STAT_L1_MEM_ERR        BIT(1)
#define DE_STAT_L2_MEM_ERR        BIT(2)
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
#define DE_STAT_L2_FIFO_EMPTY     BIT(17)
#define DE_STAT_L2_FIFO_FULL      BIT(18)
#define DE_STAT_L2_FIFO_UDF       BIT(19)
#define DE_STAT_L2_FIFO_OVF       BIT(20)
#define DE_STAT_L2_FTC            BIT(21)
#define DE_STAT_L1_FIFO_EMPTY     BIT(23)
#define DE_STAT_L1_FIFO_FULL      BIT(24)
#define DE_STAT_L1_FIFO_UDF       BIT(25)
#define DE_STAT_L1_FIFO_OVF       BIT(26)
#define DE_STAT_L1_FTC            BIT(27)
#define DE_STAT_PRELINE           BIT(28)
#define DE_STAT_DEV_FIFO_HF       BIT(29)
#define DE_STAT_WB_FTC            BIT(30)
#define DE_STAT_DEV_FTC           BIT(31)

/* Reg DE_BG_SIZE */
#define DE_BG_SIZE(w, h)      ((((uint32_t)(h) - 1) << 16) | ((w) - 1))
/* Reg DE_BG_COLOR */
#define DE_BG_COLOR(r, g, b)  (((uint32_t)(r) << 16) | ((uint32_t)(g) << 8) | (b))

/* Reg DE_LX_POS */
#define DE_LX_POS(x, y)             (((uint32_t)(y) << 16) | (x))
/* Reg DE_LX_SIZE */
#define DE_LX_SIZE(w, h)            ((((uint32_t)(h) - 1) << 16) | ((w) - 1))
/* Reg DE_LX_DEF_COLOR */
#define DE_LX_DEF_COLOR(a, r, g, b)                                             \
	(((uint32_t)(a) << 24) | ((uint32_t)(r) << 16) | ((uint32_t)(g) << 8) | (b))
/* Reg DE_LX_RT_SRC_SIZE */
#define DE_LX_RT_SRC_SIZE(w, h)     ((((uint32_t)(h) - 1) << 16) | ((w) - 1))
/* Reg DE_LX_RT_SW_X_XY & DE_LX_RT_SW_Y_XY */
#define DE_LX_RT_DELTA_XY(dx, dy)   (((uint32_t)(dy)) << 16) | (((uint32_t)(dx) & 0xFFFF))

/* Reg DE_ALPHA_CTL */
#define DE_ALPHA_EN               BIT(31)
#define DE_ALPHA_COVERAGE         (0x0 << 10)
#define DE_ALPHA_PREMULTIPLIED    (0x1 << 10)
#define DE_ALPHA_FCV_EN           BIT(8)
#define DE_ALPHA_PLANE_ALPHA(x)   ((x) & 0xff)
/* Reg DE_ALPHA_POS */
#define DE_ALPHA_POS(x, y)   (((uint32_t)(y) << 16) | (x))
/* Reg DE_ALPHA_SIZE */
#define DE_ALPHA_SIZE(w, h)  ((((uint32_t)(h) - 1) << 16) | ((w) - 1))

/* Reg DE_FILL_COLOR */
#define DE_FILL_COLOR(a, r, g, b)                                              \
	(((uint32_t)(a) << 24) | ((uint32_t)(r) << 16) | ((uint32_t)(g) << 8) | (b))
/* Reg DE_COLOR_FILL_POS */
#define DE_COLOR_FILL_POS(x, y)   (((uint32_t)(y) << 16) | (x))
/* Reg DE_COLOR_FILL_SIZE */
#define DE_COLOR_FILL_SIZE(w, h)  ((((uint32_t)(h) - 1) << 16) | ((w) - 1))

/* Reg DE_AX_COLOR */
#define DE_AX_TYPE_SEL(x)     ((uint32_t)(x) << 24)
#define DE_AX_8               DE_AX_TYPE_SEL(0)
#define DE_AX_4               DE_AX_TYPE_SEL(1)
#define DE_AX_1               DE_AX_TYPE_SEL(2)
#define DE_AX_COLOR(r, g, b)  (((uint32_t)(r) << 16) | ((uint32_t)(g) << 8) | (b))
/* Reg DE_AX_CTL */
#define DE_A14_FONT(size, stride, total)                                      \
	(((uint32_t)((size) - 1) << 20) | ((uint32_t)((stride) - 1) << 10) | ((total) - 1))

/* Reg DE_CLUT_CTL */
#define DE_CLUT_TAB_SEL(x)     ((uint32_t)(x) << 8)
#define DE_CLUT_TAB_IDX(x)     ((uint32_t)(x))
/* Reg DE_CLUT_DATA */
#define DE_CLUT_COLOR(a, r, g, b)                                              \
	(((uint32_t)(a) << 24) | ((uint32_t)(r) << 16) | ((uint32_t)(g) << 8) | (b))

/* Reg DE_LX_ABMP_CTL */
#define DE_L0_ABMP_BITOFS(bits)                (((uint32_t)(bits)) << 4) /* for 1/2/4 bpp */
#define DE_L0_ALPHA_AA_EN                      (1 << 7)
#define DE_L0_ABMP(type, big_endian, indexed)  ((type << 0) | (big_endian << 2) | (indexed << 3))
#define DE_L0_ALPHA_8                          DE_L0_ABMP(0, 0, 0)
#define DE_L0_ALPHA_4                          DE_L0_ABMP(1, 1, 0)
#define DE_L0_ALPHA_2                          (DE_L0_ABMP(3, 1, 0) | DE_L0_ALPHA_AA_EN)
#define DE_L0_ALPHA_1                          (DE_L0_ABMP(2, 1, 0) | DE_L0_ALPHA_AA_EN)
#define DE_L0_ALPHA_8_LE                       DE_L0_ABMP(0, 0, 0)
#define DE_L0_ALPHA_4_LE                       DE_L0_ABMP(1, 0, 0)
#define DE_L0_ALPHA_2_LE                       (DE_L0_ABMP(3, 0, 0) | DE_L0_ALPHA_AA_EN)
#define DE_L0_ALPHA_1_LE                       (DE_L0_ABMP(2, 0, 0) | DE_L0_ALPHA_AA_EN)
#define DE_L0_INDEX_8                          DE_L0_ABMP(0, 0, 1)
#define DE_L0_INDEX_4                          DE_L0_ABMP(1, 1, 1)
#define DE_L0_INDEX_2                          DE_L0_ABMP(3, 1, 1)
#define DE_L0_INDEX_1                          DE_L0_ABMP(2, 1, 1)
#define DE_L0_INDEX_8_LE                       DE_L0_ABMP(0, 0, 1)
#define DE_L0_INDEX_4_LE                       DE_L0_ABMP(1, 0, 1)
#define DE_L0_INDEX_2_LE                       DE_L0_ABMP(3, 0, 1)
#define DE_L0_INDEX_1_LE                       DE_L0_ABMP(2, 0, 1)

#define DE_L1_ABMP_BITOFS(bits)                (((uint32_t)(bits)) << 12) /* for 1/2/4 bpp */
#define DE_L1_ALPHA_AA_EN                      (1 << 15)
#define DE_L1_ABMP(type, big_endian, indexed)  ((type << 8) | (big_endian << 10) | (indexed << 11))
#define DE_L1_ALPHA_8                          DE_L1_ABMP(0, 0, 0)
#define DE_L1_ALPHA_4                          DE_L1_ABMP(1, 1, 0)
#define DE_L1_ALPHA_2                          (DE_L1_ABMP(3, 1, 0) | DE_L1_ALPHA_AA_EN)
#define DE_L1_ALPHA_1                          (DE_L1_ABMP(2, 1, 0) | DE_L1_ALPHA_AA_EN)
#define DE_L1_ALPHA_8_LE                       DE_L1_ABMP(0, 0, 0)
#define DE_L1_ALPHA_4_LE                       DE_L1_ABMP(1, 0, 0)
#define DE_L1_ALPHA_2_LE                       (DE_L1_ABMP(3, 0, 0) | DE_L1_ALPHA_AA_EN)
#define DE_L1_ALPHA_1_LE                       (DE_L1_ABMP(2, 0, 0) | DE_L1_ALPHA_AA_EN)
#define DE_L1_INDEX_8                          DE_L1_ABMP(0, 0, 1)
#define DE_L1_INDEX_4                          DE_L1_ABMP(1, 1, 1)
#define DE_L1_INDEX_2                          DE_L1_ABMP(3, 1, 1)
#define DE_L1_INDEX_1                          DE_L1_ABMP(2, 1, 1)
#define DE_L1_INDEX_8_LE                       DE_L1_ABMP(0, 0, 1)
#define DE_L1_INDEX_4_LE                       DE_L1_ABMP(1, 0, 1)
#define DE_L1_INDEX_2_LE                       DE_L1_ABMP(3, 0, 1)
#define DE_L1_INDEX_1_LE                       DE_L1_ABMP(2, 0, 1)

#define DE_L2_ABMP_BITOFS(bits)                (((uint32_t)(bits)) << 20) /* for 1/2/4 bpp */
#define DE_L2_ALPHA_AA_EN                      (1 << 23)
#define DE_L2_ABMP(type, big_endian, indexed)  ((type << 16) | (big_endian << 18) | (indexed << 19))
#define DE_L2_ALPHA_8                          DE_L2_ABMP(0, 0, 0)
#define DE_L2_ALPHA_4                          DE_L2_ABMP(1, 1, 0)
#define DE_L2_ALPHA_2                          (DE_L2_ABMP(3, 1, 0) | DE_L2_ALPHA_AA_EN)
#define DE_L2_ALPHA_1                          (DE_L2_ABMP(2, 1, 0) | DE_L2_ALPHA_AA_EN)
#define DE_L2_ALPHA_8_LE                       DE_L2_ABMP(0, 0, 0)
#define DE_L2_ALPHA_4_LE                       DE_L2_ABMP(1, 0, 0)
#define DE_L2_ALPHA_2_LE                       (DE_L2_ABMP(3, 0, 0) | DE_L2_ALPHA_AA_EN)
#define DE_L2_ALPHA_1_LE                       (DE_L2_ABMP(2, 0, 0) | DE_L2_ALPHA_AA_EN)
#define DE_L2_INDEX_8                          DE_L2_ABMP(0, 0, 1)
#define DE_L2_INDEX_4                          DE_L2_ABMP(1, 1, 1)
#define DE_L2_INDEX_2                          DE_L2_ABMP(3, 1, 1)
#define DE_L2_INDEX_1                          DE_L2_ABMP(2, 1, 1)
#define DE_L2_INDEX_8_LE                       DE_L2_ABMP(0, 0, 1)
#define DE_L2_INDEX_4_LE                       DE_L2_ABMP(1, 0, 1)
#define DE_L2_INDEX_2_LE                       DE_L2_ABMP(3, 0, 1)
#define DE_L2_INDEX_1_LE                       DE_L2_ABMP(2, 0, 1)

#define DE_LX_AA_P0_COFF_1_2                   (0)
#define DE_LX_AA_P0_COFF_3_4                   BIT(24)

/* Reg RT_CTL */
#define RT_MODE_SEL(x)              (x)
#define RT_MODE_CIRCLE              RT_MODE_SEL(0)
#define RT_MODE_RECT                RT_MODE_SEL(1)
#define RT_AUTOLOAD_EN              BIT(1)
#define RT_COLOR_FILL_EN            BIT(2)
#define RT_IRQ_EN                   BIT(4)
#define RT_STAT_COMPLETE            BIT(7)
#define RT_FILTER_SEL(x)            ((uint32_t)(x) << 8)
#define RT_FILTER_LINEAR            RT_FILTER_SEL(0)
#define RT_FILTER_BILINEAR          RT_FILTER_SEL(1)
#define RT_COLOR_FILL_START         BIT(16) /* used for RGB-565 fast clear */
#define RT_COLOR_FILL_BURST_LEN(x)  ((uint32_t)(x) << 17)
#define RT_COLOR_FILL_BURST_8       RT_COLOR_FILL_BURST_LEN(0)
#define RT_COLOR_FILL_BURST_16      RT_COLOR_FILL_BURST_LEN(1)
#define RT_FORMAT_SEL(x)            ((uint32_t)(x) << 28)
#define RT_FORMAT_RGB565_SWAP       RT_FORMAT_SEL(0)
#define RT_FORMAT_RGB565            RT_FORMAT_SEL(1)
#define RT_FORMAT_ARGB8888          RT_FORMAT_SEL(2)
#define RT_EN                       BIT(31)

/* Reg RT_IMG_SIZE */
#define RT_END_HEIGHT(x)        ((uint32_t)(x) << 16)
#define RT_IMG_WIDTH(x)         ((x) & 0x1ff)

/* Reg RT_IMG_START_HEIGHT */
#define RT_START_HEIGHT(x)      ((x) & 0x1ff)

/* Reg RT_SW_X_XY & RT_SW_Y_XY */
#define RT_SW_DELTA_XY(dx, dy)  (((uint32_t)(dy)) << 16) | (((uint32_t)(dx) & 0xFFFF))

/* Reg RT_FILL_COLOR */
#define RT_COLOR_ARGB_8888(a, r, g, b)                                              \
	(((uint32_t)(a) << 24) | ((uint32_t)(r) << 16) | ((uint32_t)(g) << 8) | (b))
#define RT_COLOR_RGB_565(r, g, b)                                              \
	((((uint32_t)(r) >> 3) << 11) | (((uint32_t)(g) >> 2) << 5) | ((b) >> 3))

/* Reg RT_SRC_IMG_SIZE */
#define RT_SRC_IMG_SIZE(w, h)  ((((uint32_t)(h)) << 16) | (w))

/* Reg SC_LX_SRC_IMG_SIZE */
#define SC_LX_SRC_IMG_SIZE(w, h)   ((((uint32_t)(h) - 1) << 16) | ((w) - 1))
/* Reg SC_LX_RATE */
#define SC_LX_RATE(hor, ver)       ((((uint32_t)(ver)) << 16) | (hor))
#define SC_LX_RATE_SAME            SC_RATE(4096, 4096)
/* Reg SC_LX_POS */
#define SC_LX_POS(x, y)         ((((uint32_t)(y)) << 16) | (x))

/**
  * @brief DE Module (DE)
  */
typedef struct {
	volatile uint32_t POS;          /*!< (@ 0x00000000) Layer Start Position Register                                */
	volatile uint32_t SIZE;         /*!< (@ 0x00000004) Layer Size Register                                          */
	volatile uint32_t ADDR;         /*!< (@ 0x00000008) Layer Mem Start Address Register                             */
	volatile uint32_t STRIDE;       /*!< (@ 0x0000000C) Layer Mem Stride Register                                    */
	volatile uint32_t LENGTH;       /*!< (@ 0x00000010) Layer Mem Transfer Length Register                           */
	const volatile uint32_t RESERVED1[2];
	volatile uint32_t DEF_COLOR;    /*!< (@ 0x0000001C) Layer Default Color Register                                 */
	volatile uint32_t RT_SRC_SIZE;  /*!< (@ 0x00000020) Layer Rotation Source Size Register                          */
	volatile uint32_t RT_SW_X_XY;   /*!< (@ 0x00000024) Layer Rotation SW X Register                                 */
	volatile uint32_t RT_SW_Y_XY;   /*!< (@ 0x00000028) Layer Rotation SW Y Register                                 */
	volatile uint32_t RT_SW_X0;     /*!< (@ 0x0000002C) Layer Rotation SW X0 Register                                */
	volatile uint32_t RT_SW_Y0;     /*!< (@ 0x00000030) Layer Rotation SW Y0 Register                                */
	const volatile uint32_t RESERVED2[51];
} __attribute__((__packed__)) DE_LAYER_Type;

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
	volatile uint32_t CTL1;                 /*!< (@ 0x00000024) DE Control1 Register                                 */
	const volatile uint32_t RESERVED1[54];
	DE_LAYER_Type LX_CTL[2];                /*!< (@ 0x00000100) DE Layer Control Register                            */
	volatile uint32_t ALPHA_CTL;            /*!< (@ 0x00000300) DE Alpha Control Register                            */
	volatile uint32_t ALPHA_POS;            /*!< (@ 0x00000304) Alpha Position Register                              */
	volatile uint32_t ALPHA_SIZE;           /*!< (@ 0x00000308) Alpha Size Register                                  */
	volatile uint32_t STA;                  /*!< (@ 0x0000030C) DE Status Register                                   */
	const volatile uint32_t RESERVED2[3];
	volatile uint32_t WB_MEM_ADR;           /*!< (@ 0x0000031C) WriteBack Mem Start Address Register                 */
	volatile uint32_t WB_MEM_STRIDE;        /*!< (@ 0x00000320) WriteBack Mem Stride Register                        */
	volatile uint32_t COLOR_FILL_POS;       /*!< (@ 0x00000324) Color Fill Position Register                         */
	volatile uint32_t COLOR_FILL_SIZE;      /*!< (@ 0x00000328) Fill Size Register                                   */
	volatile uint32_t FILL_COLOR;           /*!< (@ 0x0000032C) Fill Color Register                                  */
	volatile uint32_t AX_COLOR;             /*!< (@ 0x00000330) Ax Color Register                                    */
	volatile uint32_t AX_CTL;               /*!< (@ 0x00000334) Ax Control Register                                  */
	volatile uint32_t CLUT_CTL;             /*!< (@ 0x00000338) Color LUT Control Register                           */
	volatile uint32_t CLUT_DATA;            /*!< (@ 0x0000033C) Color LUT Data Register                              */
	volatile uint32_t LX_ABMP_CTL;          /*!< (@ 0x00000340) Layer ABMP Control Register                          */
	const volatile uint32_t RESERVED3[47];
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
	volatile uint32_t RT_R0M2;              /*!< (@ 0x00000434) Rotate Source Inner Radius Square Register           */
	volatile uint32_t RT_FILL_COLOR;        /*!< (@ 0x00000438) Rotate Fill Color Register                           */
	volatile uint32_t RT_SRC_IMG_SIZE;      /*!< (@ 0x0000043C) Rotate Source Image Size                             */
	const volatile uint32_t RESERVED4[48];
	volatile uint32_t RT_RESULT_X0;         /*!< (@ 0x00000500) Rotate Result of RT_SW_X0 Register                   */
	volatile uint32_t RT_RESULT_Y0;         /*!< (@ 0x00000504) Rotate Result of RT_SW_Y0 Register                   */
	volatile uint32_t RT_RESULT_FIRST_DIST; /*!< (@ 0x00000508) Rotate Result of RT_SW_FIRST_DIST Register           */
	volatile uint32_t RESULT_SRC_ADDR;      /*!< (@ 0x0000050C) Rotate Result of RT_SRC_ADDR Register                */
	const volatile uint32_t RESERVED5[60];
	volatile uint32_t SC_LX_SRC_IMG_SIZE;   /*!< (@ 0x00000600) LX Scaling Source Image Register                     */
	volatile uint32_t SC_LX_RATE;           /*!< (@ 0x00000604) LX Scaling Rate Register                             */
	volatile uint32_t SC_LX_POS;            /*!< (@ 0x00000608) LX Scaling Start Position Register                   */
	const volatile uint32_t RESERVED6[61];
	DE_LAYER_Type L2_CTL;                   /*!< (@ 0x00000700) DE Layer2 Control Register                           */
} __attribute__((__packed__)) DE_Type;      /*!< Size = 2048 (0x800)                                                 */

/**
  * @brief DE Command Configuration
  */
typedef struct {
	uint32_t pos;
	uint32_t size;
	uint32_t addr;
	uint32_t stride;
	uint32_t length;
	uint32_t def_color;
} __attribute__((__packed__)) de_layer_cfg_t;

typedef struct {
	uint32_t ctl;
	uint32_t bg_color;
	uint32_t ctl1;
	uint32_t alpha_ctl;
	uint32_t wb_addr;
	uint32_t wb_stride;
	uint32_t color_fill_pos;
	uint32_t color_fill_size;
	uint32_t fill_color;
	uint32_t lx_abmp_ctl;

	uint32_t has_nor_access : 1;
	uint32_t has_rotation : 1;
	uint32_t has_scaling : 1;
	uint32_t n_layers : 2; /* number of layers */
	uint32_t blend_idx : 2; /* blending layer index */
	uint32_t x : 9; /* display pos x */
	uint32_t y : 9; /* display pos y */
	uint16_t w; /* bg width */
	uint16_t h; /* bg height */

	struct {
		uint32_t src_size;
		uint32_t sw_x_xy;
		uint32_t sw_y_xy;
		int32_t sw_x0;
		int32_t sw_y0;
	} rotate;

	struct {
		uint32_t sc_lx_src_size;
		uint32_t sc_lx_rate;
		uint32_t sc_lx_pos;
	} scaling;

	de_layer_cfg_t layers[3];
} __attribute__((__packed__)) de_overlay_cfg_t;

typedef struct {
	uint16_t layer_idx; /* index of layer */
	uint16_t size;
	const uint32_t *clut; /* clut table address */
} __attribute__((__packed__)) de_clut_cfg_t;

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
	uint32_t src_img_size;
} __attribute__((__packed__)) de_transform_cfg_t;

#endif /* ZEPHYR_DRIVERS_DISPLAY_ENGINE_DE_LEOPARD_H_ */
