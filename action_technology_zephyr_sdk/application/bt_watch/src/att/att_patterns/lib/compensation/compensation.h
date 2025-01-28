/*******************************************************************************
 *                              US282f
 *                            Module: upgrade
 *                 Copyright(c) 2003-2016 Actions Semiconductor,
 *                            All Rights Reserved.
 *
 * History:
 *      <author>         <time>             <version>             <desc>
 *      wurui    2016-6-28 20:21:49           1.0              build this file
 *******************************************************************************/

#ifndef _COMPENSATION_H
#define _COMPENSATION_H

#define  RW_TRIM_CAP_EFUSE  (0)

#define  RW_TRIM_CAP_SNOR   (1)

#define CONFIG_COMPENSATION_FREQ_INDEX_NUM (2)

typedef enum
{
    TRIM_CAP_WRITE_NO_ERROR,
    TRIM_CAP_WRITE_ERR_HW,
    TRIM_CAP_WRITE_ERR_NO_RESOURSE
} trim_cap_write_result_e;

typedef enum
{

    TRIM_CAP_READ_NO_ERROR,

    TRIM_CAP_READ_ADJUST_VALUE,

    TRIM_CAP_READ_ERR_HW,

    TRIM_CAP_READ_ERR_NO_WRITE,

    TRIM_CAP_ERAD_ERR_VALUE
} trim_cap_read_result_e;

extern int32_t freq_compensation_read(uint32_t *trim_cap, uint32_t mode);

extern int32_t freq_compensation_write(uint32_t *trim_cap, uint32_t mode);

#endif  /*_COMPENSATION_H*/
