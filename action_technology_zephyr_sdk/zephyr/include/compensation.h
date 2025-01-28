/********************************************************************************
 * Copyright (c) 2018 Actions Semi Co., Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.

 * Author: wuyufan<mikeyang@actions-semi.com>
 *
 * Description:   usb_irq head file ( physical layer )
 *
 * Change log:
 *	2019/2/18: Created by wuyufan.
 *******************************************************************************/

#ifndef COMPENSATION_H_
#define COMPENSATION_H_

#define  RW_TRIM_CAP_EFUSE  (0)

#define  RW_TRIM_CAP_SNOR   (1)

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

extern void cap_temp_comp_start(void);

extern void cap_temp_comp_stop(void);

extern uint32_t cap_temp_comp_enabled(void);

extern int freq_compensation_get_cap(uint32_t *cap);

#endif /* COMPENSATION_H_ */
