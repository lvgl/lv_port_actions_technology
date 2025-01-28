/**
 * @copyright (c) 2003 - 2020, Goodix Co., Ltd. All rights reserved.
 *
 * @file    gh3011_example_ctrl.c
 *
 * @brief   example code for gh3011 (condensed  hbd_ctrl lib)
 *
 */

#include "gh3011_example_common.h"
#if (__USE_GOODIX_APP__)
#include "hbd_communicate.h"
#endif


/// debug log string
#if (__EXAMPLE_DEBUG_LOG_LVL__) // debug level > 0

    /// dbg run mode string
    const char dbg_rum_mode_string[][24] = 
    {
        "RUN_MODE_INVALID\0",
        "RUN_MODE_INVALID\0",
        "RUN_MODE_ADT_HB_DET\0",
        "RUN_MODE_HRV_DET\0",
        "RUN_MODE_BPF_DET\0",
        "RUN_MODE_GETRAWDATA_DET\0",
        "RUN_MODE_INVALID\0",
        "RUN_MODE_SPO2_DET\0",         
    };
    /// dbg communicate cmd string
    const char dbg_comm_cmd_string[][35] = 
    {
        "COMM_CMD_ALGO_IN_MCU_HB_START\0",    
        "COMM_CMD_ALGO_IN_MCU_HB_STOP\0",
        "COMM_CMD_ALGO_IN_APP_HB_START\0",
        "COMM_CMD_ALGO_IN_APP_HB_STOP\0",
        "COMM_CMD_ALGO_IN_MCU_HRV_START\0",
        "COMM_CMD_ALGO_IN_MCU_HRV_STOP\0",
        "COMM_CMD_ALGO_IN_APP_HRV_START\0",    
        "COMM_CMD_ALGO_IN_APP_HRV_STOP\0",    
        "COMM_CMD_ADT_SINGLE_MODE_START\0",
        "COMM_CMD_ADT_SINGLE_MODE_STOP\0",
        "COMM_CMD_ADT_CONTINUOUS_MODE_START\0",
        "COMM_CMD_ADT_CONTINUOUS_MODE_STOP\0",
        "COMM_CMD_ALGO_IN_MCU_SPO2_START\0",
        "COMM_CMD_ALGO_IN_MCU_SPO2_STOP\0",
        "COMM_CMD_ALGO_IN_APP_SPO2_START\0",
        "COMM_CMD_ALGO_IN_APP_SPO2_STOP\0",
        "COMM_CMD_INVALID\0",    
    };
    /// dbg ret val string
    const char dbg_ret_val_string[][35] = 
    {
        "HBD_RET_LED_CONFIG_ALL_OFF_ERROR\0",
        "HBD_RET_NO_INITED_ERROR\0",
        "HBD_RET_RESOURCE_ERROR\0",
        "HBD_RET_COMM_ERROR\0",
        "HBD_RET_COMM_NOT_REGISTERED_ERROR\0",
        "HBD_RET_PARAMETER_ERROR\0",
        "HBD_RET_GENERIC_ERROR\0",
        "HBD_RET_OK\0",
    };

#endif

#if (__HBD_USE_DYN_MEM__)
GU8 g_MemReady = 0;
GU8 *g_pMem = NULL;

void gh30x_malloc_memory(void)
{
    if(0 == g_MemReady)
    {
        GS32 nMemSize = HBD_GetMemRequired(0);
        
        g_pMem = hal_gh30x_memory_malloc(nMemSize);
        if(NULL != g_pMem)
        {            
            g_MemReady = 1;
        }
    }
    if(0 != g_pMem)
    {
        HBD_SetMemPtr(g_pMem);
    }
}

void gh30x_free_memory(void)
{
    g_MemReady = 0;
    if(NULL != g_pMem)
    {
        hal_gh30x_memory_free(g_pMem);
        g_pMem = NULL;
    }    
}
#endif
    
/// gh30x load new config
void gh30x_Load_new_config(const ST_REGISTER *config_ptr, uint16_t len)
{
    GU8 index = 0;
    for (index = 0; index < __RETRY_MAX_CNT_CONFIG__; index ++) // retry 
    {
        if (HBD_LoadNewRegConfigArr(config_ptr, len) == HBD_RET_OK)
        {
            break;
        }
        EXAMPLE_DEBUG_LOG_L1("gh30x load new config error\r\n");
    }
}

/// gh30x adt func start
void gh30x_adt_wear_detect_start(const ST_REGISTER *config_ptr, GU16 config_len)
{
    GS16 nRes = 0;
    GU8 index = 0;
    
    HAL_GH30X_FIFO_INT_TIMEOUT_TIMER_STOP();
    if ((config_ptr != NULL) & (config_len != 0))
    {
#if (__HBD_USE_DYN_MEM__)
        gh30x_malloc_memory();
#endif
        gh30x_Load_new_config(config_ptr, config_len);
        nRes = HBD_AdtWearDetectStart();
        if (nRes != HBD_RET_OK)  // start
        {
            EXAMPLE_DEBUG_LOG_L1("gh30x adt start error : %d\r\n", nRes);
            for (index = 0; index < __RETRY_MAX_CNT_CONFIG__; index ++) // retry 
            {
                if (HBD_SimpleInit(&gh30x_init_config) == HBD_RET_OK)
                {
                    if (HBD_LoadNewRegConfigArr(config_ptr, config_len) == HBD_RET_OK)
                    {
                        if (HBD_AdtWearDetectStart() == HBD_RET_OK)
                        {
                            EXAMPLE_DEBUG_LOG_L1("gh30x adt start retry success\r\n");
                            break;
                        }
                    }    
                }
            }
        }
    }
}

/// gh30x adt confirm func start
void gh30x_adt_confirm_start(void)
{
    GS16 nRes = 0;
    GU8 index = 0;

    HAL_GH30X_FIFO_INT_TIMEOUT_TIMER_STOP();
    HBD_AdtConfirmConfig(__HB_ADT_CONFIRM_GS_AMP__, __HB_ADT_CONFIRM_CHECK_CNT__, __HB_ADT_CONFIRM_THR_CNT__);
#if (__HBD_USE_DYN_MEM__)
        gh30x_malloc_memory();
#endif
    nRes = HBD_AdtConfirmStart();
    if (nRes != HBD_RET_OK)  // start
    {
        EXAMPLE_DEBUG_LOG_L1("gh30x adt confirm start error : %d\r\n", nRes);
        for (index = 0; index < __RETRY_MAX_CNT_CONFIG__; index ++) // retry 
        {
            if (HBD_SimpleInit(&gh30x_init_config) == HBD_RET_OK)
            {
                if (HBD_LoadNewRegConfigArr(hb_adt_confirm_reg_config, hb_adt_confirm_reg_config_len) == HBD_RET_OK)
                {
                    if (HBD_AdtConfirmStart() == HBD_RET_OK)
                    {
                        EXAMPLE_DEBUG_LOG_L1("gh30x adt confirm start retry success\r\n");
                        HAL_GH30X_FIFO_INT_TIMEOUT_TIMER_START();
                        break;
                    }
                }   
            }
        }
    }
    else
    {
        HAL_GH30X_FIFO_INT_TIMEOUT_TIMER_START();
    }
}

#if (__HB_DET_SUPPORT__)
/// gh30x hb func start
void gh30x_hb_start(void)
{
    GS16 nRes = 0;
    GU8 index = 0;

    HAL_GH30X_FIFO_INT_TIMEOUT_TIMER_STOP();

    #if (__HBA_ENABLE_WEARING__)
    GF32 wearing_config_array[3] = {0, 0, 0};
    HBD_EnableWearing(wearing_config_array);
    #endif

    HBD_SetFifoThrCnt(FIFO_THR_CONFIG_TYPE_HB, __HB_FIFO_THR_CNT_CONFIG__);
#if (__HBD_USE_DYN_MEM__)
    gh30x_malloc_memory();
#endif
    nRes = HBD_HbDetectStart();
    if (nRes != HBD_RET_OK)  // start
    {
        EXAMPLE_DEBUG_LOG_L1("gh30x hb start error : %d\r\n", nRes);
        for (index = 0; index < __RETRY_MAX_CNT_CONFIG__; index ++) // retry 
        {
            if (HBD_SimpleInit(&gh30x_init_config) == HBD_RET_OK)
            {
                if (HBD_LoadNewRegConfigArr(hb_reg_config_array, hb_reg_config_array_len) == HBD_RET_OK)
                {
                    if (HBD_HbDetectStart() == HBD_RET_OK)
                    {
                        EXAMPLE_DEBUG_LOG_L1("gh30x hb start retry success\r\n");
                        HAL_GH30X_FIFO_INT_TIMEOUT_TIMER_START();
                        break;
                    }
                }  
            }
        }
    }
    else
    {
        HAL_GH30X_FIFO_INT_TIMEOUT_TIMER_START();
    }
}

#endif

#if (__HRV_DET_SUPPORT__)
/// gh30x hrv func start
void gh30x_hrv_start(void)
{
    GS16 nRes = 0;
    GU8 index = 0;

    HAL_GH30X_FIFO_INT_TIMEOUT_TIMER_STOP();
    
    HBD_SetFifoThrCnt(FIFO_THR_CONFIG_TYPE_HRV, __HRV_FIFO_THR_CNT_CONFIG__);
#if (__HBD_USE_DYN_MEM__)
    gh30x_malloc_memory();
#endif
    // gh30x_Load_new_config(hrv_reg_config_array, hrv_reg_config_array_len);
    nRes = HBD_HbWithHrvDetectStart();
    if(nRes != HBD_RET_OK) // start
    {
        EXAMPLE_DEBUG_LOG_L1("gh30x hrv start error : %d\r\n", nRes);
        for (index = 0; index < __RETRY_MAX_CNT_CONFIG__; index ++) // retry 
        {
            if (HBD_SimpleInit(&gh30x_init_config) == HBD_RET_OK)
            {
                if (HBD_LoadNewRegConfigArr(hrv_reg_config_array, hrv_reg_config_array_len) == HBD_RET_OK)
                {
                    if (HBD_HbWithHrvDetectStart() == HBD_RET_OK)
                    {
                        EXAMPLE_DEBUG_LOG_L1("gh30x hrv start retry success\r\n");
                        HAL_GH30X_FIFO_INT_TIMEOUT_TIMER_START();
                        break;
                    }
                }  
            }
        }
    }
    else
    {
        HAL_GH30X_FIFO_INT_TIMEOUT_TIMER_START();
    }
}

#endif

#if (__BPF_DET_SUPPORT__)
/// gh30x bpf func start
void gh30x_bpf_start(void)
{
    GS16 nRes = 0;
    GU8 index = 0;

    HAL_GH30X_FIFO_INT_TIMEOUT_TIMER_STOP();
    
    HBD_SetFifoThrCnt(FIFO_THR_CONFIG_TYPE_BPF, __BPF_FIFO_THR_CNT_CONFIG__);
#if (__HBD_USE_DYN_MEM__)
    gh30x_malloc_memory();
#endif
    // gh30x_Load_new_config(hrv_reg_config_array, hrv_reg_config_array_len);
    nRes = HBD_BpfDetectStart();
    if(nRes != HBD_RET_OK) // start
    {
        EXAMPLE_DEBUG_LOG_L1("gh30x bpf start error : %d\r\n", nRes);
        for (index = 0; index < __RETRY_MAX_CNT_CONFIG__; index ++) // retry 
        {
            if (HBD_SimpleInit(&gh30x_init_config) == HBD_RET_OK)
            {
                if (HBD_LoadNewRegConfigArr(bpf_reg_config_array, bpf_reg_config_array_len) == HBD_RET_OK)
                {
                    if (HBD_BpfDetectStart() == HBD_RET_OK)
                    {
                        EXAMPLE_DEBUG_LOG_L1("gh30x bpf start retry success\r\n");
                        HAL_GH30X_FIFO_INT_TIMEOUT_TIMER_START();
                        break;
                    }
                }  
            }
        }
    }
    else
    {
        HAL_GH30X_FIFO_INT_TIMEOUT_TIMER_START();
    }
}

#endif

#if (__SPO2_DET_SUPPORT__)
/// gh30x spo2 func start
void gh30x_spo2_start(void)
{
    GS16 nRes = 0;
    GU8 index = 0;

    HAL_GH30X_FIFO_INT_TIMEOUT_TIMER_STOP();

    HBD_SetFifoThrCnt(FIFO_THR_CONFIG_TYPE_SPO2, __SPO2_FIFO_THR_CNT_CONFIG__);
#if (__HBD_USE_DYN_MEM__)
    gh30x_malloc_memory();
#endif
    // gh30x_Load_new_config(spo2_reg_config_array, spo2_reg_config_array_len);
    nRes = HBD_SpO2DetectStart();
    if (nRes != HBD_RET_OK)  // start
    {
        EXAMPLE_DEBUG_LOG_L1("gh30x spo2 start error : %d\r\n", nRes);
        for (index = 0; index < __RETRY_MAX_CNT_CONFIG__; index ++) // retry 
        {
            if (HBD_SimpleInit(&gh30x_init_config) == HBD_RET_OK)
            {
                if (HBD_LoadNewRegConfigArr(spo2_reg_config_array, spo2_reg_config_array_len) == HBD_RET_OK)
                {
                    if (HBD_SpO2DetectStart() == HBD_RET_OK)
                    {
                        EXAMPLE_DEBUG_LOG_L1("gh30x spo2 start retry success\r\n");
                        HAL_GH30X_FIFO_INT_TIMEOUT_TIMER_START();
                        break;
                    }
                }  
            }
        }
    }
    else
    {
        HAL_GH30X_FIFO_INT_TIMEOUT_TIMER_START();
    }
}
#endif

#if (__GET_RAWDATA_ONLY_SUPPORT__)
/// gh30x get rawdata only func start
void gh30x_getrawdata_start(void)
{
    GU8 index = 0;

    HAL_GH30X_FIFO_INT_TIMEOUT_TIMER_STOP();

    //HBD_SetFifoThrCnt(FIFO_THR_CONFIG_TYPE_GETRAWDATA, __GET_RAWDATA_FIFO_THR_CNT_CONFIG__);
#if (__HBD_USE_DYN_MEM__)
    gh30x_malloc_memory();
#endif
    // gh30x_Load_new_config(getrawdata_reg_config_array, rawdata_reg_config_array_len);
    if (HBD_StartHBDOnly(__GET_RAWDATA_SAMPLE_RATE__,1,__GET_RAWDATA_FIFO_THR_CNT_CONFIG__) != HBD_RET_OK)  // start
    {
        EXAMPLE_DEBUG_LOG_L1("gh30x getrawdata start error\r\n");
        for (index = 0; index < __RETRY_MAX_CNT_CONFIG__; index ++) // retry 
        {
            if (HBD_SimpleInit(&gh30x_init_config) == HBD_RET_OK)
            {
                if (HBD_LoadNewRegConfigArr(getrawdata_reg_config_array, getrawdata_reg_config_array_len) == HBD_RET_OK)
                {
                    if (HBD_StartHBDOnly(__GET_RAWDATA_SAMPLE_RATE__,1,__GET_RAWDATA_FIFO_THR_CNT_CONFIG__) == HBD_RET_OK)
                    {
                        HAL_GH30X_FIFO_INT_TIMEOUT_TIMER_START();
                        break;
                    }
                }  
            }
        }
    }
    else
    {
        HAL_GH30X_FIFO_INT_TIMEOUT_TIMER_START();
    }
}

#endif

#if (__GH30X_COMMUNICATION_INTERFACE__ == GH30X_COMMUNICATION_INTERFACE_SPI)

/// i2c exchange to spi for gh30x wrtie
uint8_t gh30x_i2c_write_exchange_to_spi(uint8_t device_id, const uint8_t write_buffer[], uint16_t length)
{
	uint8_t ret = GH30X_EXAMPLE_OK_VAL;
	if ((length == 3) && (write_buffer[0] == 0xDD) && (write_buffer[1] == 0xDD))
    {
        hal_gh30x_spi_cs_set_low();
        hal_gh30x_spi_write(&write_buffer[2], 1);
        hal_gh30x_spi_cs_set_high();
        HBD_DelayUs(10);
    }
    else
    {
        uint8_t spi_write_buffer[5] = {0};
        uint16_t spi_real_len = length - 2;

        hal_gh30x_spi_cs_set_low();
        spi_write_buffer[0] = 0xF0;
        spi_write_buffer[1] = write_buffer[0];
        spi_write_buffer[2] = write_buffer[1];
        spi_write_buffer[3] = GET_HIGH_BYTE_FROM_WORD(spi_real_len);
        spi_write_buffer[4] = GET_LOW_BYTE_FROM_WORD(spi_real_len);
        hal_gh30x_spi_write(spi_write_buffer, 5);
        hal_gh30x_spi_write(&write_buffer[2], spi_real_len);
        HBD_DelayUs(20);
        hal_gh30x_spi_cs_set_high();
        HBD_DelayUs(10);
    }
	return ret;
}

/// i2c exchange to spi for gh30x read
uint8_t gh30x_i2c_read_exchange_to_spi(uint8_t device_id, const uint8_t write_buffer[], uint16_t write_length, uint8_t read_buffer[], uint16_t read_length)
{
	uint8_t ret = GH30X_EXAMPLE_OK_VAL;
    if (write_length == 2)
    {
        uint8_t spi_write_buffer[3] = {0};
        hal_gh30x_spi_cs_set_low();
        spi_write_buffer[0] = 0xF0;
        spi_write_buffer[1] = write_buffer[0];
        spi_write_buffer[2] = write_buffer[1];
        hal_gh30x_spi_write(spi_write_buffer, 3);
        HBD_DelayUs(20);
        hal_gh30x_spi_cs_set_high();
        HBD_DelayUs(10);
        hal_gh30x_spi_cs_set_low();
        spi_write_buffer[0] = 0xF1;
        hal_gh30x_spi_write(spi_write_buffer, 1);
        hal_gh30x_spi_read(read_buffer, read_length);
        HBD_DelayUs(20);
        hal_gh30x_spi_cs_set_high();
        HBD_DelayUs(10);
    }
    
	return ret;
}

#endif

/// system test comm check
uint8_t gh30x_systemtest_comm_check(void)
{
    uint8_t ret = 0;
    #if (__SYSTEM_TEST_SUPPORT__)
    ROMAHBD_Interfcae gh30x_wr_i;
    gh30x_wr_i.WR_Fun = (Write_fun)HBD_I2cWriteReg;
    gh30x_wr_i.RD_Fun = (Read_fun)HBD_I2cReadReg;

    HBD_I2cSendCmd(0xC0);
    HBD_DelayUs(600);
    ret=HBDTEST_Comm_Check(&gh30x_wr_i);
    HBD_I2cSendCmd(0xC4);
    HBD_DelayUs(600);
    #endif
    
    return ret;
}

/// system test otp check
uint8_t gh30x_systemtest_otp_check(void)
{
    uint8_t ret = 0;

    #if (__SYSTEM_TEST_SUPPORT__)
    uint8_t systemtest_otp_buffer[64] = {0};
    ROMAHBD_Interfcae gh30x_wr_i;
    gh30x_wr_i.WR_Fun = (Write_fun)HBD_I2cWriteReg;
    gh30x_wr_i.RD_Fun = (Read_fun)HBD_I2cReadReg;

    HBD_I2cSendCmd(0xC0);
    HBD_DelayUs(600);
    ret = HBDTEST_OTP_Check(&gh30x_wr_i, systemtest_otp_buffer);
    HBD_I2cSendCmd(0xC4);
    HBD_DelayUs(600);
    #endif

    return ret;
}

/// system test os start
void gh30x_systemtest_os_start(GU8 led_num)
{
    GU8 index = 0;
    const ST_REGISTER *system_test_reg_config_ptr = NULL;
    GU16 system_test_reg_config_len = 0;

    HAL_GH30X_FIFO_INT_TIMEOUT_TIMER_STOP();
    // load config
    if (led_num == 2)
    {
        system_test_reg_config_ptr = systemtest_led2_reg_config_array;
        system_test_reg_config_len = systemtest_led2_reg_config_array_len;
    }
    else if (led_num == 1)
    {
        system_test_reg_config_ptr = systemtest_led1_reg_config_array;
        system_test_reg_config_len = systemtest_led1_reg_config_array_len;
    }
    else // fixed to 0
    {
        system_test_reg_config_ptr = systemtest_led0_reg_config_array;
        system_test_reg_config_len = systemtest_led0_reg_config_array_len;
    }
    
    gh30x_Load_new_config(system_test_reg_config_ptr, system_test_reg_config_len);
    HBD_FifoConfig(0, HBD_FUNCTIONAL_STATE_DISABLE);
    HBD_FifoConfig(1, HBD_FUNCTIONAL_STATE_DISABLE);  
    #if (__HBD_USE_DYN_MEM__)
    gh30x_malloc_memory();
    #endif
    GS8 res = HBD_HbDetectStart() ;
    if (res!= HBD_RET_OK)  // start
    {
        EXAMPLE_DEBUG_LOG_L1("gh30x system start error %d\r\n", res);
        for (index = 0; index < __RETRY_MAX_CNT_CONFIG__; index ++) // retry 
        {
            if (HBD_SimpleInit(&gh30x_init_config) == HBD_RET_OK)
            {
                if (HBD_LoadNewRegConfigArr(system_test_reg_config_ptr, system_test_reg_config_len) == HBD_RET_OK)
                {
                    if (HBD_HbDetectStart() == HBD_RET_OK)
                    {
                        HAL_GH30X_FIFO_INT_TIMEOUT_TIMER_START();
                        break;
                    }
                }  
            }
        }
    }
    HBD_FifoConfig(0, HBD_FUNCTIONAL_STATE_ENABLE);
    HBD_FifoConfig(1, HBD_FUNCTIONAL_STATE_ENABLE);
}

/// system test os calc
uint8_t gh30x_systemtest_os_calc(uint8_t led_num)
{
    uint8_t ret = 0xFF;
    #if (__SYSTEM_TEST_SUPPORT__)
    static int32_t systemtest_rawdata_buffer[__SYSTEM_TEST_DATA_CNT_CONFIG__] = {0};
    static uint8_t systemtest_rawdata_buffer_index = 0;
    static uint8_t systemtest_last_led_num = 0;

    if (systemtest_last_led_num != led_num)
    {
        systemtest_rawdata_buffer_index = 0;
        systemtest_last_led_num = led_num;
    }

    systemtest_rawdata_buffer[systemtest_rawdata_buffer_index] = HBD_I2cReadRawdataReg(g_usReadRawdataRegList[0]) & 0x0001FFFF;
    systemtest_rawdata_buffer_index++;
    EXAMPLE_DEBUG_LOG_L1("got rawdata %d,the index is %d.\r\n",systemtest_rawdata_buffer[systemtest_rawdata_buffer_index-1],systemtest_rawdata_buffer_index);
    if (systemtest_rawdata_buffer_index >= __SYSTEM_TEST_DATA_CNT_CONFIG__)
    {
        if(goodix_system_test_mode & 0x1)
        {
            EXAMPLE_DEBUG_LOG_L1("check rawdata result\n");
            const HBDTEST_ROMAHBData *hbdatalst[]={&systemtest_led0_os_result,&systemtest_led1_os_result,&systemtest_led2_os_result};
            ret = HBDTEST_Check_Rawdata_Noise((int *)systemtest_rawdata_buffer, __SYSTEM_TEST_DATA_CNT_CONFIG__, hbdatalst[led_num]);   
        }
        else if(goodix_system_test_mode & 0x2)
        {
            EXAMPLE_DEBUG_LOG_L1("check ctr result\n");
            HBDTEST_ROMALEDCheckData *hbdatalst[]={&led0std,&led1std,&led2std};
            ret = HBDTEST_Check_CTRandNoise((int *)systemtest_rawdata_buffer,__SYSTEM_TEST_DATA_CNT_CONFIG__,hbdatalst[led_num]);
            hbdatalst[led_num]->_res._flag|=0x1;
            EXAMPLE_DEBUG_LOG_L1("res is %f,%d\n",hbdatalst[led_num]->_res._CTR,hbdatalst[led_num]->_res._flag);
        }
        else if(goodix_system_test_mode & 0x4)
        {
            EXAMPLE_DEBUG_LOG_L1("check leak result\n");
            HBDTEST_ROMALEDCheckData *hbdatalst[]={&led0std,&led1std,&led2std};
            if(goodix_system_test_mode & 0x8)
            {
                ret=HBDTEST_Check_LeakandRatio((int *)systemtest_rawdata_buffer,__SYSTEM_TEST_DATA_CNT_CONFIG__,hbdatalst[led_num]);
            }
        }
        else
        {
            EXAMPLE_DEBUG_LOG_L1("test error");
            ret=EN_SELFTST_ERROR_ALL;
        }
        systemtest_rawdata_buffer_index = 0;
    }
    #else
    ret = 0;
    #endif
    return ret;
}

#if (__SYSTEM_TEST_SUPPORT__)
//analysis the config and get config
void gh30x_systemtest_param_set(uint8_t led_num,HBDTEST_ROMAConfigParam* param)
{
    const ST_REGISTER *array_list[3]={&systemtest_led0_reg_config_array[0],&systemtest_led1_reg_config_array[0],&systemtest_led2_reg_config_array[0]};
    const uint8_t lenth_list[3]={systemtest_led0_reg_config_array_len,systemtest_led1_reg_config_array_len,systemtest_led2_reg_config_array_len};
    const ST_REGISTER *reg_config_array=array_list[led_num];
    uint8_t lenth=lenth_list[led_num];
    int temp[5];//118,11a,84,136,180
    unsigned char flag=0;
    for(int j=0;j<lenth;j++)
    {
        if(reg_config_array[j].usRegAddr==0x118)
        {
            temp[0]=reg_config_array[j].usRegData;
            flag|=0x1;
        }
        if(reg_config_array[j].usRegAddr==0x11a) 
        {
            temp[1]=reg_config_array[j].usRegData;
            flag|=0x2;
        }
        if(reg_config_array[j].usRegAddr==0x84) 
        {
            temp[2]=reg_config_array[j].usRegData;
            flag|=0x4;
        }
        if(reg_config_array[j].usRegAddr==0x136) 
        {
            temp[3]=reg_config_array[j].usRegData;
            flag|=0x8;
        }
        if(reg_config_array[j].usRegAddr==0x180)
        {
            temp[4]=reg_config_array[j].usRegData;
            flag|=0x10;
        }
    }
    unsigned index=0;
    unsigned char map=temp[2]&0x1c0;
    unsigned char mapobj[][3]={{0,1,2},{0,2,1},{1,0,2},{1,2,0},{2,0,1},{2,1,0}};
    for(int j=0;j<3;j++)
    {
        if(temp[2]&(1<<j))
        {
            index=j;
            break;
        }
    }
    index=mapobj[map][index];
    int16_t resismap[8]={100,200,380,700,1000,1480,1960,2440};
    float currmap[8]={20.0,40,60,80,100,120,140,160};
    switch(index)
    {
        case 0:
            param->_ledResis=resismap[(temp[3]&0x70)>>4];
            param->_ledCurr=currmap[(temp[4]&0xe0)>>5]*(float)(temp[0]&0xff)/255.0f;
            break;
        case 1:
            param->_ledResis=resismap[(temp[3]&0x380)>>7];
            param->_ledCurr=currmap[(temp[4]&0xe0)>>5]*(float)((temp[0]&0xff00)>>8)/255.0f;
            break;
        case 2:
            param->_ledResis=resismap[(temp[3]&0x1c00)>>10];
            param->_ledCurr=currmap[(temp[4]&0xe0)>>5]*(float)(temp[1]&0xff)/255.0f;
            break;
    }
    EXAMPLE_DEBUG_LOG_L1("analysis res is %d,%f,%d\n", led_num, param->_ledCurr, param->_ledResis);
}
#endif


/********END OF FILE********* Copyright (c) 2003 - 2020, Goodix Co., Ltd. ********/
