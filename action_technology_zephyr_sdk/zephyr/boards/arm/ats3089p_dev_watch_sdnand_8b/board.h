/*
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __INC_BOARD_H
#define __INC_BOARD_H

#include <board_cfg.h>
#include <drivers/audio/audio_common.h>
#include <soc.h>

#ifdef __cplusplus
extern "C" {
#endif

/*Configure GPIO high resistance before sleep and restore GPIO after wakeup */
#define SLEEP_GPIO_REG_SET_HIGHZ      				\
	/*sensor*/ \
	/*//GPION_CTL(18), not use defaut highz*/		\
	/*//GPION_CTL(20), not use defaut highz*/		\
	GPION_CTL(25), /*audio PA1_EN */	\
	GPION_CTL(26), /*TP_PWR */	\
	/*TP*/ \
	/*//GPION_CTL(26), not use defaut highz*/		\
	/* //GPION_CTL(27), not use，defaut highz*/		\
	GPION_CTL(49), /*tp rst*/				\
	GPION_CTL(50), /*tp irq*/				\
	/*i2c0 charger */  \
	GPION_CTL(57), /*not use in sleep*/  \
	GPION_CTL(58), /*not use in sleep*/  \
	/*i2c3 audio PA/LCD I2c/Electricity meter*/  \
	GPION_CTL(51), /*not use in sleep*/  \
	GPION_CTL(52), /*not use in sleep*/  \

#define CONFIG_SD_USE_GPIO_DET      0
#define CONFIG_SD_GPIO_DET_NUM      31    /*GPIO31*/
#define CONFIG_SD_GPIO_DET_LEVEL    0    /* The GPIO level(0: low voltage 1:high voltage) to indicates the SD card has been detected */
#define CONFIG_SD_USE_GPIO_POWER    1
#define CONFIG_SD_GPIO_POWER_NUM    31   /*GPIO31*/
#define CONFIG_SD_GPIO_POWER_LEVEL  1    /* The GPIO level(0: low voltage 1:high voltage) to indicates the SD/eMMC card power on */

#define CONFIG_TPKEY_GPIO_ISR_NUM	 		50
#define CONFIG_TPKEY_GPIO_SCL_NUM	 		51
#define CONFIG_TPKEY_GPIO_SDA_NUM	 		52

#define CONFIG_PANEL_TE_GPIO		GPIO_CFG_MAKE(CONFIG_GPIO_B_NAME, 3, GPIO_ACTIVE_HIGH, 1)
#define CONFIG_PANEL_RESET_GPIO		GPIO_CFG_MAKE(CONFIG_GPIO_B_NAME, 0, GPIO_ACTIVE_LOW, 1)
//#define CONFIG_PANEL_POWER_GPIO		GPIO_CFG_MAKE(CONFIG_GPIO_B_NAME, 1, GPIO_ACTIVE_HIGH, 1)
#define CONFIG_PANEL_POWER1_GPIO	GPIO_CFG_MAKE(CONFIG_GPIO_A_NAME, 21, GPIO_ACTIVE_HIGH, 1)

#define CONFIG_TPKEY_ISR_GPIO		GPIO_CFG_MAKE(CONFIG_GPIO_B_NAME, 18, GPIO_ACTIVE_LOW, 1)
#define CONFIG_TPKEY_RESET_GPIO		GPIO_CFG_MAKE(CONFIG_GPIO_B_NAME, 17, GPIO_ACTIVE_LOW, 1)
//#define CONFIG_TPKEY_POWER_GPIO		GPIO_CFG_MAKE(CONFIG_GPIO_A_NAME, 26, GPIO_ACTIVE_HIGH, 1)

#if IS_ENABLED(CONFIG_GPIOKEY)
	/* The GPIO MFP for GPIO KEY */
#if CONFIG_GPIOKEY_PRESSED_VOLTAGE_LEVEL
#define CONFIG_GPIOKEY_MFP    PIN_MFP_SET(GPIO_24,   GPIOKEY_MFP_CFG)
#else
#define CONFIG_GPIOKEY_MFP    PIN_MFP_SET(GPIO_24,   GPIOKEY_MFP_PU_CFG)
#endif
#endif

#define CONFIG_SPINAND_POWER_GPIO           64    /* GPIO64 */
#define CONFIG_SPINAND_DATA                 1     /* SPINand Flash Use for data or boot; 0:boot; 1:data */

/* ACC Sensor GPIO Config */
#define CONFIG_SENSOR_ACC_POWER_GPIO        0
#define CONFIG_SENSOR_ACC_RESET_GPIO        0
#define CONFIG_SENSOR_ACC_ISR_GPIO          54    /* GPIO54 */
#define CONFIG_SENSOR_ACC_TRIG_IO           5     /* PPI_triger5 */

/* Heart-rate Sensor GPIO Config */
#define CONFIG_SENSOR_HR_POWER_GPIO        22    /* GPIO22 */
#define CONFIG_SENSOR_HR_RESET_GPIO        0    /* GPIO62 */
#define CONFIG_SENSOR_HR_ISR_GPIO          74    /* GPIO74 */
#define CONFIG_SENSOR_HR_TRIG_IO           6     /* PPI_triger6 */

/* Magnet Sensor GPIO Config */
#define CONFIG_SENSOR_MAG_POWER_GPIO        0
#define CONFIG_SENSOR_MAG_RESET_GPIO        0
#define CONFIG_SENSOR_MAG_ISR_GPIO          0
#define CONFIG_SENSOR_MAG_TRIG_IO           0

/* Baro Sensor GPIO Config */
#define CONFIG_SENSOR_BARO_POWER_GPIO        0
#define CONFIG_SENSOR_BARO_RESET_GPIO        0
#define CONFIG_SENSOR_BARO_ISR_GPIO          0
#define CONFIG_SENSOR_BARO_TRIG_IO           0


#define BOARD_BATTERY_CAP_MAPPINGS      \
	{0, 3200},  \
	{5, 3300},  \
	{10, 3400}, \
	{20, 3550}, \
	{30, 3650}, \
	{40, 3750}, \
	{50, 3800}, \
	{60, 3850}, \
	{70, 3900}, \
	{80, 3950}, \
	{90, 4000}, \
	{100, 4050},

#define BOARD_ADCKEY_KEY_MAPPINGS	\
	{KEY_ADFU,          0x05}, \
	{KEY_PREVIOUSSONG,	0x376},	\
	{KEY_MENU,		    0x532},	\
	{KEY_TBD,	        0x69C},	\
	{KEY_VOLUMEDOWN,    0x86D}, \
	{KEY_VOLUMEUP,	    0x9EE}, \
	{KEY_NEXTSONG,	    0xB87},

/* @brief The macro to define an audio device */
#define AUDIO_LINE_IN0 (AUDIO_DEV_TYPE_LINEIN | 0)
#define AUDIO_LINE_IN1 (AUDIO_DEV_TYPE_LINEIN | 1)
#define AUDIO_LINE_IN2 (AUDIO_DEV_TYPE_LINEIN | 2)
#define AUDIO_ANALOG_MIC0 (AUDIO_DEV_TYPE_AMIC | 0)
#define AUDIO_ANALOG_MIC1 (AUDIO_DEV_TYPE_AMIC | 1)
#define AUDIO_ANALOG_MIC2 (AUDIO_DEV_TYPE_AMIC | 2)
#define AUDIO_ANALOG_FM0 (AUDIO_DEV_TYPE_FM | 0)
#define AUDIO_DIGITAL_MIC0 (AUDIO_DEV_TYPE_DMIC | 0)
#define AUDIO_DIGITAL_MIC1 (AUDIO_DEV_TYPE_DMIC | 1)

struct board_pinmux_info{
	const struct acts_pin_config *pins_config;
	int pins_num;
};

struct pwm_acts_pin_config{
	unsigned int pin_num;
	unsigned int pin_chan;
	unsigned int mode;
};

struct board_pwm_pinmux_info{
	const struct pwm_acts_pin_config *pins_config;
	int pins_num;
};


extern void board_get_mmc0_pinmux_info(struct board_pinmux_info *pinmux_info);
extern void board_get_spdiftx0_pinmux_info(struct board_pinmux_info *pinmux_info);
extern void board_get_spdifrx0_pinmux_info(struct board_pinmux_info *pinmux_info);
extern void board_get_i2stx0_pinmux_info(struct board_pinmux_info *pinmux_info);
extern void board_get_pwm_pinmux_info(struct board_pwm_pinmux_info *pinmux_info);
extern void board_get_spinand_pinmux_info(struct board_pinmux_info *pinmux_info);
extern void board_get_spinand_gpiohighz_info(struct board_pinmux_info *pinmux_info);
extern void board_set_ntc_pinmux_info(bool enable, bool use_as_ntc);

#if 1
/* @brief Get the mapping relationship between the hardware inputs and audio devices */
int board_audio_device_mapping(audio_input_map_t *input_map);
int board_extern_pa_ctl(u8_t pa_class, bool is_on);
#endif
#if IS_ENABLED(CONFIG_GPS)
int gps_power095_pin_ctl( bool is_on);
int gps_power_pin_ctl( bool is_on);
int gps_wake_up_pin_ctl( bool is_on);
#endif
#ifdef CONFIG_PANEL
void board_lcd_suspend(bool aod_en, bool early_suspend);
void board_lcd_resume(bool aod_en, bool late_resume);
#endif

#ifdef __cplusplus
}
#endif

#endif /* __INC_BOARD_H */
