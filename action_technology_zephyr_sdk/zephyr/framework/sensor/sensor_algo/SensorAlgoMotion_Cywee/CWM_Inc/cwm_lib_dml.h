#ifndef __CWM_LIB_DML_H__
#define __CWM_LIB_DML_H__

#define SCL_DML_DRV_ENABLE              400
#define SCL_DML_DRV_INIT                401
#define SCL_DML_DRV_HW_CONFIG           402
#define SCL_DML_DRV_AG_CONFIG           403
#define SCL_DML_DRV_M_CONFIG            404
#define SCL_DML_DEBUG                   405
#define SCL_DML_GET_LIB_INFO            406
#define SCL_DML_GET_INITED_LIST         407
#define SCL_DML_DRV_AG_STATUS           408
#define SCL_DML_DRV_ANYM_CONFIG         409
#define SCL_DML_DRV_AG_PERF_CONFIG      410
#define SCL_DML_DRV_M_PERF_CONFIG       411
#define SCL_DML_DRV_AG_EXT_CONFIG       412
#define SCL_DML_DRV_M_EXT_CONFIG        413
#define SCL_DML_DRV_AG_REG_ACCESS       414
#define SCL_DML_DRV_M_REG_ACCESS        415
#define SCL_DML_DRV_AG_ODR_CALI         416

#define SCL_DML_AUTO_RANGE_CONFIG       420

int CWM_Dml_LibInit(void);
int CWM_Dml_process(void);


#define DML_SENSOR_DATA_VALID_DT         (1 << 0)
#define DML_SENSOR_DATA_VALID_ACC        (1 << 1)
#define DML_SENSOR_DATA_VALID_GYRO       (1 << 2)
#define DML_SENSOR_DATA_VALID_MAG        (1 << 3)
#define DML_SENSOR_DATA_VALID_ACC_TEMP   (1 << 4)

typedef struct {
    int     hwId;
    int     auxId;
    uint64_t ts_ns;

    int     validData;

    int dt_us;

    float   acc_ratio;      //acc_data = acc_org * acc_ratio
    int16_t acc_org[3];     //sensor hw original output data (axis adjusted)
    float   acc_data[3];    //unit: m/s^2 (axis adjusted)

    float   acc_temp;       //unit: celsius
    
    float   gyro_ratio;     //gyro_data = gyro_org * gyro_ratio
    int16_t gyro_org[3];    //sensor hw original output data (axis adjusted)
    float   gyro_data[3];   //unit: rad/s (axis adjusted)
    
    float   mag_data[3];    //unit: uT (axis adjusted)
} dml_sensorData_t;

typedef void (*FN_SENSOR_DATA_OUTPUT)(const dml_sensorData_t *);

int CWM_Dml_sensorDataOutput_register(FN_SENSOR_DATA_OUTPUT sdo_callBack_func);
void CWM_Dml_sensorDataOutput_logConfig(void);
void CWM_Dml_sensorDataOutput_logOutput(const dml_sensorData_t *pSd);


typedef struct {
    int disable_unitConv;      // [input]  0: [acc_data] and [gyro_data] have output
                               //          1: [acc_data] and [gyro_data] no output

    int16_t acc_raw[3];        // [input]  sensor hw original output data (axis un-adjusted)
    int16_t gyro_raw[3];       // [input]  sensor hw original output data (axis un-adjusted)

    int16_t acc_org[3];        // [output] sensor hw original output data (axis    adjusted)
    int16_t gyro_org[3];       // [output] sensor hw original output data (axis    adjusted)

    float   acc_data[3];       // [output] unitConv to unit: m/s^2 (axis adjusted)
    float   gyro_data[3];      // [output] unitConv to unit: rad/s (axis adjusted)
} dml_cust_data_ag_t;

void CWM_Dml_custData_converter_ag(dml_cust_data_ag_t *data);

#endif
