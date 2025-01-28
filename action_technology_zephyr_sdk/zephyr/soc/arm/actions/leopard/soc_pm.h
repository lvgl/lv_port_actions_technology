/*
 * Copyright (c) 2018 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file reboot configuration macros for Actions SoC
 */

#ifndef	_ACTIONS_SOC_PM_H_
#define	_ACTIONS_SOC_PM_H_

//reboot type, mask is 0xf00, low byte for details
//reboot reason, mask is 0xff, low byte for details
#define REBOOT_TYPE_NORMAL			0x000
#define REBOOT_TYPE_GOTO_ADFU		0x100
#define REBOOT_TYPE_GOTO_SYSTEM		0x200
#define REBOOT_TYPE_GOTO_RECOVERY	0x300
#define REBOOT_TYPE_GOTO_BTSYS		0x400
#define REBOOT_TYPE_GOTO_WIFISYS	0x500
#define REBOOT_TYPE_GOTO_SWJTAG		0x600
#define REBOOT_TYPE_GOTO_OTA		0x700



#define     WAKE_CTL_BATLV_VOL_e                                         29
#define     WAKE_CTL_BATLV_VOL_SHIFT                                     27
#define     WAKE_CTL_BATLV_VOL_MASK                                      (0x7<<27)
#define     WAKE_CTL_DC5VLV_VOL_e                                        26
#define     WAKE_CTL_DC5VLV_VOL_SHIFT                                    24
#define     WAKE_CTL_DC5VLV_VOL_MASK                                     (0x7<<24)
#define     WAKE_CTL_WIO1LV_VOL_e                                        23
#define     WAKE_CTL_WIO1LV_VOL_SHIFT                                    22
#define     WAKE_CTL_WIO1LV_VOL_MASK                                     (0x3<<22)
#define     WAKE_CTL_WIO0LV_VOL_e                                        21
#define     WAKE_CTL_WIO0LV_VOL_SHIFT                                    20
#define     WAKE_CTL_WIO0LV_VOL_MASK                                     (0x3<<20)
#define     WAKE_CTL_REMOTE_WKEN                                         (1<<12)
#define     WAKE_CTL_BATLV_WKEN                                          (1<<11)
#define     WAKE_CTL_DC5VLV_WKEN                                         (1<<10)
//#define     WAKE_CTL_WIO1LV_DETEN                                        (1<<9)
#define     WAKE_CTL_WIO0LV_DETEN                                        (1<<8)
//#define     WAKE_CTL_WIO1LV_WKEN                                         (1<<7)
#define     WAKE_CTL_WIO0LV_WKEN                                         (1<<6)
#define     WAKE_CTL_WIO_WKEN                                            (1<<5)
#define     WAKE_CTL_ALARM8HZ_WKEN                                       (1<<4)
#define     WAKE_CTL_DC5VOUT_WKEN                                        (1<<3)
#define     WAKE_CTL_DC5VIN_WKEN                                         (1<<2)
#define     WAKE_CTL_SHORT_WKEN                                          (1<<1)
#define     WAKE_CTL_LONG_WKEN                                           (1<<0)

union sys_pm_wakeup_src {
	uint32_t data;
	struct {
		uint32_t long_onoff : 1; /* ONOFF key long pressed wakeup */
		uint32_t short_onoff : 1; /* ONOFF key short pressed wakeup */
		uint32_t bat : 1; /* battery plug in wakeup */
		uint32_t alarm : 1; /* RTC alarm wakeup */
		uint32_t wio : 1; /* WIO wakeup */
		uint32_t remote : 1; /* remote wakeup */
		uint32_t batlv : 1; /*battery low power*/
		uint32_t dc5vlv : 1; /*dc5v low voltage*/
		uint32_t dc5vin : 1; /*dc5v in wakeup*/
		uint32_t watchdog : 1; 	/* watchdog reboot */
		uint32_t onoff_reset : 1; /* long onff reset */
	} t;
};

struct sys_pm_backup_time {
	uint32_t rtc_time_sec;
	uint32_t rtc_time_msec;
	uint32_t counter8hz_cycles;
	uint32_t user_alarm_cycles;
	uint32_t rc32k_freq;
	uint8_t is_backup_time_valid;
	uint8_t is_user_alarm_on;
	uint8_t is_user_cur_use;
	uint8_t is_use_alarm_cal;
};
typedef void (*shipmode_callback_t)(void *arg);


void sys_pm_reboot(int type);
int sys_pm_get_reboot_reason(u16_t *reboot_type, u8_t *reason);
int sys_pm_get_wakeup_source(union sys_pm_wakeup_src *src);
void sys_pm_set_dc5v_low_wakeup(int enable, int threshold);
int sys_pm_get_dc5v_low_threshold(uint8_t *volt);
void sys_pm_set_bat_low_wakeup(int enable, int threshold);
int sys_pm_get_bat_low_threshold(uint8_t *volt);
void sys_pm_set_onoff_wakeup(int enable);
void sys_pm_poweroff(void);
void sys_pm_factory_poweroff(void);

void sys_pm_shimode_register_callback(shipmode_callback_t func, void *arg);
void sys_pm_shipmode(void);



/*soc_sleep.c*/
void soc_enter_deep_sleep(void);
void soc_enter_light_sleep(void);
void soc_udelay(uint32_t us);

/*soc_pm.c*/

void sys_pm_enter_deep_sleep(void);
void sys_pm_enter_light_sleep(void);


#endif /* _ACTIONS_SOC_PM_H_	*/
