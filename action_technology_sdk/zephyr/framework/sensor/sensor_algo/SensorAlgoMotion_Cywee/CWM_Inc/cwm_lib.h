#ifndef __CWM_LIB_H__
#define __CWM_LIB_H__

#include <stdint.h>
#include "CWM_EventStructure.h"
#include "OsApi.h"
#include "CWM_CustomDataStruct.h"

/**
  * @brief Setting Control control ID
  * @api CWM_SettingControl
  * @param CtrId
  */
#define SCL_LOG                 0
#define SCL_USER_INFO           1
#define SCL_DATE_TIME           2
#define SCL_SEDENTARY           3
#define SCL_PEDO_RESET          4 // old
#define SCL_RESET_DAILY_INFO    4 // new
#define SCL_REQ_SLEEPING_DATA   5
#define SCL_LOW_POWER_MODE      6
#define SCL_SET_ACTIVITY_MODE   7
#define SCL_GET_CHIP_INFO       8
#define SCL_ACTIVITY_CONFIG     9
#define SCL_LIB_DEBUG           10
#define SCL_PEDO_CONFIG         11
#define SCL_GET_LIB_INFO        12
#define SCL_SWIM_CONFIG         13
#define SCL_BIKING_CONFIG       14
#define SCL_ACTIVITY_PAUSE      15
#define SCL_HAND_UPDOWN_CONFIG  16
#define SCL_CHIP_VENDOR_CONFIG  17
#define SCL_SLEEP_CONFIG        18
#define SCL_ABS_STATIC_CONFIG   19
#define SCL_HEART_RATE_CONFIG   20
#define SCL_REQ_SWIM_EXIT       21
#define SCL_WATCH_FALL_CONFIG   22
#define SCL_AR_ALERT_CONFIG     23
#define SCL_WM_CONFIG           25
#define SCL_INACTIVITY_CONFIG   26
#define SCL_SET_INACTIVITY_MODE 27
#define SCL_STAND_CONFIG        28
#define SCL_SS_CONFIG           29
#define SCL_ACT_INFO_CONFIG     30
#define SCL_REQ_ACTIVITY_EXIT   31
#define SCL_ACT_PAI_INFO        33
#define SCL_ACT_INTSY_INFO      35
#define SCL_DUMP_WI_INFO        36
#define SCL_ACT_ZONE_TIME       37
#define SCL_ACT_TRAINING_INFO   38
#define SCL_WATCH_INTF_RESET    40

#define SCL_SZ_BREACH_CONFIG    50

#define SCL_ALGO_PROC_CONFIG    100
#define SCL_INPUT_SENSOR_CONFIG 101
#define SCL_INPUT_DT_CONFIG     102
#define SCL_SENS_RANGE_DETECT_CONFIG     103
#define SCL_SENS_CALI_CONFIG    110
#define SCL_SENS_CALI_CTRL_MAG  111
#define SCL_SENS_CALI_CTRL_A    112
#define SCL_SPV_CONFIG          113
#define SCL_SPV_MODE            114
#define SCL_SENS_CALI_SET_MAG   115
#define SCL_SENS_CALI_SET_A     116


/**
  * @brief Custom Sensor Input sensorId
  * @api CWM_CustomSensorInput
  * @param CustomSensorData.sensorType extSensorId
  * @
  * @api CWM_CustomSensorInput_Fifo_Start CWM_CustomSensorInput_Fifo_End
  * @param extSensorId
  */
#define CUSTOM_ACC              0
#define CUSTOM_GYRO             1
#define CUSTOM_MAG              2
#define CUSTOM_BARO             3
#define CUSTOM_TEMP             4
#define CUSTOM_HEARTRATE        5
#define CUSTOM_GNSS             6
#define CUSTOM_OFFBODY_DETECT   7
#define CUSTOM_ON_CHARGING      8
#define CUSTOM_ACC_ANY_MOTION   9
#define CUSTOM_SENS1            10


/**
  * @brief sensor enable/disable
  * @api CWM_Sensor_Enable CWM_Sensor_Disable
  * @param sensorId
  */
#define IDX_ACCEL                    0
#define IDX_GYRO                     1
#define IDX_MAG                      2
#define IDX_BARO                     3
#define IDX_TEMP                     4
#define IDX_HEARTRATE                5
#define IDX_GNSS                     6
#define IDX_OFFBODY_DETECT           7
#define IDX_ALGO_SEDENTARY           8
#define IDX_ALGO_WATCH_HANDUP        9
#define IDX_ALGO_SLEEP               10
#define IDX_REQUEST_SENSOR           11
#define IDX_ALGO_SHAKE               13
#define IDX_ALGO_ACTIVITY_OUTPUT     14
#define IDX_ALGO_ANY_MOTION          15
#define IDX_ALGO_NO_MOTION           16
#define IDX_ALGO_FALLING             17
#define IDX_ONCHARGING_DETECT        18
#define IDX_ALGO_ABSOLUTE_STATIC     19
#define IDX_ALGO_SPV                 20
#define IDX_ALGO_SENS_CALIBRATION    21
#define IDX_ALGO_INACTIVITY_OUTPUT   22
#define IDX_ALGO_STAND               23
#define IDX_ALGO_SENS_RANGE_DETECT   24

/**
  * @brief activity mode setting
  * @api CWM_SettingControl
  * @param SCL_SET_ACTIVITY_MODE
  */
#define ACTIVITY_MODE_NORMAL                1001

typedef struct {
    int iData[16];
} SettingControl_t;

typedef void (*FN_SENSOR_LISTEN)(pSensorEVT_t);


#define LOS_DATA_TYPE_INT               0
#define LOS_DATA_TYPE_STR               1

/**
  * @brief 
  * @param dataType: cmd data type indicater
  *                  0 (LOS_DATA_TYPE_INT): int type data, dataInt accessible
  *                  1 (LOS_DATA_TYPE_STR): string type data, *dataStr and dataStr_length accessible
  */
typedef struct {
    const char  *optionString;
    const char  *sectionName;
    const char  *apiName;
    int         cmdId;
    int         dataType;
    int         dataInt[16];
    const char  *dataStr;
    int         dataStr_length;
} los_cmd_data_t;

typedef void (*FN_LOS_CMD_EXECUTE)(los_cmd_data_t *);

/**
  * @brief  CyweeMotion internal process Initial
  */
void CWM_LibPreInit(OsAPI *api);

/**
  * @brief  Sensor Event register callback, when sensor event is triggered, 
  *         this function will be called to notify
  */
void CWM_LibPostInit(FN_SENSOR_LISTEN pfn);

/**
  * @brief  lib deinit for special case. Not any situation can be used normally
  */
void CWM_LibDeinit(void);

/**
  * @brief  Sensor enable based on sensor index provided
  * @param  sensorId: sensor register handle, if not setup this handle don't care
  * @retval 0 is success, otherwise is failed
  */
int CWM_Sensor_Enable(uint32_t sensorId);

/**
  * @brief  Sensor disable based on sensor index provided
  * @param  sensorId: sensor register handle, if not setup this handle don't care
  * @retval 0 is success, otherwise is failed
  */
int CWM_Sensor_Disable(uint32_t sensorId);

/**
  * @brief  Read sensor data and output through sensor call-back function
  * @retval 0 is no data to process, mcu can enter low power mode,
            otherwise has some data needs process this function again.
  */
int CWM_process(void);

/**
  * @brief  Read sensor data and output through sensor call-back function
  */
int CWM_process2(int dt_us);

/**
  * @brief  Get how many event in CyweeMotion Library to be processed
  * @retval 0 is no data to process, mcu can enter low power mode,
            otherwise has some data
  */
int CWM_IsReadyForProcess(void);

/**
  * @brief  Put custom sensor data.
  * @param  SensorDataInput: sensor data input handle, input format as below:
  *                          sensorType= CustomSensorDataIndex
  *                          fData     = sensor raw data
  * @retval 0 is success, otherwise is failed
  */
int CWM_CustomSensorInput(CustomSensorData *SensorDataInput);

/**
  * @brief Get the duration time of the next event
  * @retval the duration time (unit nanosecond)
  */
int64_t CWM_GetNextActionDuration_ns(void);

/**
  * @brief  set lib config
  * @param  Control ID
  * @param  Setting control array
  * @retval 0 is success, otherwise is failed
  */
int CWM_SettingControl(int CtrId, SettingControl_t *pData);

/**
  * @brief Fifo initial
  * @param max recored number of acc fifo
  */
void CWM_CustomSensorInput_Fifo_Init(int maxRecords_Acc);

/**
  * @brief  setting lib can start input fifo
  * @param  sensor ID
  * @param  timestamp that interrupt trigger to read hw fifo data
  * @param  fifo delta_time
  * @retval 0 is success, otherwise is failed
  */
int CWM_CustomSensorInput_Fifo_Start(uint32_t extSensorId, uint64_t triggerFifoRead_timeStamp_ns, uint64_t dt_ns);

/**
  * @brief  setting lib to stop input fifo
  * @param  sensor ID
  */
void CWM_CustomSensorInput_Fifo_End(uint32_t extSensorId);

/**
  * @brief  Execute option string
  * @param  optionString
  * @param  sectionString section execute order 
  * @param  custCallbackFunc custom option string execute function
  * @param  skipExeDefault don't execute default command
  */
void CWM_LoadOptionString(const char *optionString, const char *sectionString, FN_LOS_CMD_EXECUTE custCallbackFunc, int skipExeDefault);

void cm__as1__init(void);
void cm__as1__deinit(void);	
#endif
