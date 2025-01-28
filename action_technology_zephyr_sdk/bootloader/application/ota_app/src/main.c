/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <device.h>
#include <string.h>
#include <soc.h>
#include <recovery.h>
#include <board_cfg.h>
#include <partition/partition.h>
#ifdef CONFIG_WATCHDOG
#include <watchdog_hal.h>
#endif

extern void boot_to_application(void);
extern void boot_to_ota_app(void);

int check_adfu(void)
{
	#if IS_ENABLED(CONFIG_TXRX_ADFU)
	if(check_adfu_connect(CONFIG_ADFU_TX_GPIO, CONFIG_ADFU_RX_GPIO))
		return 1;
	#endif
	#if IS_ENABLED(CONFIG_GPIO_ADFU)
	if(check_adfu_gpiokey(CONFIG_ADFU_KEY_GPIO))
		return 1;
	#endif	
	return 0;
}

int main(void)
{
	u32_t jtag_flag = 0;
	u16_t reboot_type;
	u8_t reason;
	//soc_watchdog_clear();
#ifdef CONFIG_WATCHDOG
	watchdog_clear();
#endif
	if(partition_valid_check()){		
		printk("part invalid,go to ota\n");
		goto exit_to_ota;
	}
	sys_pm_get_reboot_reason(&reboot_type, &reason);
	printk("reboot_type0x%x, reason=0x%x\n", reboot_type, reason);
	if(reboot_type == REBOOT_TYPE_GOTO_OTA){		
		printk("reboot to ota\n");
		goto exit_to_ota;
	}
		
	if(check_adfu()){
		jtag_set();
		soc_pstore_set(SOC_PSTORE_TAG_FLAG_JTAG, 1);
		sys_pm_reboot(REBOOT_TYPE_GOTO_ADFU);
	}
	soc_pstore_get(SOC_PSTORE_TAG_FLAG_JTAG, &jtag_flag);
	if(jtag_flag){
		printk("jtag switch\n");
		jtag_set();
	}

	if (ota_upgrade_is_allowed()) {
#if CONFIG_FLASH_LOAD_OFFSET
		boot_to_ota_app();
#endif
		recovery_main();		
	}
	//soc_watchdog_clear();
#ifdef CONFIG_WATCHDOG
	watchdog_clear();
#endif
	boot_to_application();
	sys_pm_reboot(REBOOT_TYPE_GOTO_OTA);
exit_to_ota:	
	// boot fail enter ota
	if(ota_main())// ota fail reboot adfu
		sys_pm_reboot(REBOOT_TYPE_GOTO_ADFU);
	else // ota ok reboot system
		sys_pm_reboot(REBOOT_TYPE_NORMAL);

	return 0;
}
