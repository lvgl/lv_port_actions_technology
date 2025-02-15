/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief board init functions
 */

#include <kernel.h>
#include <init.h>
#include <device.h>
#include <soc.h>
#include "board.h"
#include <drivers/gpio.h>
#include <board_cfg.h>

#define CONFIG_GPIO_HIGHZ  (0x1000)
void boot_to_application(void)
{
	boot_to_app(0, BOOT_FLASH_ID_NOR);
}

void boot_to_ota_app(void)
{
	boot_to_part(5, 0, BOOT_FLASH_ID_NOR); // 5=temp part
}

static const struct acts_pin_config board_pin_config[] = {
    /*UART0 */
#if IS_ENABLED(CONFIG_UART_0)
    /* uart0 tx */
    PIN_MFP_SET(GPIO_28,  UART0_MFP_CFG),
    /* uart0 rx */
    PIN_MFP_SET(GPIO_29,  UART0_MFP_CFG),
#endif

    /*UART1 */
#if IS_ENABLED(CONFIG_UART_1)
    /* uart1 tx */
    PIN_MFP_SET(GPIO_16,  UART1_MFP_CFG),
    /* uart1 rx */
    PIN_MFP_SET(GPIO_17,  UART1_MFP_CFG),
#endif

#if IS_ENABLED(CONFIG_SPI_FLASH_0)
    /* SPI0 CS */
    PIN_MFP_SET(GPIO_0,   SPINOR_MFP_CFG),
    /* SPI0 MISO */
    PIN_MFP_SET(GPIO_1,   SPINOR_MFP_CFG),
    /* SPI0 CLK */
    PIN_MFP_SET(GPIO_2,   SPINOR_MFP_CFG),
    /* SPI0 MOSI */
    PIN_MFP_SET(GPIO_3,   SPINOR_MFP_CFG),
    /* SPI0 IO2 */
    PIN_MFP_SET(GPIO_6,   SPINOR_MFP_PU_CFG),
    /* SPI0 IO3 */
    PIN_MFP_SET(GPIO_7,   SPINOR_MFP_PU_CFG),
#endif

#if IS_ENABLED(CONFIG_ACTS_BATTERY_NTC)
    PIN_MFP_SET(GPIO_20,  BATNTC_MFP_CFG),
#endif

#if IS_ENABLED(CONFIG_I2CMT_0)
    /* I2C CLK*/
    PIN_MFP_SET(GPIO_18, I2CMT_MFP_CFG(MFP0_I2CMT)),
    /* I2C DATA*/
    PIN_MFP_SET(GPIO_19, I2CMT_MFP_CFG(MFP0_I2CMT)),
#endif

#if IS_ENABLED(CONFIG_I2CMT_1)
    /* I2C CLK*/
    PIN_MFP_SET(GPIO_55, I2CMT_MFP_CFG(MFP1_I2CMT)),
    /* I2C DATA*/
    PIN_MFP_SET(GPIO_56, I2CMT_MFP_CFG(MFP1_I2CMT)),
#endif

#if IS_ENABLED(CONFIG_I2C_0)
    /* I2C CLK*/
    PIN_MFP_SET(GPIO_57, I2C_MFP_CFG(MFP0_I2C)),
    /* I2C DATA*/
    PIN_MFP_SET(GPIO_58, I2C_MFP_CFG(MFP0_I2C)),
#endif

#if IS_ENABLED(CONFIG_I2C_1)
    /* I2C CLK*/
    PIN_MFP_SET(GPIO_51, I2C_MFP_CFG(MFP1_I2C)),
    /* I2C DATA*/
    PIN_MFP_SET(GPIO_52, I2C_MFP_CFG(MFP1_I2C)),
#endif

#if IS_ENABLED(CONFIG_I2C_2)
    /* I2C CLK*/
    PIN_MFP_SET(GPIO_61, I2C_MFP_CFG(MFP2_I2C)),
    /* I2C DATA*/
    PIN_MFP_SET(GPIO_62, I2C_MFP_CFG(MFP2_I2C)),
#endif

#if IS_ENABLED(CONFIG_I2C_3)
    /* I2C CLK*/
    PIN_MFP_SET(GPIO_60, I2C_MFP_CFG(MFP3_I2C)),
    /* I2C DATA*/
    PIN_MFP_SET(GPIO_59, I2C_MFP_CFG(MFP3_I2C)),
#endif

#if IS_ENABLED(CONFIG_CEC)
    PIN_MFP_SET(GPIO_12,  CEC_MFP_CFG),
#endif

#if IS_ENABLED(CONFIG_AUDIO_I2SRX_0)
    /*I2SRX0 mclk*/
    PIN_MFP_SET(GPIO_53,   I2SRX_MFP_CFG),
    /*I2SRX0 bclk*/
    PIN_MFP_SET(GPIO_54,   I2SRX_MFP_CFG),
    /*I2SRX0 lrclk*/
    PIN_MFP_SET(GPIO_55,   I2SRX_MFP_CFG),
    /*I2SRX0 d0*/
    PIN_MFP_SET(GPIO_56,   I2SRX_MFP_CFG),
#endif

#if IS_ENABLED(CONFIG_SPI_1)
    /*SPI SS*/
    PIN_MFP_SET(GPIO_24,  SPI_MFP_CFG(MFP_SPI1)),
    /* SPI CLK*/
    PIN_MFP_SET(GPIO_25,  SPI_MFP_CFG(MFP_SPI1)),
    /* SPI MISO*/
    PIN_MFP_SET(GPIO_26,  SPI_MFP_CFG(MFP_SPI1)),
    /* SPI MOSI*/
    PIN_MFP_SET(GPIO_27,  SPI_MFP_CFG(MFP_SPI1)),
#endif

#if IS_ENABLED(CONFIG_SPI_2)
    /*SPI SS*/
    PIN_MFP_SET(GPIO_30,  SPI_MFP_CFG(MFP_SPI2)),
    /* SPI CLK*/
    PIN_MFP_SET(GPIO_31,  SPI_MFP_CFG(MFP_SPI2)),
    /* SPI MISO*/
    PIN_MFP_SET(GPIO_32,  SPI_MFP_CFG(MFP_SPI2)),
    /* SPI MOSI*/
    PIN_MFP_SET(GPIO_33,  SPI_MFP_CFG(MFP_SPI2)),
#endif

#if IS_ENABLED(CONFIG_SPI_3)
    /*SPI SS*/
    PIN_MFP_SET(GPIO_20,  SPI_MFP_CFG(MFP_SPI3)),
    /* SPI CLK*/
    PIN_MFP_SET(GPIO_21,  SPI_MFP_CFG(MFP_SPI3)),
    /* SPI MISO*/
    PIN_MFP_SET(GPIO_22,  SPI_MFP_CFG(MFP_SPI3)),
    /* SPI MOSI*/
    PIN_MFP_SET(GPIO_23,  SPI_MFP_CFG(MFP_SPI3)),
#endif

#if IS_ENABLED(CONFIG_SPIMT_0)
    /* SPI SS*/
    PIN_MFP_SET(GPIO_49,  SPIMT_MFP_CFG(MFP0_SPIMT)),
    /* SPI CLK*/
    PIN_MFP_SET(GPIO_50,  SPIMT_MFP_CFG(MFP0_SPIMT)),
    /* SPI MISO*/
    PIN_MFP_SET(GPIO_51,  SPIMT_MFP_CFG(MFP0_SPIMT)),
    /* SPI MOSI*/
    PIN_MFP_SET(GPIO_52,  SPIMT_MFP_CFG(MFP0_SPIMT)),
    /* SPI SS1*/
    PIN_MFP_SET(GPIO_61,  SPIMT_MFP_CFG(MFP0_SPIMT)),
#endif

#if IS_ENABLED(CONFIG_SPIMT_1)
    /* SPI SS*/
    PIN_MFP_SET(GPIO_53,  SPIMT_MFP_CFG(MFP1_SPIMT)),
    /* SPI CLK*/
    PIN_MFP_SET(GPIO_54,  SPIMT_MFP_CFG(MFP1_SPIMT)),
    /* SPI MISO*/
    PIN_MFP_SET(GPIO_55,  SPIMT_MFP_CFG(MFP1_SPIMT)),
    /* SPI MOSI*/
    PIN_MFP_SET(GPIO_56,  SPIMT_MFP_CFG(MFP1_SPIMT)),
#endif
};

#if IS_ENABLED(CONFIG_MMC_0)
static const struct acts_pin_config board_mmc0_config[] = {
    /* MMC0 CMD*/
    PIN_MFP_SET(GPIO_10,   SDC0_MFP_CFG_VAL),
    /* MMC0 CLK*/
    PIN_MFP_SET(GPIO_12,  (GPIO_CTL_MFP(SDC0_MFP_SEL)|GPIO_CTL_PADDRV_LEVEL(3))),
    /* MMC0 DATA0 */
    PIN_MFP_SET(GPIO_11,   SDC0_MFP_CFG_VAL),
    /* MMC0 DATA1 */
    PIN_MFP_SET(GPIO_13,	SDC0_MFP_CFG_VAL),
    /* MMC0 DATA2 */
    PIN_MFP_SET(GPIO_8,    SDC0_MFP_CFG_VAL),
    /* MMC0 DATA3 */
    PIN_MFP_SET(GPIO_9,    SDC0_MFP_CFG_VAL),
};
#endif

#if IS_ENABLED(CONFIG_AUDIO_SPDIFTX_0)
static const struct acts_pin_config board_spdiftx0_config[] = {
    PIN_MFP_SET(GPIO_9, SPDIFTX_MFP_CFG),
};
#endif

#if IS_ENABLED(CONFIG_AUDIO_SPDIFRX_0)
static const struct acts_pin_config board_spdifrx0_config[] = {
    PIN_MFP_SET(GPIO_13,  SPDIFRX_MFP_CFG)
};
#endif

#if IS_ENABLED(CONFIG_AUDIO_I2STX_0)
static const struct acts_pin_config board_i2stx0_config[] = {
    /*I2STX0 mclk*/
    PIN_MFP_SET(GPIO_49, I2STX_MFP_CFG),
    /*I2STX0 bclk*/
    PIN_MFP_SET(GPIO_50, I2STX_MFP_CFG),
    /*I2STX0 lrclk*/
    PIN_MFP_SET(GPIO_51, I2STX_MFP_CFG),
    /*I2STX0 d0*/
    PIN_MFP_SET(GPIO_52, I2STX_MFP_CFG),
};
#endif


#if IS_ENABLED(CONFIG_SPINAND_3)
static const struct acts_pin_config board_spinand_spi3_config[] = {
    /* IOVCC1 poweron */
    PIN_MFP_SET(CONFIG_SPINAND_POWER_GPIO, 0x1F),
    /*SPI3 IO2*/
    PIN_MFP_SET(GPIO_8,   SPINAND_MFP_PU_CFG),
    /*SPI3 IO3*/
    PIN_MFP_SET(GPIO_9,   SPINAND_MFP_PU_CFG),
    /*SPI3 SS*/
    PIN_MFP_SET(GPIO_10,  SPINAND_MFP_CFG),
    /*SPI3 CLK*/
    PIN_MFP_SET(GPIO_11,  SPINAND_MFP_CFG),
    /*SPI3 IO1*/
    PIN_MFP_SET(GPIO_12,  SPINAND_MFP_CFG),
    /*SPI3 IO0*/
    PIN_MFP_SET(GPIO_13,  SPINAND_MFP_CFG),
};

static const struct acts_pin_config board_spinand_spi3_gpiohighz_config[] = {
    /* IOVCC1 poweroff */
    PIN_MFP_SET(CONFIG_SPINAND_POWER_GPIO, CONFIG_GPIO_HIGHZ),
	/*SPI3 IO2*/
	PIN_MFP_SET(GPIO_8,  CONFIG_GPIO_HIGHZ),
	/*SPI3 IO3*/
	PIN_MFP_SET(GPIO_9,  CONFIG_GPIO_HIGHZ),
	/*SPI3 SS*/
	PIN_MFP_SET(GPIO_10,  CONFIG_GPIO_HIGHZ),
	/*SPI3 CLK*/
	PIN_MFP_SET(GPIO_11,  CONFIG_GPIO_HIGHZ),
	/*SPI3 IO1*/
	PIN_MFP_SET(GPIO_12,  CONFIG_GPIO_HIGHZ),
	/*SPI3 IO0*/
	PIN_MFP_SET(GPIO_13,  CONFIG_GPIO_HIGHZ),
};
#endif


#if IS_ENABLED(CONFIG_PWM)
/* Look at CONFIG_PWM_PIN_CHAN_MAP select the available pwm gpio */
static const struct pwm_acts_pin_config board_pwm_config[] = {
    /* GPIO5 used as pwm channel 1*/
    PWM_PIN_MFP_SET(GPIO_5, 1, PWM_MFP_CFG),
    /* GPIO21 used as pwm channel 7*/
    PWM_PIN_MFP_SET(GPIO_21, 7, PWM_MFP_CFG),
};
#endif

#if IS_ENABLED(CONFIG_PANEL)
static const struct acts_pin_config board_lcd_pin_config[] = {
    /* lcd cs */
    PIN_MFP_SET(GPIO_30, (GPIO_CTL_MFP(LCD_MFP_SEL)|GPIO_CTL_PULLUP)),
    /* lcd scl */
    PIN_MFP_SET(GPIO_34, LCD_MFP_SEL),
    /* lcd_d0*/
    PIN_MFP_SET(GPIO_14, LCD_MFP_SEL),
    /* lcd_d1*/
    PIN_MFP_SET(GPIO_15, LCD_MFP_SEL),
    /* lcd_d2*/
    PIN_MFP_SET(GPIO_16, LCD_MFP_SEL),
    /* lcd_d3*/
    PIN_MFP_SET(GPIO_17, LCD_MFP_SEL),
    /* lcd power: IOVCC2 */
    PIN_MFP_SET(GPIO_33, 0x1F),
};
#endif

#if IS_ENABLED(CONFIG_ADCKEY)

#define CONFIG_ADCKEY_GPIO

#ifdef CONFIG_ADCKEY_GPIO
#define CONFIG_ADCKEY_GPIO_NUM (GPIO_21)
#else
#define CONFIG_ADCKEY_WIO_NUM  (WIO_0)
#define CONFIG_ADCKEY_WIO_MFP  (3)
#endif

static void board_adckey_pinmux_init(void)
{
#ifdef CONFIG_ADCKEY_GPIO
    acts_pinmux_set(CONFIG_ADCKEY_GPIO_NUM, ADCKEY_MFP_CFG);
#else
    sys_write32(CONFIG_ADCKEY_WIO_MFP, WIO0_CTL + (CONFIG_ADCKEY_WIO_NUM * 4));
#endif
}
#endif

static int board_early_init(const struct device *arg)
{
    ARG_UNUSED(arg);
    acts_pinmux_setup_pins(board_pin_config, ARRAY_SIZE(board_pin_config));

#if IS_ENABLED(CONFIG_MMC_0)
    acts_pinmux_setup_pins(board_mmc0_config, ARRAY_SIZE(board_mmc0_config));
#endif
#if IS_ENABLED(CONFIG_ADCKEY)
    board_adckey_pinmux_init();
#endif
#if IS_ENABLED(CONFIG_PANEL)
    acts_pinmux_setup_pins(board_lcd_pin_config, ARRAY_SIZE(board_lcd_pin_config));
#endif
#ifdef CONFIG_RTT_CONSOLE
	jtag_set();
#endif

    return 0;
}

static int board_later_init(const struct device *arg)
{
    ARG_UNUSED(arg);
    printk("%s %d: \n", __func__, __LINE__);
    return 0;
}

/* UART registers struct */
struct acts_uart_reg {
    volatile uint32_t ctrl;

    volatile uint32_t rxdat;

    volatile uint32_t txdat;

    volatile uint32_t stat;

    volatile uint32_t br;
} ;

void uart_poll_out_ch(int c)
{
    struct acts_uart_reg *uart = (struct acts_uart_reg*)UART0_REG_BASE;
    /* Wait for transmitter to be ready */
    while (uart->stat &  BIT(6));
    /* send a character */
    uart->txdat = (uint32_t)c;

}
/*for early printk*/
int arch_printk_char_out(int c)
{
    if ('\n' == c)
        uart_poll_out_ch('\r');
    uart_poll_out_ch(c);
    return 0;
}

void board_get_mmc0_pinmux_info(struct board_pinmux_info *pinmux_info)
{
#if IS_ENABLED(CONFIG_MMC_0)
    pinmux_info->pins_config = board_mmc0_config;
    pinmux_info->pins_num = ARRAY_SIZE(board_mmc0_config);
#endif
}


void board_get_spdiftx0_pinmux_info(struct board_pinmux_info *pinmux_info)
{
#if IS_ENABLED(CONFIG_AUDIO_SPDIFTX_0)
    pinmux_info->pins_config = board_spdiftx0_config;
    pinmux_info->pins_num = ARRAY_SIZE(board_spdiftx0_config);
#endif
}

void board_get_spdifrx0_pinmux_info(struct board_pinmux_info *pinmux_info)
{
#if IS_ENABLED(CONFIG_AUDIO_SPDIFRX_0)
    pinmux_info->pins_config = board_spdifrx0_config;
    pinmux_info->pins_num = ARRAY_SIZE(board_spdifrx0_config);
#endif
}

void board_get_i2stx0_pinmux_info(struct board_pinmux_info *pinmux_info)
{
#if IS_ENABLED(CONFIG_AUDIO_SPDIFRX_0)
    pinmux_info->pins_config = board_i2stx0_config;
    pinmux_info->pins_num = ARRAY_SIZE(board_i2stx0_config);
#endif
}

void board_get_pwm_pinmux_info(struct board_pwm_pinmux_info *pinmux_info)
{
#if IS_ENABLED(CONFIG_PWM)
    pinmux_info->pins_config = board_pwm_config;
    pinmux_info->pins_num = ARRAY_SIZE(board_pwm_config);
#endif
}

void board_get_spinand_pinmux_info(struct board_pinmux_info *pinmux_info)
{
#if IS_ENABLED(CONFIG_SPINAND_3)
	pinmux_info->pins_config = board_spinand_spi3_config;
	pinmux_info->pins_num = ARRAY_SIZE(board_spinand_spi3_config);
#endif
}

void board_get_spinand_gpiohighz_info(struct board_pinmux_info *pinmux_info)
{
#if IS_ENABLED(CONFIG_SPINAND_3)
	pinmux_info->pins_config = board_spinand_spi3_gpiohighz_config;
	pinmux_info->pins_num = ARRAY_SIZE(board_spinand_spi3_gpiohighz_config);
#endif
}

#if IS_ENABLED(CONFIG_PANEL)
static uint32_t lcd_pin_backup[3];

void board_lcd_suspend(bool aod_en, bool early_suspend)
{
    if (early_suspend) {
        lcd_pin_backup[0] = sys_read32(GPION_CTL(35)); /* lcd te */
        sys_write32(CONFIG_GPIO_HIGHZ, GPION_CTL(35));

        if (aod_en == false) {
            lcd_pin_backup[1] = sys_read32(GPION_CTL(21)); /* lcd power */
            sys_write32(CONFIG_GPIO_HIGHZ, GPION_CTL(21));

            lcd_pin_backup[2] = sys_read32(GPION_CTL(32)); /* lcd reset */
            sys_write32(CONFIG_GPIO_HIGHZ, GPION_CTL(32));

            sys_write32(CONFIG_GPIO_HIGHZ, GPION_CTL(30)); /* lcd cs */
            sys_write32(CONFIG_GPIO_HIGHZ, GPION_CTL(33)); /* lcd power IOVCC2 */
        }
    }

    sys_write32(CONFIG_GPIO_HIGHZ, GPION_CTL(34)); /* lcd scl */
    sys_write32(CONFIG_GPIO_HIGHZ, GPION_CTL(14)); /* lcd d0 */
    sys_write32(CONFIG_GPIO_HIGHZ, GPION_CTL(15)); /* lcd d1 */
    sys_write32(CONFIG_GPIO_HIGHZ, GPION_CTL(16)); /* lcd d2 */
    sys_write32(CONFIG_GPIO_HIGHZ, GPION_CTL(17)); /* lcd d3 */
}

void board_lcd_resume(bool aod_en, bool late_resume)
{
    acts_pinmux_setup_pins(board_lcd_pin_config, ARRAY_SIZE(board_lcd_pin_config));

    if (late_resume) {
        sys_write32(lcd_pin_backup[0], GPION_CTL(35)); /* lcd te */

        if (aod_en == false) {
            sys_write32(lcd_pin_backup[1], GPION_CTL(21)); /* lcd GND_LEDK */
            sys_write32(lcd_pin_backup[2], GPION_CTL(32)); /* lcd reset */
        }
    }
}
#endif /* CONFIG_PANEL */

SYS_INIT(board_early_init, PRE_KERNEL_1, 5);

SYS_INIT(board_later_init, POST_KERNEL, 5);
