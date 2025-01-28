/*
* Copyright (c) 2019 Actions Semiconductor Co., Ltd
*
* SPDX-License-Identifier: Apache-2.0
*/

/**
* @file ap_autotest_rf_rx_cfo_trim.c
*/

#include "ap_autotest_rf_rx.h"
#include "compensation.h"

#define CONFIG_APPROACH_OLD_EFUSE

#ifdef CONFIG_APPROACH_OLD_EFUSE
static u32_t my_abs(s32_t value)
{
    if (value > 0)
        return value;
    else
        return (0 - value);
}
#endif

u8_t att_read_trim_cap(u32_t mode)
{
    u32_t trim_cap;
    int ret_val;

    ret_val = freq_compensation_read(&trim_cap, mode);

    if (ret_val == TRIM_CAP_READ_NO_ERROR) {
        return trim_cap;
    } else {
    	LOG_I("use hosc cap\n");
        extern int soc_get_hosc_cap(void);
        return soc_get_hosc_cap();
    }
}


u8_t att_write_trim_cap(u8_t index, u32_t mode)
{
    u32_t trim_cap = index;
    u32_t value_bak;
    int ret_val;

#ifdef CONFIG_APPROACH_OLD_EFUSE
	//Read the old written value and make an approximation comparison with the new value.
	//If the old value is a subset of the new value, you can avoid writing once more
	ret_val = freq_compensation_read(&value_bak, mode);
	if (ret_val == TRIM_CAP_READ_NO_ERROR) { /*FIXME : return must differentiate between efuse and nvram*/
		if (my_abs(index - value_bak) <= 2)
			trim_cap = value_bak;
		LOG_I("old val %d set val %d new val %d\n", value_bak, index, trim_cap);
	}
#endif

    ret_val = freq_compensation_write(&trim_cap, mode);

    if (ret_val == TRIM_CAP_WRITE_NO_ERROR) {
        //Read the frequency offset again to see if they are equal
        ret_val = freq_compensation_read(&value_bak, mode);
        if (ret_val == TRIM_CAP_READ_NO_ERROR) {
            if (value_bak == trim_cap)
                return trim_cap;
			else
				LOG_E("write and read not equal %d!=%d\n", value_bak, trim_cap);
        }
        return 0;
    } else {
        if (ret_val == TRIM_CAP_WRITE_ERR_NO_RESOURSE)
            LOG_E("efuse write over!\n");
        else if (ret_val == TRIM_CAP_WRITE_ERR_HW)
            LOG_E("efuse write HW err!\n");

        return 0;
    }
}




