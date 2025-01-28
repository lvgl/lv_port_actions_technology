/**
 * @copyright (c) 2003 - 2020, Goodix Co., Ltd. All rights reserved.
 *
 * @file    gh3011_example.h
 *
 * @brief   example code for gh3011 (condensed  hbd_ctrl lib)
 *
 */

#ifndef _GH3011_EXAMPLE_H_
#define _GH3011_EXAMPLE_H_


#include "gh3011_example_common.h" 


/**
 * @brief start run mode enum
 */
typedef enum
{
    GH30X_RUN_MODE_HB = RUN_MODE_ADT_HB_DET,   
    GH30X_RUN_MODE_HRV = RUN_MODE_HRV_DET,
    GH30X_RUN_MODE_SPO2 = RUN_MODE_SPO2_DET,
    GH30X_RUN_MODE_BPF = RUN_MODE_BPF_DET,
	GH30X_RUN_MODE_GETRAW = RUN_MODE_GETRAWDATA_DET,
} EMGh30xRunMode;

/// hb algo scenario, call before start: <0=> default, <1=> routine, <2=> run, <3=> clibm,  <4=> bike, <5=> irregular
#define GH30X_HBA_SCENARIO_CONFIG(sce)         do { HBD_HbAlgoScenarioConfig(sce); } while(0)

/// gh30x module init
int gh30x_module_init(void);

/// gh30x module start, with adt 
void gh30x_module_start(EMGh30xRunMode start_run_mode);

/// gh30x module start, without adt 
void gh30x_module_start_without_adt(EMGh30xRunMode start_run_mode);

/// gh30x module stop
void gh30x_module_stop(void);

/// ble recv data handler
void ble_module_recv_data_via_gdcs(uint8_t *data, uint8_t length);

/// uart recv data handler
void uart_module_recv_data(uint8_t *data, uint8_t length);

typedef enum
{
    HBDTEST_TESTIEM_COMM = 0x1,   
    HBDTEST_TESTIEM_OTP = 0x2,
    HBDTEST_TESTIEM_CTR = 0x4,
    HBDTEST_TESTIEM_LEAK = 0x8,
    HBDTEST_ONLYTEST_CTR_LEAK =0x1c,
    HBDTEST_TEST_ALL_TEST = 0x1f
} EMGh30xTestItem;

void gh30x_systemtest_start(EMGh30xTestItem mode);

#endif /* _GH3011_EXAMPLE_H_ */

/********END OF FILE********* Copyright (c) 2003 - 2020, Goodix Co., Ltd. ********/
