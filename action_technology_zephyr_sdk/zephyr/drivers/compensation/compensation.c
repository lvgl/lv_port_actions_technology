/********************************************************************************
 *                            USDK(ZS283A)
 *                            Module: SYSTEM
 *                 Copyright(c) 2003-2017 Actions Semiconductor,
 *                            All Rights Reserved.
 *
 * History:
 *      <author>      <time>                      <version >          <desc>
 ********************************************************************************/
/*!
 * \file     compensation.c
 * \brief
 * \author
 * \version  1.0
 * \date
 *******************************************************************************/
#include <init.h>
#include <kernel.h>
#include <string.h>
#include <compensation.h>
#include <soc_atp.h>
#include <drivers/nvram_config.h>
#include <soc.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define SYS_LOG_DOMAIN "comp"

#ifndef SYS_LOG_LEVEL
#define SYS_LOG_LEVEL CONFIG_SYS_LOG_DEFAULT_LEVEL
#endif

#include <logging/log.h>

//2*8=16bit
#define MAX_EFUSE_CAP_RECORD  (CONFIG_COMPENSATION_FREQ_INDEX_NUM)

typedef struct
{
    uint32_t value[MAX_EFUSE_CAP_RECORD];
} cap_value_t;

#if defined(CONFIG_SOC_SERIES_WOODPECKERFPGA)
#define HOSC_CALIB_SIM
#endif

#ifdef HOSC_CALIB_SIM
static uint8_t hosc_val[MAX_EFUSE_CAP_RECORD];

static int soc_get_hosc_calib_sim(int id, unsigned char *calib_val)
{
    *calib_val = hosc_val[id];
    return 0;
}

static int soc_set_hosc_calib_sim(int id, unsigned char calib_val)
{
    hosc_val[id] = calib_val;
    return 0;
}
#endif

static uint32_t read_efuse_freq_value(uint32_t *read_cap_value, int *index)
{
    int i;
    cap_value_t cap_value;

    for (i = 0; i < MAX_EFUSE_CAP_RECORD; i++) {
#ifndef HOSC_CALIB_SIM
        soc_atp_get_hosc_calib(i, (unsigned int *)(&cap_value.value[i]));
#else
        soc_get_hosc_calib_sim(i, (unsigned int *)(&cap_value.value[i]));
#endif
    }

    // the latter value is the newer efuse and return the last value which modified.
    for (i = 0; i < MAX_EFUSE_CAP_RECORD; i++) {
        if (cap_value.value[i] == 0) {
            break;
        }
    }

    //efuse that never written
    if (i == 0) {
        *read_cap_value = 0xff;
        *index = 0xff;
    } else {
        *read_cap_value = cap_value.value[i - 1];
        *index = i - 1;
    }

    return 0;
}

static int32_t read_efuse_freq_compensation(uint32_t *cap_value, int *index)
{
    uint32_t trim_cap_value;

    read_efuse_freq_value(&trim_cap_value, index);

    if (*index != 0xff) {
        *cap_value = trim_cap_value;
    }

    return 0;
}

static int32_t spi_nor_freq_compensation_param_read(uint32_t *trim_cap)
{
	uint32_t  val ;
    int ret = nvram_config_get("BT_CFO_VAL", (void *) &val, 4);

    if (ret > 0){
		printk("nvram cap_val=%d\n", val);
        *trim_cap = val;
    }
    return ret;
}

int32_t freq_compensation_read(uint32_t *trim_cap, uint32_t mode)
{
    int ret_val, ret_val_nvram;
    int index;
    uint32_t trim_cap_value_bak = 0;

    ret_val_nvram = spi_nor_freq_compensation_param_read(&trim_cap_value_bak);

#ifdef CONFIG_COMPENSATION_HOSC_CAP_NVRAM_PRIORITY
    if (ret_val_nvram > 0) {
        // update the norflash parameters
        *trim_cap = trim_cap_value_bak;
        // the parameter partition is valid
        ret_val = TRIM_CAP_READ_NO_ERROR;

        return ret_val;
    }
#endif

    ret_val = read_efuse_freq_compensation(trim_cap, &index);

    if (index == 0xff) {
        if (ret_val_nvram > 0) {
            // update the norflash parameters
            *trim_cap = trim_cap_value_bak;
        } else {
            // update the default value
            *trim_cap = CONFIG_COMPENSATION_DEFAULT_HOSC_CAP;
        }
        // the parameter partition is valid.
        ret_val = TRIM_CAP_READ_NO_ERROR;
    } else if (index < (MAX_EFUSE_CAP_RECORD - 1)) {
        // if efuse value is already valid and there is also a space to write, return the frequency offset value.
        ret_val = TRIM_CAP_READ_NO_ERROR;
    } else {
        if (mode == RW_TRIM_CAP_EFUSE) {
            if (*trim_cap == 0xff) {
                // update the default value
                *trim_cap = CONFIG_COMPENSATION_DEFAULT_HOSC_CAP;
            }
            // the parameter partition is valid.
            ret_val = TRIM_CAP_READ_NO_ERROR;
        } else {
            if (ret_val_nvram > 0) {
                // update the norflash parameters
                *trim_cap = trim_cap_value_bak;
            }
            // the parameter partition is valid.
            ret_val = TRIM_CAP_READ_NO_ERROR;
        }
    }

    return ret_val;
}
#ifndef CONFIG_COMPENSATION_HOSC_CAP_NVRAM_PRIORITY
static int32_t write_efuse_new_value(int new_value, int old_index)
{
    int new_index;
    int ret_val;

    if (old_index != 0xff) {
        new_index = old_index + 1;
    } else {
        new_index = 0;
    }

    if (new_index < MAX_EFUSE_CAP_RECORD) {
        //ret_val = soc_set_hosc_calib_sim(new_index, new_value);
        ret_val = -2;
    } else {
        return -2;
    }

    return ret_val;
}

static int32_t write_efuse_freq_compensation(uint32_t *cap_value)
{
    int trim_cap_value;
    int index;
    int old_cap_value;

    // only write the least 8bits
    trim_cap_value = (*cap_value) & 0xff;

    read_efuse_freq_value(&old_cap_value, &index);

    if (index != 0xff) {
        if (old_cap_value == trim_cap_value) {
            return 0;
        } else {
            // add a new frequency offset value
            //efuse has already full
            if (index == (MAX_EFUSE_CAP_RECORD - 1)) {
                return -2;
            } else {
                return write_efuse_new_value(trim_cap_value, index);
            }
        }
    } else {
        //efuse that never been written
        return write_efuse_new_value(trim_cap_value, index);
    }
}
#endif

static int32_t spi_nor_freq_compensation_param_write(uint32_t *trim_cap)
{
#ifdef CONFIG_PROPERTY
    return nvram_config_set_factory("BT_CFO_VAL", trim_cap, 4);
#else
    return 0;
#endif
}

int32_t freq_compensation_write(uint32_t *trim_cap, uint32_t mode)
{
    int ret_val;

    // total 2 symmetrical bytes
    *trim_cap &= 0xffff;

    ret_val = spi_nor_freq_compensation_param_write(trim_cap);

#ifdef CONFIG_COMPENSATION_HOSC_CAP_NVRAM_PRIORITY
    // write norflash
    ret_val = spi_nor_freq_compensation_param_write(trim_cap);

    if (ret_val == 0) {
        ret_val = TRIM_CAP_WRITE_NO_ERROR;
    } else {
        ret_val = TRIM_CAP_WRITE_ERR_HW;
    }
#else
    ret_val = write_efuse_freq_compensation(trim_cap);

    if (mode == RW_TRIM_CAP_SNOR) {
        //efuse has no space
        if (ret_val == -2) {
            //write norflash
            ret_val = spi_nor_freq_compensation_param_write(trim_cap);

            if (ret_val == 0) {
                ret_val = TRIM_CAP_WRITE_NO_ERROR;
            } else {
                ret_val = TRIM_CAP_WRITE_ERR_HW;
            }
        } else if (ret_val == -1) {
            ret_val = TRIM_CAP_WRITE_ERR_HW;
        } else {
            ret_val = TRIM_CAP_WRITE_NO_ERROR;
        }
    } else {
        //efuse has no space
        if (ret_val == -2) {
            ret_val = TRIM_CAP_WRITE_ERR_NO_RESOURSE;
        } else if (ret_val == -1) {
            ret_val = TRIM_CAP_WRITE_ERR_HW;
        } else {
            ret_val = TRIM_CAP_WRITE_NO_ERROR;
        }
    }
#endif
    return ret_val;
}


int freq_compensation_get_cap(uint32_t *cap)
{
    uint32_t cap_adjust = 0;
    int ret_val;

    ret_val = freq_compensation_read(&cap_adjust, RW_TRIM_CAP_SNOR);

    if (ret_val != TRIM_CAP_READ_NO_ERROR) {
        *cap = CONFIG_COMPENSATION_DEFAULT_HOSC_CAP;
    }
    else{
        *cap = cap_adjust;
    }
    return ret_val;
}
