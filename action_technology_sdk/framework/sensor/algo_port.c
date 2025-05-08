/*******************************************************************************
 * @file    algo_port.c
 * @author  MEMS Application Team
 * @version V1.0
 * @date    2020-08-12
 * @brief   sensor testing
 *******************************************************************************/

/******************************************************************************/
// includes
/******************************************************************************/
#include <stdio.h>
#include <string.h>
#include <zephyr.h>
#include <soc.h>
#include <sys_manager.h>
#include <sensor_hal.h>
#include <sensor_algo.h>
#include <hr_algo.h>
#include <os_common_api.h>
#include <linker/linker-defs.h>
#ifdef CONFIG_SYS_WAKELOCK
#include <sys_wakelock.h>
#endif
#include "sensor_algo.h"
#include "sensor_port.h"
#include "sensor_sleep.h"
#include "algo_port.h"
#include "gps_manager.h"

#if CONFIG_AEM_WATCH_SUPPORT
#include "aem_adapter_sensor.h"
#endif

/******************************************************************************/
// declarations
/******************************************************************************/
extern int sensor_service_callback(int evt_id, sensor_res_t *res);

static void usr_evt_handler(sensor_evt_t *evt);
static void hr_wear_handler(uint8_t wearing_state, bool check);
static void hr_hb_handler(uint8_t hb_val, uint8_t hb_lvl_val, uint16_t rr_val);
static void hr_spo2_handler(uint8_t spo2_val, uint8_t spo2_lvl_val, uint8_t hb_val,
                            uint8_t hb_lvl_val, uint16_t rr_val[4], uint8_t rr_lvl_val, uint8_t rr_cnt, uint16_t spo2_r_val);
static void hr_hrv_handler(uint16_t *rr_val_arr, uint8_t rr_val_cnt, uint8_t rr_lvl);

/******************************************************************************/
// variables
/******************************************************************************/
const static sensor_os_api_t os_api =
    {
        .user_handler = usr_evt_handler,
};

const static hr_os_api_t hr_os_api =
    {
        .wear_handler = hr_wear_handler,
        .hb_handler = hr_hb_handler,
        .spo2_handler = hr_spo2_handler,
        .hrv_handler = hr_hrv_handler,
};
#ifdef CONFIG_GPS_MANAGER
static int _gps_res_callback(int evt_id, gps_res_t *res)
{
    SYS_LOG_INF("_gps_res_callback %d:%d:%d,fix_quality:%d,tracked:%d\n", res->gga_data.time.hours,
                res->gga_data.time.minutes, res->gga_data.time.seconds, res->gga_data.fix_quality, res->gga_data.satellites_tracked);

    sensor_raw_t sdata;
    memset(&sdata, 0, sizeof(sdata));
    sdata.id = IN_GNSS;

    if (res)
    {
        /*
        "double[0] = latitude (degrees)
double[1] = longitude (degrees)
double[2] = altitude (meters)
double[3] = bearing (heading in degrees)
double[4] = speed (m/s)
double[5] = Horizontal_Accuracy (meters)
double[6] = Vertical_Accuracy (meters)
double[7] = HDOP
double[8] = VDOP"
        */
        sdata.dData[0] = minmea_tofloat(&res->gga_data.latitude);
        sdata.dData[1] = minmea_tofloat(&res->gga_data.longitude);
        sdata.dData[2] = minmea_tofloat(&res->gga_data.altitude);

#ifdef CONFIG_GPS_PARSE_RMC_ENABLE
        sdata.dData[3] = minmea_tofloat(&res->rmc_data.course);
        sdata.dData[4] = minmea_tofloat(&res->rmc_data.speed);
#endif

        sdata.dData[7] = minmea_tofloat(&res->gga_data.hdop);
    }

    printk("IN_GNSS:%0.4f,%0.4f,%0.4f\n", sdata.dData[0], sdata.dData[1], sdata.dData[2]);
    sensor_algo_input(&sdata);
    // call algo
    sensor_algo_process(IN_GNSS);
    return 0;
}
#endif

/******************************************************************************/
// functions
/******************************************************************************/
__sleepfunc static void usr_evt_handler(sensor_evt_t *evt)
{
    sensor_res_t *res = sensor_algo_get_result();

    // dump event
    // sensor_algo_dump_event(evt);

    // process in sleep mode
    if (soc_in_sleep_mode())
    {
        switch (evt->id)
        {
        case ALGO_HANDUP:
            printk("handup=%d\r\n", res->handup);
            if (res->handup == 1)
            {
                sensor_sleep_wakeup(1);
            }
            break;

        case RAW_HEARTRATE:
            printk("hr=%d\r\n", (int)res->heart_rate);
            if (res->heart_rate > 0)
            {
                sensor_sleep_wakeup(0);
            }
            break;
        }
    }
    else
    {
        // process in system mode
        switch (evt->id)
        {
        case REQ_SENSOR:
            SYS_LOG_INF("REQ_SENSOR, id:%d,on:%d\n", (int)evt->fData[1], (int)evt->fData[2]);
            if ((int)evt->fData[2])
            {
                sensor_hal_enable((int)evt->fData[1]);
                if ((int)evt->fData[1] == IN_HEARTRATE)
                {
                    extern int sensor_get_hr_mode(void);
                    int mode = sensor_get_hr_mode();

                    sensor_res_t *res = sensor_algo_get_result();
                    if (res)
                    {
                        res->onbody = 0xff;
                    }

                    hr_algo_start(mode);
                }
                if ((int)evt->fData[1] == IN_GNSS)
                {
#ifdef CONFIG_GPS_MANAGER
                    gps_manager_enable(0, 0);
                    gps_manager_add_callback(_gps_res_callback);
#endif
                }
            }
            else
            {
                sensor_hal_disable((int)evt->fData[1]);
                if ((int)evt->fData[1] == IN_HEARTRATE)
                {
                    hr_algo_stop();
                }
                if ((int)evt->fData[1] == IN_GNSS)
                {
#ifdef CONFIG_GPS_MANAGER
                    gps_manager_disable(0, 0);
#endif
                }
            }
            break;

        case ALGO_ACTIVITY_OUTPUT:
            sensor_algo_dump_result(res);
            break;

        case RAW_HEARTRATE:
            SYS_LOG_INF("usr_evt_handler, hr=%d\n", (int)res->heart_rate);
            break;

        case ALGO_HANDUP:
            SYS_LOG_INF("handup=%d", res->handup);
#ifdef CONFIG_SYS_WAKELOCK
#if CONFIG_AEM_WATCH_SUPPORT
            adapter_sen_update_gesture_state(res->handup);
#else
            if (res->handup == 1)
            {

                sys_wake_lock(FULL_WAKE_LOCK);
                sys_wake_unlock(FULL_WAKE_LOCK);
            }
            else if (res->handup == 2)
            {
                system_request_fast_standby();
            }
#endif
#endif
            break;
        }

        // callback
        sensor_service_callback(evt->id, res);
    }
}

static void hr_wear_handler(uint8_t wearing_state, bool check)
{
    sensor_raw_t sdata;
    sensor_res_t *res = sensor_algo_get_result();

    SYS_LOG_INF("hr_wear_handler,wear:%d,pre:%d\n", wearing_state, res->onbody);
    if (res && check ? res->onbody != wearing_state : true)
    {
        // input wear data
        memset(&sdata, 0, sizeof(sdata));
        sdata.id = IN_OFFBODY;
        sdata.fData[0] = wearing_state;
        sensor_algo_input(&sdata);

        // call algo
        sensor_algo_process(sdata.id);
    }
}

static void hr_hb_handler(uint8_t hb_val, uint8_t hb_lvl_val, uint16_t rr_val)
{
    sensor_raw_t sdata;

    // input wear data
    memset(&sdata, 0, sizeof(sdata));
    sdata.id = IN_HEARTRATE;
    sdata.fData[0] = hb_val;
    sdata.fData[1] = (hb_lvl_val == 0) ? 3 : 0;
    sdata.fData[2] = 200;
    sdata.fData[3] = 40;
    sensor_algo_input(&sdata);

    // call algo
    sensor_algo_process(sdata.id);
}

static void hr_spo2_handler(uint8_t spo2_val, uint8_t spo2_lvl_val, uint8_t hb_val,
                            uint8_t hb_lvl_val, uint16_t rr_val[4], uint8_t rr_lvl_val, uint8_t rr_cnt, uint16_t spo2_r_val)
{
    SYS_LOG_INF("hr_spo2_handler,spo2:%d\n", spo2_val);

#if CONFIG_AEM_WATCH_SUPPORT
    adapter_sen_set_spo2(spo2_val);
#endif
}

static void hr_hrv_handler(uint16_t *rr_val_arr, uint8_t rr_val_cnt, uint8_t rr_lvl)
{
    SYS_LOG_INF("hr_hrv_handler,hrv:%d\n", rr_lvl);
#if CONFIG_AEM_WATCH_SUPPORT
    adapter_sen_set_hrv(rr_lvl);
#endif
}

void algo_init(void)
{
    SYS_LOG_INF("algo_init\n");
    // init algo
    sensor_algo_init(&os_api);

    // init fifo
    sensor_algo_input_fifo_init(20);

    // init heart-rate algo
    hr_algo_init(&hr_os_api);
}

__sleepfunc void algo_handler(int id, sensor_dat_t *dat)
{
    int idx;
    uint64_t ts, delta;
    float val[3];
    sensor_raw_t sdata;
    int ret;
#ifdef CONFIG_SENSOR_ALGO_MOTION_CYWEE_DML
    sensor_cvt_ag_t ag_cov = {0};
#endif

    // printk("sensor algo_handler id=%d, evt=%d\n", id, dat->evt);

    // input data
    switch (id)
    {
    case ID_ACC:
    case ID_MAG:
        if (dat)
        {
            // start fifo
            if (dat->cnt > 1)
            {
                ts = dat->ts * 1000000ull;
#ifdef CONFIG_SENSOR_ALGO_MOTION_CYWEE_DML
                delta = sensor_algo_get_data_period(id) * 1000000ull;
#else
                delta = dat->pd * 1000000ull;
#endif
                sensor_algo_input_fifo_start(id, ts, delta);
            }
            // input data
            for (idx = 0; idx < dat->cnt; idx++)
            {
                // get value
#ifdef CONFIG_SENSOR_ALGO_MOTION_CYWEE_DML
                sensor_hal_cvt_data(id, dat->buf + dat->sz * idx, dat->sz, NULL, ag_cov.acc_raw);
                sensor_algo_convert_data(&ag_cov);
                memcpy(val, ag_cov.acc_data, sizeof(float) * 3);
#else
                sensor_hal_get_value(id, dat, idx, val);
#endif
                // printk("%s %.2f %.2f %.2f\n", sensor_hal_get_name(id), val[0], val[1], val[2]);

                // input acc data
                memset(&sdata, 0, sizeof(sensor_raw_t));
                sdata.id = id;
                memcpy(sdata.fData, val, sizeof(float) * 3);
                sensor_algo_input(&sdata);
            }
            // end fifo
            if (dat->cnt > 1)
            {
                sensor_algo_input_fifo_end(id);
            }
        }
        // call algo
        sensor_algo_process(id);
        break;

    case ID_BARO:
        if (dat)
        {
            if (dat->cnt == 0)
            {
                ret = sensor_hal_poll_data(id, dat, NULL);
                if (ret)
                {
                    printk("sensor %d poll failed!\n", id);
                    break;
                }
            }
            // get value
            sensor_hal_get_value(id, dat, 0, val);
            // printk("%s %0.2f %0.2f\n", sensor_hal_get_name(id), val[0], val[1]);

            // input baro data
            memset(&sdata, 0, sizeof(sdata));
            sdata.id = IN_BARO;
            sdata.fData[0] = val[0];
            sdata.fData[1] = 1.0f;
            sdata.fData[2] = val[1];
            sensor_algo_input(&sdata);
        }
        // call algo
        sensor_algo_process(id);
        break;

    case ID_HR:
        // call algo
        hr_algo_process();
        break;
    }
}
