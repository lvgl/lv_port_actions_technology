/**
 * @copyright (c) 2003 - 2020, Goodix Co., Ltd. All rights reserved.
 *
 * @file    gh3011_example_process.c
 *
 * @brief   example code for gh3011 (condensed  hbd_ctrl lib)
 *
 */

#include "gh3011_example_common.h"
#include "gh3011_example.h"
#include <math.h>


/// app mode start flag
bool goodix_app_start_app_mode = false;

/// system test mode start flag
uint8_t goodix_system_test_mode = 0;

/// system test os led num
uint8_t goodix_system_test_os_led_num = 0;

#if (__HB_NEED_ADT_CONFIRM__)
    /// adt confirm flag
    bool adt_confirm_flag = false; 
    /// start flag without adt confirm flag
    bool hb_start_without_adt_confirm = false; 
#endif

/// gsensor fifo index and len
static uint16_t gsensor_soft_fifo_buffer_index = 0;
/// gsnesor fifo
static ST_GS_DATA_TYPE gsensor_soft_fifo_buffer[GSENSOR_SOFT_FIFO_BUFFER_MAX_LEN];

/// gh30x run mode
uint8_t gh30x_run_mode = RUN_MODE_INVALID; 



/// gh30x module init, include gsensor init
int gh30x_module_init(void)
{ 
    static bool first_init_flag = true;
    GS8 init_err_flag = HBD_RET_OK;

    /* log all version string */
    EXAMPLE_DEBUG_LOG_L1(GH30X_EXAMPLE_VER_STRING);
    EXAMPLE_DEBUG_LOG_L1("hbd ctrl lib version: %s\r\n", HBD_GetHbdVersion());
#if (__HBD_HB_ENABLE__)
    EXAMPLE_DEBUG_LOG_L1("hba version: %s\r\n", HBD_GetHbaVersion());
#endif
#if (__SPO2_DET_SUPPORT__)
    EXAMPLE_DEBUG_LOG_L1("spo2 version: %s\r\n", HBD_GetSpo2Version());
#endif
#if (__HRV_DET_SUPPORT__)
    EXAMPLE_DEBUG_LOG_L1("hrv version: %s\r\n", HBD_GetHrvVersion());
#endif
#if (__BPF_DET_SUPPORT__)
    EXAMPLE_DEBUG_LOG_L1("bpf version: %s\r\n", HBD_GetBpfVersion());
    EXAMPLE_DEBUG_LOG_L1("af version: %s\r\n", HBD_GetAFVersion());
#endif
    EXAMPLE_DEBUG_LOG_L1("nadt version: %s\r\n", HBD_GetNadtVersion());
    EXAMPLE_DEBUG_LOG_L1("hbd lib compile time: %s\r\n", HBD_GetHbdCompileTime());
#if (__SYSTEM_TEST_SUPPORT__)
    EXAMPLE_DEBUG_LOG_L1("test lib version: %s\r\n", HBDTEST_Get_TestlibVersion());
#endif
    if (first_init_flag)
    {
        #if (__GH30X_COMMUNICATION_INTERFACE__ == GH30X_COMMUNICATION_INTERFACE_SPI)
            hal_gh30x_spi_init(); // spi init
            HBD_SetI2cRW(HBD_I2C_ID_SEL_1L0L, gh30x_i2c_write_exchange_to_spi, gh30x_i2c_read_exchange_to_spi); // register i2c exchange to spi api
        #else // (__GH30X_COMMUNICATION_INTERFACE__ == GH30X_COMMUNICATION_INTERFACE_I2C)
            hal_gh30x_i2c_init(); // i2c init
            HBD_SetI2cRW(HBD_I2C_ID_SEL_1L0L, hal_gh30x_i2c_write, hal_gh30x_i2c_read); // register i2c RW func api
        #endif

        #if (__PLATFORM_DELAY_US_CONFIG__)
            HBD_SetDelayUsCallback(hal_gh30x_delay_us);
        #endif
    }

    init_err_flag = HBD_SimpleInit(&gh30x_init_config); // init gh30x
    if (HBD_RET_OK != init_err_flag)  
	{
        EXAMPLE_DEBUG_LOG_L1("gh30x init error[%s]\r\n", dbg_ret_val_string[DEBUG_HBD_RET_VAL_BASE + init_err_flag]);
    	return GH30X_EXAMPLE_ERR_VAL;
	}
	
	init_err_flag = gsensor_drv_init(); // gsensor init
    if (GH30X_EXAMPLE_ERR_VAL == init_err_flag)  
	{
        EXAMPLE_DEBUG_LOG_L1("gsensor init error\r\n");
    	return GH30X_EXAMPLE_ERR_VAL;
	}
	
    if (first_init_flag)
    {
        hal_gsensor_int1_init(); // gsensor int pin init
        hal_gh30x_int_init(); // gh30x int pin init

        #if (__GH30X_IRQ_PLUSE_WIDTH_CONFIG__)
        HBD_SetIrqPluseWidth(255); // set Irq pluse width (255us)
        #endif

        gh30x_comm_pkg_init(); // comm pkg init

        HAL_GH30X_FIFO_INT_TIMEOUT_TIMER_INIT();
    }

    EXAMPLE_DEBUG_LOG_L1("gh30x module init ok\r\n");
    first_init_flag = false;
    return GH30X_EXAMPLE_OK_VAL;  
}

/// gh30x module start, with adt 
void gh30x_module_start(EMGh30xRunMode start_run_mode)
{
    #if ((__HB_DET_SUPPORT__) && (__HB_NEED_ADT_CONFIRM__))
    if (start_run_mode == RUN_MODE_ADT_HB_DET)
    {
        hb_start_without_adt_confirm = true;
    }
    #endif
    gh30x_run_mode = (uint8_t)start_run_mode;
    EXAMPLE_DEBUG_LOG_L1("gh30x module start, mode [%s]\r\n", dbg_rum_mode_string[gh30x_run_mode]);
    if (gh30x_run_mode == RUN_MODE_SPO2_DET)
    {
        SEND_MCU_SPO2_UNWEAR_EVT(NULL, 0); // send start cmd with unwear evt data
    }
    gsensor_enter_normal_and_clear_buffer();
    gh30x_start_adt_with_mode((uint8_t)start_run_mode);
}

/// gh30x module start, without adt 
void gh30x_module_start_without_adt(EMGh30xRunMode start_run_mode)
{
    #if ((__HB_DET_SUPPORT__) && (__HB_NEED_ADT_CONFIRM__))
    if (start_run_mode == RUN_MODE_ADT_HB_DET)
    {
        hb_start_without_adt_confirm = true;
    }
    #endif
    gh30x_run_mode = (uint8_t)start_run_mode;
    EXAMPLE_DEBUG_LOG_L1("gh30x module start without adt, mode [%s]\r\n", dbg_rum_mode_string[gh30x_run_mode]);
    gsensor_enter_clear_buffer_and_enter_fifo();
    gh30x_start_func_with_mode((uint8_t)start_run_mode);
}

/// gh30x module stop
void gh30x_module_stop(void)
{
    gsensor_enter_normal_and_clear_buffer();
    gh30x_stop_func();
    gh30x_run_mode = RUN_MODE_INVALID;
    EXAMPLE_DEBUG_LOG_L1("gh30x module stop\r\n");
    #if (__USE_GOODIX_APP__)
	goodix_app_start_app_mode = false; // if call stop, clear app mode
    #endif
}

/// gh30x reset evt handler
void gh30x_reset_evt_handler(void)
{
    GS8 reinit_ret = HBD_RET_OK;
    GU8 reinit_cnt = __RESET_REINIT_CNT_CONFIG__;
    gsensor_enter_normal_and_clear_buffer();
    // reinit
    do 
    {
        reinit_ret = HBD_SimpleInit(&gh30x_init_config);
        reinit_cnt --;
    } while (reinit_ret != HBD_RET_OK);
    if ((reinit_ret == HBD_RET_OK) && (gh30x_run_mode != RUN_MODE_INVALID)) // if reinit ok, restart last mode
    {	
        #if (__USE_GOODIX_APP__)
        if (goodix_app_start_app_mode)
        {
            SEND_GH30X_RESET_EVT();
        }
        else
        #endif
        {
            gh30x_start_adt_with_mode(gh30x_run_mode);
        }
    }
    EXAMPLE_DEBUG_LOG_L1("got gh30x reset evt, reinit [%s]\r\n", dbg_ret_val_string[DEBUG_HBD_RET_VAL_BASE + reinit_ret]);
}

/// gh30x unwear  evt handler
void gh30x_unwear_evt_handler(void)
{
    #if (__USE_GOODIX_APP__)
	if (goodix_app_start_app_mode)
    {
        SEND_AUTOLED_FAIL_EVT();
        EXAMPLE_DEBUG_LOG_L1("got gh30x unwear evt, restart func\r\n");
        HBD_FifoConfig(0, HBD_FUNCTIONAL_STATE_DISABLE);
        HBD_FifoConfig(1, HBD_FUNCTIONAL_STATE_DISABLE);
        gh30x_start_func_whithout_adt_confirm(gh30x_run_mode);
        HBD_FifoConfig(0, HBD_FUNCTIONAL_STATE_ENABLE);
        HBD_FifoConfig(1, HBD_FUNCTIONAL_STATE_ENABLE);
    }
    else
    #endif
    {
        gsensor_enter_normal_and_clear_buffer();
        if (gh30x_run_mode == RUN_MODE_ADT_HB_DET)
        {
            #if (__HB_START_WITH_GSENSOR_MOTION__)
            gsensor_drv_enter_motion_det_mode();
            EXAMPLE_DEBUG_LOG_L1("got gh30x unwear evt, start gsensor motion\r\n");
            #else
            gh30x_start_adt_with_mode(gh30x_run_mode);
            EXAMPLE_DEBUG_LOG_L1("got gh30x unwear evt, start adt detect\r\n");
            #endif
            SEND_MCU_HB_MODE_WEAR_STATUS(WEAR_STATUS_UNWEAR, NULL, 0);
        }
        else
        {
            if (gh30x_run_mode == RUN_MODE_SPO2_DET)
            {
                SEND_MCU_SPO2_UNWEAR_EVT(NULL, 0);
            }
            gh30x_start_adt_with_mode(gh30x_run_mode);
            EXAMPLE_DEBUG_LOG_L1("got gh30x unwear evt, start adt detect\r\n");
        }
        HAL_GH30X_FIFO_INT_TIMEOUT_TIMER_STOP();
        handle_wear_status_result(WEAR_STATUS_UNWEAR);
    }
}

/// gh30x wear evt handler
void gh30x_wear_evt_handler(void)
{
    gsensor_enter_clear_buffer_and_enter_fifo();
    gh30x_start_func_with_mode(gh30x_run_mode);

#if (__HB_NEED_ADT_CONFIRM__)
    EXAMPLE_DEBUG_LOG_L1("got gh30x wear evt, start func[%s], adt confrim [%d]\r\n", dbg_rum_mode_string[gh30x_run_mode], adt_confirm_flag);
    if (gh30x_run_mode != RUN_MODE_ADT_HB_DET)
    {
        handle_wear_status_result(WEAR_STATUS_WEAR);
    }
#else
    EXAMPLE_DEBUG_LOG_L1("got gh30x wear evt, start func[%s]\r\n", dbg_rum_mode_string[gh30x_run_mode]);
    if (gh30x_run_mode == RUN_MODE_ADT_HB_DET)
    {
        SEND_MCU_HB_MODE_WEAR_STATUS(WEAR_STATUS_WEAR, NULL, 0);
    }
    handle_wear_status_result(WEAR_STATUS_WEAR);
#endif
}

/// calc unwear status handle
void gh30x_handle_calc_unwear_status(void)
{
    gsensor_enter_normal_and_clear_buffer();
    if (gh30x_run_mode == RUN_MODE_ADT_HB_DET)
    {
        #if (__HB_START_WITH_GSENSOR_MOTION__)
        gsensor_drv_enter_motion_det_mode();
        EXAMPLE_DEBUG_LOG_L1("calc unwear status, start gsensor motion\r\n");
        #else
        gh30x_start_adt_with_mode(gh30x_run_mode);
        EXAMPLE_DEBUG_LOG_L1("calc unwear status, start adt detect\r\n");
        #endif
    }
    else
    {
        gh30x_start_adt_with_mode(gh30x_run_mode);
        EXAMPLE_DEBUG_LOG_L1("calc unwear status, start adt detect\r\n");
        handle_wear_status_result(WEAR_STATUS_UNWEAR);
    }
}

#if (__HB_DET_SUPPORT__)
/// fifo evt hb mode calc
static void gh30x_fifo_evt_hb_mode_calc(GS32 *dbg_rawdata_ptr)
{
    GU16 dbg_rawdata_len = __ALGO_CALC_DBG_BUFFER_LEN__;
    #if (__HB_NEED_ADT_CONFIRM__)
    if (adt_confirm_flag)
    {
        GU8 adt_confirm_res = HBD_AdtConfirmCalculateByFifoIntDbgOutputData(gsensor_soft_fifo_buffer, gsensor_soft_fifo_buffer_index, 
                                    __GS_SENSITIVITY_CONFIG__, (GS32 (*)[DBG_MCU_MODE_PKG_LEN])dbg_rawdata_ptr, &dbg_rawdata_len);
        EXAMPLE_DEBUG_LOG_L1("adt confirm calc, gs_len=%d, result=0x%x\r\n", gsensor_soft_fifo_buffer_index, adt_confirm_res);
        if (adt_confirm_res != ADT_CONFRIM_STATUS_DETECTING)
        {
            adt_confirm_flag = false;
            HBD_Stop();
            if (adt_confirm_res == ADT_CONFRIM_STATUS_WEAR)
            {
                gh30x_module_start_without_adt((EMGh30xRunMode)gh30x_run_mode);
                HAL_GH30X_FIFO_INT_TIMEOUT_TIMER_START();
                SEND_MCU_HB_MODE_WEAR_STATUS(WEAR_STATUS_WEAR, (GS32 (*)[DBG_MCU_MODE_PKG_LEN])dbg_rawdata_ptr, dbg_rawdata_len);
                handle_wear_status_result(WEAR_STATUS_WEAR);
            }
            else if (adt_confirm_res == ADT_CONFRIM_STATUS_UNWEAR)
            {
                SEND_MCU_HB_MODE_WEAR_STATUS(WEAR_STATUS_UNWEAR, (GS32 (*)[DBG_MCU_MODE_PKG_LEN])dbg_rawdata_ptr, dbg_rawdata_len);
                gh30x_handle_calc_unwear_status();
            }
        }
        EXAMPLE_LOG_RAWDARA("adt confirm calc:\r\n", dbg_rawdata_ptr, dbg_rawdata_len);
    }
    else
    #endif
	#ifdef __HBD_API_EX__
		{
	    ST_HB_RES stHbRes = {0};
		//GU8 voice_broadcast = 0;
        //GU16 rr_value = 0;
        GU8 hb_res = 0;
		GU8 currentarr[3] = {0};
#if (__ALGO_CALC_WITH_DBG_DATA__)
        GU16 current = 0;
        current = HBD_I2cReadReg(0x0122);
        currentarr[0] = current & 0xFF;
        currentarr[1] = (current >> 8) & 0xFF;
        current = HBD_I2cReadReg(0x0124);
        currentarr[2] = current & 0xFF;
#endif
        hb_res = HBD_HbCalculateByFifoIntEx(gsensor_soft_fifo_buffer, gsensor_soft_fifo_buffer_index, __GS_SENSITIVITY_CONFIG__, 
                        (GS32 (*)[DBG_MCU_MODE_PKG_LEN])dbg_rawdata_ptr, &dbg_rawdata_len, &stHbRes);
        (void)hb_res;
        (void)currentarr;
		handle_hb_mode_result(&stHbRes, (GS32 (*)[DBG_MCU_MODE_PKG_LEN])dbg_rawdata_ptr, dbg_rawdata_len);
        EXAMPLE_DEBUG_LOG_L1("hb calc, gs_len=%d, result=%d,%d,%d,%d\r\n", gsensor_soft_fifo_buffer_index, stHbRes.uchHbValue, stHbRes.uchAccuracyLevel, stHbRes.uchWearingState, stHbRes.uchWearingQuality);
        if (stHbRes.uchWearingState == WEAR_STATUS_UNWEAR)
        {
            SEND_MCU_HB_MODE_RESULT(stHbRes.uchHbValue, stHbRes.uchAccuracyLevel, WEAR_STATUS_UNWEAR, stHbRes.uchWearingQuality, voice_broadcast, hb_res, rr_value, currentarr, 
                                    (GS32 (*)[DBG_MCU_MODE_PKG_LEN])dbg_rawdata_ptr, dbg_rawdata_len, stHbRes.uchScene, stHbRes.uchSNR, stHbRes.uchMotionState, stHbRes.uchSleepFlag);
            HBD_Stop();
            gh30x_handle_calc_unwear_status();
        }
        else
        {
            HAL_GH30X_FIFO_INT_TIMEOUT_TIMER_START();
            SEND_MCU_HB_MODE_RESULT(stHbRes.uchHbValue, stHbRes.uchAccuracyLevel, WEAR_STATUS_WEAR, stHbRes.uchWearingQuality, voice_broadcast, hb_res, rr_value, currentarr, 
                                    (GS32 (*)[DBG_MCU_MODE_PKG_LEN])dbg_rawdata_ptr, dbg_rawdata_len, stHbRes.uchScene, stHbRes.uchSNR, stHbRes.uchMotionState, stHbRes.uchSleepFlag);
        }
        BLE_MODULE_SEND_HB(stHbRes.uchHbValue);
        EXAMPLE_LOG_RAWDARA("hb calc:\r\n", dbg_rawdata_ptr, dbg_rawdata_len);
    }
		#else //__HBD_API_EX__
		
    {
        GU8 hb_value = 0;
        GU8 hb_value_lvl = 0;
        GU8 wearing_state = 0;
        GU8 wearing_quality = 0;
        GU8 voice_broadcast = 0;
        GU16 rr_value = 0;
        GU8 hb_res = 0;
		GU8 currentarr[3] = {0};
#if (__ALGO_CALC_WITH_DBG_DATA__)
        GU16 current = 0;
        current = HBD_I2cReadReg(0x0122);
        currentarr[0] = current & 0xFF;
        currentarr[1] = (current >> 8) & 0xFF;
        current = HBD_I2cReadReg(0x0124);
        currentarr[2] = current & 0xFF;
#endif
        hb_res = HBD_HbCalculateWithLvlByFifoIntDebugOutputData(gsensor_soft_fifo_buffer, gsensor_soft_fifo_buffer_index, __GS_SENSITIVITY_CONFIG__, 
                        &hb_value, &hb_value_lvl, &wearing_state, &wearing_quality, &voice_broadcast, &rr_value, (GS32 (*)[DBG_MCU_MODE_PKG_LEN])dbg_rawdata_ptr, &dbg_rawdata_len);
        (void)hb_res;
        (void)currentarr;
		//handle_hb_mode_result(hb_value, hb_value_lvl, wearing_state, rr_value, (GS32 (*)[DBG_MCU_MODE_PKG_LEN])dbg_rawdata_ptr, dbg_rawdata_len);
        EXAMPLE_DEBUG_LOG_L1("hb calc, gs_len=%d, result=%d,%d,%d,%d\r\n", gsensor_soft_fifo_buffer_index, hb_value, hb_value_lvl, wearing_state, rr_value);
        if (wearing_state == WEAR_STATUS_UNWEAR)
        {
            SEND_MCU_HB_MODE_RESULT(hb_value, hb_value_lvl, WEAR_STATUS_UNWEAR, wearing_quality, voice_broadcast, hb_res, rr_value, currentarr, (GS32 (*)[DBG_MCU_MODE_PKG_LEN])dbg_rawdata_ptr, dbg_rawdata_len);
            HBD_Stop();
            gh30x_handle_calc_unwear_status();
        }
        else
        {
            HAL_GH30X_FIFO_INT_TIMEOUT_TIMER_START();
            SEND_MCU_HB_MODE_RESULT(hb_value, hb_value_lvl, WEAR_STATUS_WEAR, wearing_quality, voice_broadcast, hb_res, rr_value, currentarr, (GS32 (*)[DBG_MCU_MODE_PKG_LEN])dbg_rawdata_ptr, dbg_rawdata_len);
        }
        BLE_MODULE_SEND_HB(hb_value);
        EXAMPLE_LOG_RAWDARA("hb calc:\r\n", dbg_rawdata_ptr, dbg_rawdata_len);
    }
#endif //__HBD_API_EX__
}
#endif

#if (__GET_RAWDATA_ONLY_SUPPORT__)
/// fifo evt getrawdata mode
static void gh30x_fifo_evt_getrawdata_mode_only(GS32 *dbg_rawdata_ptr)
{
    GU16 dbg_rawdata_len = __ALGO_CALC_DBG_BUFFER_LEN__;
    GS32 rawdata[__GET_RAWDATA_BUF_LEN__ ][2];
    GU8 currentarr[3] = {0};
#if (__ALGO_CALC_WITH_DBG_DATA__)
    GU16 current = 0;
    current = HBD_I2cReadReg(0x0122);
    currentarr[0] = current & 0xFF;
    currentarr[1] = (current >> 8) & 0xFF;
    current = HBD_I2cReadReg(0x0124);
    currentarr[2] = current & 0xFF;
#endif
    GU8 nRes = 0;
    nRes = HBD_GetRawdataByFifoInt((GU8) __GET_RAWDATA_BUF_LEN__, (GS32 (*)[2])rawdata, &dbg_rawdata_len);
    GU8 i = 0;
    static GU8 uchPackId = 0;
    for (i = 0; i < dbg_rawdata_len && NULL != dbg_rawdata_ptr; i++)
    {
        dbg_rawdata_ptr[i * 6] = rawdata[i][0];
        dbg_rawdata_ptr[i * 6 + 1] = rawdata[i][1];
        dbg_rawdata_ptr[i * 6 + 2] = 0;
        dbg_rawdata_ptr[i * 6 + 3] = 0;
        dbg_rawdata_ptr[i * 6 + 4] = 0;
        dbg_rawdata_ptr[i * 6 + 5] = uchPackId++;
    }
    (void)currentarr;
    GU8 wearing_state = WEAR_STATUS_WEAR;
    if(1 == nRes)
    {
        wearing_state = WEAR_STATUS_UNWEAR;
    }
    handle_getrawdata_mode_result(wearing_state, rawdata, dbg_rawdata_len);
    EXAMPLE_DEBUG_LOG_L1("get rawdata only, rawdata_len=%d\r\n", dbg_rawdata_len);
    EXAMPLE_DEBUG_LOG_L1("rawdata:%ld,%ld\n",(rawdata[0][0]&0x1ffff),(rawdata[0][1]&0x1ffff));

    if (wearing_state == WEAR_STATUS_UNWEAR)
    {
        SEND_MCU_HB_MODE_RESULT(0, 0, WEAR_STATUS_UNWEAR, 0, 0, 0, 0, currentarr, (GS32 (*)[DBG_MCU_MODE_PKG_LEN])dbg_rawdata_ptr, dbg_rawdata_len);
        HBD_Stop();
        gh30x_handle_calc_unwear_status();
    }
    else
    {
        HAL_GH30X_FIFO_INT_TIMEOUT_TIMER_START();
        SEND_MCU_HB_MODE_RESULT(0, 0, WEAR_STATUS_WEAR, 0, 0, 0, 0, currentarr, (GS32 (*)[DBG_MCU_MODE_PKG_LEN])dbg_rawdata_ptr, dbg_rawdata_len);
    }
    EXAMPLE_LOG_RAWDARA("get rawdata only:\r\n", dbg_rawdata_ptr, dbg_rawdata_len);
}
#endif

#if (__SPO2_DET_SUPPORT__)
#ifdef __HBD_API_EX__
/// fifo evt spo2 mode calc
static void gh30x_fifo_evt_spo2_mode_calc(GS32 *dbg_rawdata_ptr)
{
    GU16 dbg_rawdata_len = __ALGO_CALC_DBG_BUFFER_LEN__;
    ST_SPO2_RES stSpo2Res = {0};
    GU8 abnormal_state = 0;
    (void)abnormal_state;    
    //GU8 currentarr[3] = { 0 };
    #if (__ALGO_CALC_WITH_DBG_DATA__)
	GU16 current = 0;
    current = HBD_I2cReadReg(0x0122);
    currentarr[0] = current & 0xFF;
    currentarr[1] = (current >> 8) & 0xFF;
    current = HBD_I2cReadReg(0x0124);
    currentarr[2] = current & 0xFF;
    #endif
    HBD_Spo2CalculateByFifoIntEx(gsensor_soft_fifo_buffer, gsensor_soft_fifo_buffer_index, __GS_SENSITIVITY_CONFIG__,
                                (GS32 (*)[DBG_MCU_MODE_PKG_LEN])dbg_rawdata_ptr, &dbg_rawdata_len, &stSpo2Res);

    #if (__SPO2_GET_ABN_STATE__)
    abnormal_state = HBD_GetSpo2AbnormalState();
    #endif
    handle_spo2_mode_result(&stSpo2Res, (GS32 (*)[DBG_MCU_MODE_PKG_LEN])dbg_rawdata_ptr, dbg_rawdata_len);
    EXAMPLE_DEBUG_LOG_L1("spo2 calc, gs_len=%d, result=%d,%f,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\r\n", gsensor_soft_fifo_buffer_index, 
                        stSpo2Res.uchSpo2, stSpo2Res.fSpo2, stSpo2Res.uchHbValue, stSpo2Res.uchHbConfidentLvl, 
                        stSpo2Res.usHrvRRVal[0], stSpo2Res.usHrvRRVal[1], stSpo2Res.usHrvRRVal[2], stSpo2Res.usHrvRRVal[3], 
                        stSpo2Res.uchHrvConfidentLvl, stSpo2Res.uchHrvcnt, stSpo2Res.usSpo2RVal, stSpo2Res.uchWearingState);
    
    if (stSpo2Res.uchWearingState == WEAR_STATUS_UNWEAR)
    {
        SEND_MCU_SPO2_UNWEAR_EVT((GS32 (*)[DBG_MCU_MODE_PKG_LEN])dbg_rawdata_ptr, dbg_rawdata_len);						
        HBD_Stop();
        gh30x_handle_calc_unwear_status();
    }
    else
    {
        HAL_GH30X_FIFO_INT_TIMEOUT_TIMER_START();
        FIXED_SPO2_ALGO_RES(stSpo2Res.uchSpo2);
        SEND_MCU_SPO2_MODE_RESULT(&stSpo2Res, abnormal_state, currentarr, (GS32 (*)[DBG_MCU_MODE_PKG_LEN])dbg_rawdata_ptr, dbg_rawdata_len);
    }
    EXAMPLE_LOG_RAWDARA("spo2 calc:\r\n", dbg_rawdata_ptr, dbg_rawdata_len);
}
#else
/// fifo evt spo2 mode calc
static void gh30x_fifo_evt_spo2_mode_calc(GS32 *dbg_rawdata_ptr)
{
    GU16 dbg_rawdata_len = __ALGO_CALC_DBG_BUFFER_LEN__;
    GU8 spo2_value = 0;
    GU8 spo2_lvl = 0;
    GU8 hb_value = 0;
    GU8 hb_lvl = 0;
    GU16 hrv_val[4] = {0};
    GU8 hrv_lvl = 0;
    GU8 hrv_cnt = 0; 
    GU16 spo2_r_value = 0;
    GU8 wearing_state = 0;
    GU8 valid_lvl = 0;
    GU8 abnormal_state = 0;
    (void)abnormal_state;    
    GU8 currentarr[3] = { 0 };
    #if (__ALGO_CALC_WITH_DBG_DATA__)
	GU16 current = 0;
    current = HBD_I2cReadReg(0x0122);
    currentarr[0] = current & 0xFF;
    currentarr[1] = (current >> 8) & 0xFF;
    current = HBD_I2cReadReg(0x0124);
    currentarr[2] = current & 0xFF;
    #endif

    HBD_Spo2CalculateByFifoIntDbgRawdataInnerUse(gsensor_soft_fifo_buffer, gsensor_soft_fifo_buffer_index, __GS_SENSITIVITY_CONFIG__, 
                        &spo2_value, &spo2_lvl, &hb_value, &hb_lvl, &hrv_val[0], &hrv_val[1], &hrv_val[2], &hrv_val[3], &hrv_lvl, &hrv_cnt,
                        &spo2_r_value, &wearing_state, (GS32 (*)[DBG_MCU_MODE_PKG_LEN])dbg_rawdata_ptr, &dbg_rawdata_len, &valid_lvl);

    #if (__SPO2_GET_ABN_STATE__)
    abnormal_state = HBD_GetSpo2AbnormalState();
    #endif
    handle_spo2_mode_result(spo2_value, spo2_lvl, hb_value, hb_lvl, hrv_val, hrv_lvl, hrv_cnt, spo2_r_value, wearing_state, (GS32 (*)[DBG_MCU_MODE_PKG_LEN])dbg_rawdata_ptr, dbg_rawdata_len);
    //EXAMPLE_DEBUG_LOG_L1("fspo2 : %f\r\n", fSpo2Value);
    EXAMPLE_DEBUG_LOG_L1("spo2 calc, gs_len=%d, result=%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\r\n", gsensor_soft_fifo_buffer_index, spo2_value, spo2_lvl, hb_value, hb_lvl, hrv_val[0], hrv_val[1], hrv_val[2], 
                                            hrv_val[3], hrv_lvl, hrv_cnt, spo2_r_value, wearing_state);
    
    if (wearing_state == WEAR_STATUS_UNWEAR)
    {
        SEND_MCU_SPO2_UNWEAR_EVT((GS32 (*)[DBG_MCU_MODE_PKG_LEN])dbg_rawdata_ptr, dbg_rawdata_len);						
        HBD_Stop();
        gh30x_handle_calc_unwear_status();
    }
    else
    {
        HAL_GH30X_FIFO_INT_TIMEOUT_TIMER_START();
        FIXED_SPO2_ALGO_RES(spo2_value);
        SEND_MCU_SPO2_MODE_RESULT(spo2_value, spo2_lvl, hb_value, hb_lvl, hrv_val, hrv_lvl, hrv_cnt, spo2_r_value, 
                                  wearing_state, valid_lvl, abnormal_state, currentarr, (GS32 (*)[DBG_MCU_MODE_PKG_LEN])dbg_rawdata_ptr, dbg_rawdata_len);
    }
    EXAMPLE_LOG_RAWDARA("spo2 calc:\r\n", dbg_rawdata_ptr, dbg_rawdata_len);
}
#endif //#ifdef __HBD_API_EX__
#endif //#if (__SPO2_DET_SUPPORT__)

#if (__HRV_DET_SUPPORT__)
/// fifo evt hrv mode calc
#ifdef __HBD_API_EX__
static void gh30x_fifo_evt_hrv_mode_calc(GS32 *dbg_rawdata_ptr)
{
    GU16 dbg_rawdata_len = __ALGO_CALC_DBG_BUFFER_LEN__;
	  ST_HB_RES stHbRes = {0};
		ST_HRV_RES stHrvRes = {0};
    //GU16 current = 0;
    //GU8 currentarr[3] = { 0 };
    #if (__ALGO_CALC_WITH_DBG_DATA__)
    current = HBD_I2cReadReg(0x0122);
    currentarr[0] = current & 0xFF;
    currentarr[1] = (current >> 8) & 0xFF;
    current = HBD_I2cReadReg(0x0124);
    currentarr[2] = current & 0xFF;
    #endif
    //hrv_rr_value_fresh_cnt = HBD_HrvCalculateWithLvlByFifoIntDbgRawdata(gsensor_soft_fifo_buffer, gsensor_soft_fifo_buffer_index, __GS_SENSITIVITY_CONFIG__, hrv_rr_value_array, &hrv_lvl, (GS32 (*)[DBG_MCU_MODE_PKG_LEN])dbg_rawdata_ptr, &dbg_rawdata_len);
		HBD_HbWithHrvCalculateByFifoIntDbgDataEx(gsensor_soft_fifo_buffer, gsensor_soft_fifo_buffer_index, __GS_SENSITIVITY_CONFIG__, 
                                             &stHbRes, &stHrvRes, (GS32 (*)[DBG_MCU_MODE_PKG_LEN])dbg_rawdata_ptr, &dbg_rawdata_len);
    if (stHbRes.uchWearingState == WEAR_STATUS_UNWEAR)
    {
        stHrvRes.usRRvalueArr[0] = 255;
        stHrvRes.usRRvalueArr[1] = 255;
        stHrvRes.usRRvalueArr[2] = 255;
        stHrvRes.usRRvalueArr[3] = 255;
        SEND_MCU_HRV_MODE_RESULT(stHrvRes.usRRvalueArr, 0, 0, 255, currentarr, 0, 0);
        gh30x_unwear_evt_handler();
    }
    else
    {
        HAL_GH30X_FIFO_INT_TIMEOUT_TIMER_START();
        handle_hrv_mode_result(stHrvRes.usRRvalueArr, stHrvRes.uchRRvalueCnt, stHrvRes.uchHrvConfidentLvl, (GS32 (*)[DBG_MCU_MODE_PKG_LEN])dbg_rawdata_ptr, dbg_rawdata_len);
        //handle_hb_mode_result(hb_value, 0, WEAR_STATUS_WEAR, 0, 0, 0);
		handle_hb_mode_result(&stHbRes, 0, 0);
        SEND_MCU_HRV_MODE_RESULT(stHrvRes.usRRvalueArr, stHrvRes.uchRRvalueCnt, stHrvRes.uchHrvConfidentLvl, stHbRes.uchHbValue, currentarr, (GS32 (*)[DBG_MCU_MODE_PKG_LEN])dbg_rawdata_ptr, dbg_rawdata_len);
        EXAMPLE_DEBUG_LOG_L1("hrv calc, result=%d,%d,%d,%d,%d,%d\r\n", stHrvRes.usRRvalueArr[0], stHrvRes.usRRvalueArr[1], stHrvRes.usRRvalueArr[2], stHrvRes.usRRvalueArr[3], stHrvRes.uchRRvalueCnt, stHrvRes.uchHrvConfidentLvl); //just print 4 result
    }
    BLE_MODULE_SEND_RRI(stHrvRes.usRRvalueArr, stHrvRes.uchRRvalueCnt);
}
#else //#ifdef __HBD_API_EX__
static void gh30x_fifo_evt_hrv_mode_calc(GS32 *dbg_rawdata_ptr)
{
    GU16 dbg_rawdata_len = __ALGO_CALC_DBG_BUFFER_LEN__;
    GU16 hrv_rr_value_array[HRV_MODE_RES_MAX_CNT] = {0};
    GU8 hrv_rr_value_fresh_cnt = 0;
    GU8 hrv_lvl = 0;
    GU8 hb_value = 0;
    GU8 hb_value_lvl = 0;
    GU8 wearing_state = 0;
    GU8 wearing_quality = 0;
    GU8 voice_broadcast = 0;
    GU16 current = 0;
    GU8 currentarr[3] = { 0 };
    #if (__ALGO_CALC_WITH_DBG_DATA__)
    current = HBD_I2cReadReg(0x0122);
    currentarr[0] = current & 0xFF;
    currentarr[1] = (current >> 8) & 0xFF;
    current = HBD_I2cReadReg(0x0124);
    currentarr[2] = current & 0xFF;
    #endif
    //hrv_rr_value_fresh_cnt = HBD_HrvCalculateWithLvlByFifoIntDbgRawdata(gsensor_soft_fifo_buffer, gsensor_soft_fifo_buffer_index, __GS_SENSITIVITY_CONFIG__, hrv_rr_value_array, &hrv_lvl, (GS32 (*)[DBG_MCU_MODE_PKG_LEN])dbg_rawdata_ptr, &dbg_rawdata_len);
    HBD_HbWithHrvCalculateByFifoIntDbgData(gsensor_soft_fifo_buffer, gsensor_soft_fifo_buffer_index, __GS_SENSITIVITY_CONFIG__, \
                                                                    &hb_value, &hb_value_lvl, &wearing_state, &wearing_quality, &voice_broadcast, \
                                                                    hrv_rr_value_array, &hrv_rr_value_fresh_cnt, &hrv_lvl, \
                                                                    (GS32 (*)[DBG_MCU_MODE_PKG_LEN])dbg_rawdata_ptr, &dbg_rawdata_len);
    if (wearing_state == WEAR_STATUS_UNWEAR)
    {
        hrv_rr_value_array[0] = 255;
        hrv_rr_value_array[1] = 255;
        hrv_rr_value_array[2] = 255;
        hrv_rr_value_array[3] = 255;
        SEND_MCU_HRV_MODE_RESULT(hrv_rr_value_array, 0, 0, 255, currentarr, 0, 0);
        gh30x_unwear_evt_handler();
    }
    else
    {
        HAL_GH30X_FIFO_INT_TIMEOUT_TIMER_START();
        handle_hrv_mode_result(hrv_rr_value_array, hrv_rr_value_fresh_cnt, hrv_lvl, (GS32 (*)[DBG_MCU_MODE_PKG_LEN])dbg_rawdata_ptr, dbg_rawdata_len);
        handle_hb_mode_result(hb_value, 0, WEAR_STATUS_WEAR, 0, 0, 0);
        SEND_MCU_HRV_MODE_RESULT(hrv_rr_value_array, hrv_rr_value_fresh_cnt, hrv_lvl, hb_value, currentarr, (GS32 (*)[DBG_MCU_MODE_PKG_LEN])dbg_rawdata_ptr, dbg_rawdata_len);
        EXAMPLE_DEBUG_LOG_L1("hrv calc, result=%d,%d,%d,%d,%d\r\n", hrv_rr_value_array[0], hrv_rr_value_array[1], hrv_rr_value_array[2], hrv_rr_value_array[3], 
                                                                    hrv_rr_value_fresh_cnt); //just print 4 result
    }
    BLE_MODULE_SEND_RRI(hrv_rr_value_array, hrv_rr_value_fresh_cnt);
}
#endif //#ifdef __HBD_API_EX__
#endif

#if (__BPF_DET_SUPPORT__)
/// fifo evt hrv mode calc
static void gh30x_fifo_evt_bpf_mode_calc(GS32 *dbg_rawdata_ptr)
{
    GU16 dbg_rawdata_len = __ALGO_CALC_DBG_BUFFER_LEN__;
    ST_BPF_RES stBPFRes;
    #if (__ALGO_CALC_WITH_DBG_DATA__)
    GU16 current = 0;
    GU8 currentarr[3] = { 0 };
    current = HBD_I2cReadReg(0x0122);
    currentarr[0] = current & 0xFF;
    currentarr[1] = (current >> 8) & 0xFF;
    current = HBD_I2cReadReg(0x0124);
    currentarr[2] = current & 0xFF;
    #endif
    GU8 nRes = HBD_BpfCalculateByFifoIntDbg(gsensor_soft_fifo_buffer, gsensor_soft_fifo_buffer_index, __GS_SENSITIVITY_CONFIG__, \
                                 &stBPFRes, (GS32 (*)[DBG_MCU_MODE_PKG_LEN])dbg_rawdata_ptr, &dbg_rawdata_len);
    handle_bpf_mode_result(nRes, &stBPFRes, (GS32 (*)[DBG_MCU_MODE_PKG_LEN])dbg_rawdata_ptr, dbg_rawdata_len);
    SEND_MCU_BPF_MODE_RESULT(&stBPFRes, currentarr, (GS32 (*)[DBG_MCU_MODE_PKG_LEN])dbg_rawdata_ptr, dbg_rawdata_len);
    EXAMPLE_DEBUG_LOG_L1("glen: %d, rlen: %d\r\n", gsensor_soft_fifo_buffer_index, dbg_rawdata_len);
    if(stBPFRes.uchWearingState == WEAR_STATUS_UNWEAR)
    {
        gh30x_unwear_evt_handler();
    }
    else
    {
        HAL_GH30X_FIFO_INT_TIMEOUT_TIMER_START();
        EXAMPLE_DEBUG_LOG_L1("BPF res num: %d, Confidence: %d AF:%d %d, Res Refresh: %d\r\n", stBPFRes.uchResultNum, stBPFRes.uchConfidence, stBPFRes.uchAFib, stBPFRes.uchAFibConfidence, nRes);
        for(GS16 i=0; i<stBPFRes.uchResultNum && 1 == nRes; ++i)
        {
            EXAMPLE_DEBUG_LOG_L1(" MaxSlope: %d, MaxSlopeTime: %d, RRI: %d, AC: %d, SystolicTime: %d, DiastolicTime: %d, A1: %d, A2: %d\r\n", 
                                 stBPFRes.usMaxSlope[i], stBPFRes.usMaxSlopeTime[i], stBPFRes.usRRI[i], stBPFRes.usAC[i], 
                                 stBPFRes.usSystolicTime[i], stBPFRes.usDiastolicTime[i], (int)stBPFRes.nA1[i], (int)stBPFRes.nA2[i] );
        }
    }
    EXAMPLE_LOG_RAWDARA("bpf calc:\r\n", dbg_rawdata_ptr, dbg_rawdata_len);
}
#endif


/// gh30x fifo evt handler
void gh30x_fifo_evt_handler(void)
{
    GS32 *dbg_rawdata_buffer_ptr = NULL;
    #if (__DBG_OUTPUT_RAWDATA__)
    GS32 dbg_rawdata_buffer[__ALGO_CALC_DBG_BUFFER_LEN__][DBG_MCU_MODE_PKG_LEN];
    dbg_rawdata_buffer_ptr = (GS32 *)dbg_rawdata_buffer;
    #endif
    EXAMPLE_DEBUG_LOG_L1("got gh30x fifo evt, func[%s]\r\n", dbg_rum_mode_string[gh30x_run_mode]);
    gsensor_read_fifo_data();
    #if (USE_FAKE_GS_DATA_FOR_TEST)
    gh30x_get_sin_gsensor_data(1.5, 1, 1);
    #endif
    switch (gh30x_run_mode)
    {
    #if (__HB_DET_SUPPORT__)
        case RUN_MODE_ADT_HB_DET:
            gh30x_fifo_evt_hb_mode_calc(dbg_rawdata_buffer_ptr);
            break;
    #endif

    #if (__SPO2_DET_SUPPORT__)
        case RUN_MODE_SPO2_DET:
            gh30x_fifo_evt_spo2_mode_calc(dbg_rawdata_buffer_ptr);
            break; 
    #endif

    #if (__HRV_DET_SUPPORT__)
        case RUN_MODE_HRV_DET:
            gh30x_fifo_evt_hrv_mode_calc(dbg_rawdata_buffer_ptr);
            break;      
    #endif
        
    #if (__BPF_DET_SUPPORT__)
        case RUN_MODE_BPF_DET:
            gh30x_fifo_evt_bpf_mode_calc(dbg_rawdata_buffer_ptr);
            break;      
    #endif
	
	#if (__GET_RAWDATA_ONLY_SUPPORT__)
        case RUN_MODE_GETRAWDATA_DET:
            gh30x_fifo_evt_getrawdata_mode_only(dbg_rawdata_buffer_ptr);
            break;
    #endif

        default:
            EXAMPLE_DEBUG_LOG_L1("clac that mode[%s] is not support!\r\n", dbg_rum_mode_string[gh30x_run_mode]);
            break;   
    }
    gsensor_drv_clear_buffer(gsensor_soft_fifo_buffer, &gsensor_soft_fifo_buffer_index, GSENSOR_SOFT_FIFO_BUFFER_MAX_LEN);
}

/// gh30x newdata evt handler
void gh30x_new_data_evt_handler(void)
{
    ST_GS_DATA_TYPE gsensor_data;
    gsensor_drv_get_data(&gsensor_data); // get gsensor data
    #if (USE_FAKE_GS_DATA_FOR_TEST)
    gh30x_get_sin_gsensor_data(1.5, 1, 0);
    gsensor_data.sXAxisVal = gsensor_soft_fifo_buffer[0].sXAxisVal;
    gsensor_data.sYAxisVal = gsensor_soft_fifo_buffer[0].sYAxisVal;
    gsensor_data.sZAxisVal = gsensor_soft_fifo_buffer[0].sZAxisVal;
    #endif
    #if (__USE_GOODIX_APP__)
	if ((goodix_app_start_app_mode) && ((gh30x_run_mode == RUN_MODE_ADT_HB_DET) || (gh30x_run_mode == RUN_MODE_HRV_DET) || (gh30x_run_mode == RUN_MODE_SPO2_DET) || (gh30x_run_mode == RUN_MODE_GETRAWDATA_DET)))
	{
        EXAMPLE_DEBUG_LOG_L2("got gh30x new data evt, send rawdata to app\n");
        if (GH30X_AUTOLED_ERR_VAL == HBD_SendRawdataPackageByNewdataInt(&gsensor_data, __GS_SENSITIVITY_CONFIG__))
        {
            SEND_AUTOLED_FAIL_EVT();
        }
    }
    else
    #endif
    {
        #if (__USE_GOODIX_APP__)
        goodix_app_start_app_mode = false; // if call stop, clear app mode
        #endif

        #if (__SYSTEM_TEST_SUPPORT__)
        if (goodix_system_test_mode)
        {
            if(!ledmask[goodix_system_test_os_led_num])
            {
                if(goodix_system_test_os_led_num<2)
                {
                    goodix_system_test_os_led_num++;
                    gh30x_systemtest_os_start(goodix_system_test_os_led_num);
                }
                else
                {
                    gh30x_systemtest_part2_handle(0);
                }
            }
            EXAMPLE_DEBUG_LOG_L1("got gh30x new data evt, put data to system test module.\r\n");
            GU8 os_test_ret = gh30x_systemtest_os_calc(goodix_system_test_os_led_num);
            if (os_test_ret != 0xFF) // test has done
            {
                EXAMPLE_DEBUG_LOG_L1("system test os[led %d] ret: %d!\r\n", goodix_system_test_os_led_num, os_test_ret);
                if (goodix_system_test_os_led_num < 2&&(!os_test_ret))
                {
                    goodix_system_test_os_led_num++;
                    gh30x_systemtest_os_start(goodix_system_test_os_led_num);
                    EXAMPLE_DEBUG_LOG_L1("system test change to test next led:%d!\r\n", goodix_system_test_os_led_num);
                }
                else
                {
//                    goodix_system_test_mode = false;
//                    EXAMPLE_DEBUG_LOG_L1("system test has done!\r\n");
                    HBD_Stop();
                    gh30x_systemtest_part2_handle(os_test_ret);
                }
            }
        }
        else
        #endif
        {
            EXAMPLE_DEBUG_LOG_L1("got gh30x new data evt, shouldn't reach here!!\r\n");
            gh30x_handle_calc_unwear_status();
        }
    }
}

/// gh30x fifo full evt handler
void gh30x_fifo_full_evt_handler(void)
{
    HBD_Stop();
    gsensor_enter_clear_buffer_and_enter_fifo();
    gh30x_start_func_with_mode(gh30x_run_mode);
    EXAMPLE_DEBUG_LOG_L1("got gh30x fifo full evt, func[%s],  shouldn't reach here!!\r\n", dbg_rum_mode_string[gh30x_run_mode]);
}

/// gh30x int msg handler
void gh30x_int_msg_handler(void)
{
	GU8 gh30x_irq_status;
    GU8 gh30x_adt_working_flag;
    HAL_GH30X_FIFO_INT_TIMEOUT_TIMER_STOP();
    gh30x_irq_status = HBD_GetIntStatus();
	gh30x_adt_working_flag = HBD_IsAdtWearDetectHasStarted();

    if (gh30x_irq_status == INT_STATUS_FIFO_WATERMARK)
    {
        gh30x_fifo_evt_handler();
    }
    else if (gh30x_irq_status == INT_STATUS_NEW_DATA)
    {      
        gh30x_new_data_evt_handler();
    }
    else if (gh30x_irq_status == INT_STATUS_WEAR_DETECTED)
    {
        gh30x_wear_evt_handler();
    }
    else if (gh30x_irq_status == INT_STATUS_UNWEAR_DETECTED)
    {
        gh30x_unwear_evt_handler();
    }
    else if (gh30x_irq_status == INT_STATUS_CHIP_RESET) // if gh30x reset, need reinit
    {
		gh30x_reset_evt_handler();
    }
	else if (gh30x_irq_status == INT_STATUS_FIFO_FULL) // if gh30x fifo full, need restart
    {
        gh30x_fifo_full_evt_handler();
    }
	
	if ((gh30x_adt_working_flag == 1) && (gh30x_irq_status != INT_STATUS_WEAR_DETECTED) && (gh30x_irq_status != INT_STATUS_UNWEAR_DETECTED)) // adt working
	{
		gh30x_start_adt_with_mode(gh30x_run_mode);
	}
}

/// gh30x fifo int timeout msg handler
void gh30x_fifo_int_timeout_msg_handler(void)
{
    GU8 gh30x_irq_status_1;
	GU8 gh30x_irq_status_2;

    HAL_GH30X_FIFO_INT_TIMEOUT_TIMER_STOP();

	EXAMPLE_DEBUG_LOG_L1("fifo int time out!!!\r\n");

	gh30x_irq_status_1 = HBD_GetIntStatus();
	gh30x_irq_status_2 = HBD_GetIntStatus();
    if ((gh30x_irq_status_1 == INT_STATUS_FIFO_WATERMARK) && (gh30x_irq_status_2 == INT_STATUS_INVALID))
	{
		gh30x_fifo_evt_handler();
	}
	else
    {
        gh30x_reset_evt_handler();
    }
}

/// communicate parse handler
void gh30x_communicate_parse_handler(GS8 communicate_type, GU8 *buffer, GU8 length) 
{
    EM_COMM_CMD_TYPE comm_cmd_type  = HBD_CommParseHandler(communicate_type, buffer, length); // parse recv data
    if (communicate_type == (GS8)COMM_TYPE_INVALID_VAL)
    {
        EXAMPLE_DEBUG_LOG_L1("comm_type error, pelase check inited or not, @ref gh30x_module_init!!!\r\n");
    }
    else
    {
        EXAMPLE_DEBUG_LOG_L1("parse: cmd[%x-%s], comm_type[%d], length[%d]\r\n", buffer[0], dbg_comm_cmd_string[(uint8_t)comm_cmd_type], communicate_type, length);
    }
    if (comm_cmd_type < COMM_CMD_INVALID)
    {
        handle_goodix_communicate_cmd(comm_cmd_type);
        if ((comm_cmd_type == COMM_CMD_ALGO_IN_APP_HB_START) 
            || (comm_cmd_type == COMM_CMD_ALGO_IN_APP_HRV_START) 
            || ( comm_cmd_type == COMM_CMD_ALGO_IN_APP_SPO2_START)) // handle all app mode cmd
        {
            GU8 app_start_mode = RUN_MODE_INVALID;
            goodix_app_start_app_mode = true;
            if (comm_cmd_type == COMM_CMD_ALGO_IN_APP_HB_START)
            {
                app_start_mode = RUN_MODE_ADT_HB_DET;
            }
            else if (comm_cmd_type == COMM_CMD_ALGO_IN_APP_HRV_START)
            {
                app_start_mode = RUN_MODE_HRV_DET;
            }
            else if (comm_cmd_type == COMM_CMD_ALGO_IN_APP_SPO2_START)
            {
                app_start_mode = RUN_MODE_SPO2_DET;
            }
            HBD_FifoConfig(0, HBD_FUNCTIONAL_STATE_DISABLE);
            HBD_FifoConfig(1, HBD_FUNCTIONAL_STATE_DISABLE);
            gh30x_start_func_whithout_adt_confirm(app_start_mode);
            HBD_FifoConfig(0, HBD_FUNCTIONAL_STATE_ENABLE);
            HBD_FifoConfig(1, HBD_FUNCTIONAL_STATE_ENABLE);
        }
        else if ((comm_cmd_type == COMM_CMD_ADT_SINGLE_MODE_START)
            || (comm_cmd_type == COMM_CMD_ALGO_IN_MCU_HRV_START)
            || (comm_cmd_type == COMM_CMD_ALGO_IN_MCU_SPO2_START)) // handle mcu mode cmd
        {
            GU8 mcu_start_mode = RUN_MODE_INVALID;
            goodix_app_start_app_mode = false;
            if (comm_cmd_type == COMM_CMD_ADT_SINGLE_MODE_START)
            {
                mcu_start_mode = RUN_MODE_ADT_HB_DET;
            }
            else if (comm_cmd_type == COMM_CMD_ALGO_IN_MCU_HRV_START)
            {
                mcu_start_mode = RUN_MODE_HRV_DET;
            }
            else if (comm_cmd_type == COMM_CMD_ALGO_IN_MCU_SPO2_START)
            {
                mcu_start_mode = RUN_MODE_SPO2_DET;
            }
            gh30x_module_start((EMGh30xRunMode)mcu_start_mode);
        }
#if (__BPF_DET_SUPPORT__)
        else if (comm_cmd_type == COMM_CMD_ALGO_IN_MCU_BP_START) // handle bpf mcu mode cmd
        {
            goodix_app_start_app_mode = false;
            gh30x_module_start((EMGh30xRunMode)RUN_MODE_BPF_DET);
        }
#endif
        else // handle all stop cmd
        {
            goodix_app_start_app_mode = false;
            gh30x_module_stop();
        }
    }
}

/// enter normal mode and clear fifo buffer
void gsensor_enter_normal_and_clear_buffer(void)
{
    gsensor_drv_enter_normal_mode();  
	gsensor_drv_clear_buffer(gsensor_soft_fifo_buffer, &gsensor_soft_fifo_buffer_index, GSENSOR_SOFT_FIFO_BUFFER_MAX_LEN);
}

/// clear fifo buffer and enter fifo mode
void gsensor_enter_clear_buffer_and_enter_fifo(void)
{
    gsensor_drv_enter_normal_mode();
    gsensor_drv_clear_buffer(gsensor_soft_fifo_buffer, &gsensor_soft_fifo_buffer_index, GSENSOR_SOFT_FIFO_BUFFER_MAX_LEN);
    gsensor_drv_enter_fifo_mode();
}

/// motion detect irq handler
void gsensor_motion_has_detect(void)
{ 
    gsensor_enter_normal_and_clear_buffer();
    gh30x_start_adt_with_mode(gh30x_run_mode);
    EXAMPLE_DEBUG_LOG_L1("got gsensor motion evt, start adt [%s]\r\n", dbg_rum_mode_string[gh30x_run_mode]);
}

/// get data into fifo buffer
void gsensor_read_fifo_data(void)
{
    gsensor_drv_get_fifo_data(gsensor_soft_fifo_buffer, &gsensor_soft_fifo_buffer_index, GSENSOR_SOFT_FIFO_BUFFER_MAX_LEN);
}

/// start gh30x adt func with adt_run_mode
void gh30x_start_adt_with_mode(uint8_t adt_run_mode)
{
    const ST_REGISTER *reg_config_ptr = NULL;
    uint16_t reg_config_len = 0;
    switch (adt_run_mode)
    {
    #if (__HB_DET_SUPPORT__)
        case RUN_MODE_ADT_HB_DET:
            #if (__HB_NEED_ADT_CONFIRM__)
            if (hb_start_without_adt_confirm)
            {
                reg_config_ptr = hb_reg_config_array;
                reg_config_len = hb_reg_config_array_len;
            }
            else
            {
                reg_config_ptr = hb_adt_confirm_reg_config;
                reg_config_len = hb_adt_confirm_reg_config_len;
            } 
            #else
                reg_config_ptr = hb_reg_config_array;
                reg_config_len = hb_reg_config_array_len;
            #endif
            break;
    #endif

    #if (__SPO2_DET_SUPPORT__)
        case RUN_MODE_SPO2_DET:
            reg_config_ptr = spo2_reg_config_array;
            reg_config_len = spo2_reg_config_array_len;
            break; 
    #endif

    #if (__HRV_DET_SUPPORT__)
        case RUN_MODE_HRV_DET:
            reg_config_ptr = hrv_reg_config_array;
            reg_config_len = hrv_reg_config_array_len;
            break;      
    #endif

    #if (__BPF_DET_SUPPORT__)
        case RUN_MODE_BPF_DET:
            reg_config_ptr = bpf_reg_config_array;
            reg_config_len = bpf_reg_config_array_len;
            break;      
    #endif
	
	#if (__GET_RAWDATA_ONLY_SUPPORT__)
        case RUN_MODE_GETRAWDATA_DET:
            reg_config_ptr = getrawdata_reg_config_array;
            reg_config_len = getrawdata_reg_config_array_len;
            break;      
    #endif

        default:
            EXAMPLE_DEBUG_LOG_L1("adt start that mode[%s] is not support!\r\n", dbg_rum_mode_string[gh30x_run_mode]);
            break;   
    }
    gh30x_adt_wear_detect_start(reg_config_ptr, reg_config_len);
}

/// start gh30x func with func_run_mode
void gh30x_start_func_with_mode(uint8_t func_run_mode)
{
    switch (func_run_mode)
    {
    #if (__HB_DET_SUPPORT__)
        case RUN_MODE_ADT_HB_DET:
            #if (__HB_NEED_ADT_CONFIRM__)
                if (hb_start_without_adt_confirm)
                {
                    hb_start_without_adt_confirm = false;
                    #if (__USE_GOODIX_APP__)
                    if (!goodix_app_start_app_mode)
                    {
                        gh30x_Load_new_config(hb_reg_config_array, hb_reg_config_array_len);
                    }
                    #endif
                    gh30x_hb_start();
                }
                else
                {
                    gh30x_adt_confirm_start();
                    adt_confirm_flag = true;
                }
            #else
                gh30x_hb_start();
            #endif
            break;
    #endif

    #if (__SPO2_DET_SUPPORT__)
        case RUN_MODE_SPO2_DET:
            gh30x_spo2_start();
            break; 
    #endif

    #if (__HRV_DET_SUPPORT__)
        case RUN_MODE_HRV_DET:
            gh30x_hrv_start();
            break;      
    #endif
        
    #if (__BPF_DET_SUPPORT__)
        case RUN_MODE_BPF_DET:
            gh30x_bpf_start();
            break;      
    #endif
	
	#if (__GET_RAWDATA_ONLY_SUPPORT__)
        case RUN_MODE_GETRAWDATA_DET:
            gh30x_getrawdata_start();
            break; 
    #endif

        default:
            EXAMPLE_DEBUG_LOG_L1("func start that mode[%s] is not support!\r\n", dbg_rum_mode_string[gh30x_run_mode]);
            break;   
    } 
}

/// stop gh30x func
void gh30x_stop_func(void)
{
    #if (__HB_NEED_ADT_CONFIRM__)
    hb_start_without_adt_confirm = false;
    adt_confirm_flag = false;
    #endif
    HAL_GH30X_FIFO_INT_TIMEOUT_TIMER_STOP();
    HBD_Stop();
    #if (__HBD_USE_DYN_MEM__)
    gh30x_free_memory();
    #endif
}

/// gh30x start func fix adt confirm
void gh30x_start_func_whithout_adt_confirm(uint8_t start_run_mode)
{
    #if ((__HB_DET_SUPPORT__) && (__HB_NEED_ADT_CONFIRM__))
    if (start_run_mode == RUN_MODE_ADT_HB_DET)
    {
        hb_start_without_adt_confirm = true;
    }
    #endif
    gsensor_enter_normal_and_clear_buffer();
    gh30x_start_func_with_mode(start_run_mode);
    gh30x_run_mode = start_run_mode;
    EXAMPLE_DEBUG_LOG_L1("gh30x module start without adt confirm, mode [%s]\r\n", dbg_rum_mode_string[gh30x_run_mode]);
}


#if (__SYSTEM_TEST_SUPPORT__)
/// gh30x module system test os check
void gh30x_module_system_test_os_start(void)
{
    handle_before_system_os_test();
    gh30x_module_stop();
    goodix_system_test_os_led_num = 0;
    gh30x_systemtest_os_start(goodix_system_test_os_led_num);
    EXAMPLE_DEBUG_LOG_L1("system test os check start\r\n");
}
#endif


//gh30x module system test
void gh30x_systemtest_start(EMGh30xTestItem mode)
{	
#if (__SYSTEM_TEST_SUPPORT__)
    #if __PLATFORM_DELAY_US_CONFIG__
    EXAMPLE_DEBUG_LOG_L1("delay has been out");
    HBDTEST_set_delayFunc(&hal_gh30x_delay_us);
    #endif
    uint8_t ret=0;
    if(!mode)
    {
        handle_system_test_result(EN_PARAM_FAIL,0);
        EXAMPLE_DEBUG_LOG_L1("system test check fail, ret = %d\r\n", EN_PARAM_FAIL);
    }
	if(mode & 0x1)
    {
        gh30x_module_stop();
        ret = gh30x_systemtest_comm_check();
        if(ret)
        {
            handle_system_test_result(ret,0);
            return;
        }
        EXAMPLE_DEBUG_LOG_L1("system test comm check, ret = %d\r\n", ret);
    }
    if(mode & 0x2)
    {
        gh30x_module_stop();
        ret = gh30x_systemtest_otp_check();
        if(ret)
        {
            handle_system_test_result(ret,0);
            return;
        }
        EXAMPLE_DEBUG_LOG_L1("system test otp check, ret = %d\r\n", ret);
    }
    if(mode & 0xc)
    {
        goodix_system_test_mode = (mode & 0x1c) >> 1;
        EXAMPLE_DEBUG_LOG_L1("begin goodix_system_test_mode is %d,mode is %d!\r\n",goodix_system_test_mode,mode);
        HBDTEST_ROMALEDCheckData *hbdatalst[]={&led0std,&led1std,&led2std};
        for(int i=0; i<3; i++)
        {
            gh30x_systemtest_param_set(i,&hbdatalst[i]->_param);
        }
        gh30x_module_system_test_os_start();
        EXAMPLE_DEBUG_LOG_L1("come to second part.\n");
    }
    else
    {
        handle_system_test_result(0,0);
    }
#else
    EXAMPLE_DEBUG_LOG_L1("__SYSTEM_TEST_SUPPORT__ disabled in config\r\n");
#endif
}

#if (__SYSTEM_TEST_SUPPORT__)
void gh30x_systemtest_part2_handle(GU8 ret)
{
    EXAMPLE_DEBUG_LOG_L1("now goodix_system_test_mode is %d!\r\n",goodix_system_test_mode);
    for(GS16 i=0;i<3;i++)
    {
        if(goodix_system_test_mode & (1<<i) )
        {
            goodix_system_test_mode^=(1<<i);
            if(i==2&& goodix_system_test_mode&0x8)
            {
                goodix_system_test_mode^=(1<<3);
            }
            break;
        }
    }
    EXAMPLE_DEBUG_LOG_L1("now goodix_system_test_mode is %d!\r\n",goodix_system_test_mode);
    if(goodix_system_test_mode&&(!ret))
    {
        gh30x_module_system_test_os_start();
    }
    else
    {
        goodix_system_test_mode=0;
        EXAMPLE_DEBUG_LOG_L1("system part2 test has done!\r\n");
        HBD_Stop();
        handle_system_test_result(ret,0);
    }
}
#endif

///generate fake G-sensor sin data
void gh30x_get_sin_gsensor_data(float Hz, uint8_t range, uint8_t fifomode)
{
    static int ntime = 0;
    int ms = 1;
    int nGH30Xclk = 32768;
    int nSampleRate = nGH30Xclk / HBD_I2cReadReg(0x0016);
    if(fifomode)
    {
        switch(gh30x_run_mode)
        {
            case RUN_MODE_ADT_HB_DET:
                ms = 1000 * (__HB_FIFO_THR_CNT_CONFIG__ * 1.05) / nSampleRate;
                break;
            case RUN_MODE_SPO2_DET:
                ms = 1000 * (__SPO2_FIFO_THR_CNT_CONFIG__ * 1.05) / nSampleRate;
                break;
            case RUN_MODE_HRV_DET:
                ms = 1000 * (__HRV_FIFO_THR_CNT_CONFIG__ * 1.05) / nSampleRate;
                break;
            default:
                ms = 1000;
                break;
        }
    }
    
    int nPeriod = nSampleRate / Hz;
    int i = 0;
    float fResult;
    int datenum = nSampleRate * ms / 1000;    
    if(0 != (nSampleRate * ms) % 1000)
    {
        ++datenum;
    }
    gsensor_soft_fifo_buffer_index = 0;
    for(;i<datenum;++i)
    {
        if(ntime >= nPeriod)
        {
            ntime = 0;
        }
        fResult = sinf(2 * 3.14 * Hz * ntime / nSampleRate);
        gsensor_soft_fifo_buffer[i].sXAxisVal = (GS16)(fResult * range * 512);
        gsensor_soft_fifo_buffer[i].sYAxisVal = gsensor_soft_fifo_buffer[i].sXAxisVal;
        gsensor_soft_fifo_buffer[i].sZAxisVal = gsensor_soft_fifo_buffer[i].sXAxisVal;
        ++gsensor_soft_fifo_buffer_index;
        ++ntime;
    }
}

/********END OF FILE********* Copyright (c) 2003 - 2020, Goodix Co., Ltd. ********/
