/*******************************************************************************
 * @file    sensor_algo.h
 * @author  MEMS Application Team
 * @version V1.0
 * @date    2021-5-25
 * @brief   sensor algorithm api
*******************************************************************************/

#ifndef _SENSOR_ALGO_H
#define _SENSOR_ALGO_H

/******************************************************************************/
//includes
/******************************************************************************/
#include <stdint.h>
#include <stdarg.h>

/******************************************************************************/
//constants
/******************************************************************************/

/* Input sensor id */
typedef enum {
	IN_ACC = 0,
	IN_GYRO,
	IN_MAG,
	IN_BARO,
	IN_TEMP,
	IN_HEARTRATE,
	IN_GNSS,
	IN_OFFBODY,
} sensor_in_e;

/* Output sensor id */
typedef enum {
	RAW_ACCEL = 0,
	RAW_GYRO,
	RAW_MAG,
	RAW_BARO,
	RAW_TEMP,
	RAW_HEARTRATE,
	RAW_GNSS,
	RAW_OFFBODY,
	ALGO_SEDENTARY,
	ALGO_HANDUP,
	ALGO_SLEEP,
	REQ_SENSOR,
	ALGO_ACTTYPEDETECT,
	ALGO_SHAKE,
	ALGO_ACTIVITY_OUTPUT,
	ALGO_ANY_MOTION,
	ALGO_NO_MOTION,
	ALGO_FALLING,
	ALGO_ORIENTATION,
} sensor_out_e;

/* Activity mode */
typedef enum {
	NORMAL = 1001,				// acc + baro
	TREADMILL = 1002,			// acc + hr
	OUTDOOR_RUNNING = 1003,	// acc + hr + gnss
	OUTDOOR_BIKING = 3001,		// acc + baro + hr +gnss
	MAIN_EXTRA = 100,				// main extra info
	BIKING_EXTRA = 120,				// biking extra info
} activity_mode_e;

/* Control id */
typedef enum {
	SCL_LOG = 0,
	SCL_USER_INFO = 1,
	SCL_DATE_TIME = 2,
	SCL_SEDENTARY = 3,
	SCL_PEDO_RESET = 4,
	SCL_REQ_SLEEPING_DATA = 5,
	SCL_LOW_POWER_MODE = 6,
	SCL_SET_ACTIVITY_MODE = 7,
	SCL_GET_CHIP_INFO = 8,
	SCL_ACTIVITY_CONFIG = 9,
	SCL_LIB_DEBUG = 10,
	SCL_PEDO_CONFIG = 11,
	SCL_GET_LIB_INFO = 12,
	SCL_SWIM_CONFIG = 13,
	SCL_BIKING_CONFIG = 14,
	SCL_ACTIVITY_PAUSE = 15,
	SCL_HAND_UPDOWN_CONFIG = 16,
	SCL_CHIP_VENDOR_CONFIG = 17,
	SCL_SLEEP_CONFIG = 18,
	SCL_ALGO_PROC_CONFIG = 100,
	SCL_INPUT_SENSOR_CONFIG = 101,
} sensor_ctl_e;

/******************************************************************************/
//typedef
/******************************************************************************/
#ifdef __CC_ARM                         /* ARM Compiler */
#pragma anon_unions
#endif

/* Input sensor data */
typedef struct {
	uint32_t id;
	union {
		float fData[16];
		double dData[10];
	};
} sensor_raw_t;

/* Ouput sensor event */
typedef struct {
	uint32_t id;
	uint32_t index;
	union {
		float fData[16];
		double dData[8];
		int iData[16];
	};
	uint64_t timestamp_ns;
} sensor_evt_t;

/* Sensor control data */
typedef struct {
	union {
		int iData[16];
		float fData[16];
		double dData[8];
	};
} sensor_ctl_t;

/* Sensor date time */
typedef struct {
	uint8_t month;
	uint8_t day;
	uint8_t hour;
	uint8_t minute;
} sensor_datetime_t;

/* Sensor algorithm result */
typedef struct {
	/* orientation values are in degrees */
	float orientation;
	
	/* pressure in hectopascal (hPa) */
	float pressure;
	
	/* altitude in meter (m) */
	float altitude;
	
	/* temperature is in degrees centigrade (Celsius) */
	float temperature;
	
	/* heart reate in ppm */
	float heart_rate;
	
	/* gnss data */
	struct gnss_s {
		float latitude;		// degrees
		float longitude;	// degrees
		float altitude;		// meters
		float bearing;		// heading in degrees
		float speed;		// m/s
	} gnss;
	
	/* on-body/off-body status */
	uint32_t onbody; 		//1:on-body 0:off-body
	
	/* sedentary status */
	uint32_t sedentary; 	//1:enter 2:exit 3:remider
	
	/* handup status */
	uint32_t handup; 		//1:hand-up 2:hand-down
	
	/* sleeping data */
	struct sleeping_s {
		// status 0:enter 1:shallow 2:deep 3:awake 4:exit 12:rapid eye movement
		uint32_t status;
		// time [month-day-hour-minute]
		sensor_datetime_t time_enter;
		sensor_datetime_t time_shallow;
		sensor_datetime_t time_deep;
		sensor_datetime_t time_awake;
		sensor_datetime_t time_rem;
		sensor_datetime_t time_exit;
		// duration in minute
		uint32_t duration_total;
		uint32_t duration_shallow;
		uint32_t duration_deep;
		uint32_t duration_awake;
		uint32_t duration_rem;
	} sleeping;
	
	/* activity mode */
	uint32_t activity_mode;
	
	/* activity detect type */
	uint32_t activity_detect;
	
	/* pedometer data */
	struct pedometer_s {
		// 1:static 2:walk 4:upstairs 5:downstairs 6:run 7:bike 8:vehicle
		uint32_t activity_type;
		float total_steps;		// steps
		float total_distance;	// meters
		float total_calories;	// Kcal
		float cur_step_freq;	// steps/minute
		float avg_step_freq;	// steps/minute
		float max_step_freq;	// steps/minute
		float cur_step_len;		// meters
		float avg_step_len;		// meters
		float cur_pace;			// minutes/Km
		float avg_pace;			// minutes/Km
		float max_pace;			// minutes/Km
		float slope;			// percentage
		float elevation;		// meters
		float elevation_up;		// meters
		float elevation_down;	// meters
		float floors_up;		// floors
		float floors_down;		// floors
		float vo2max;			// ml/kg/min
		float hr_intensity;		// 
		float hr_zone;			//
	} pedometer;

	/* biking data */
	struct biking_s {
		float latitude;		// degrees
		float longitude;	// degrees
		float distance;		// meters
		float calories;		// Kcal
		float cur_speed;	// km/hour
		float avg_speed;	// km/hour
		float max_speed;	// km/hour
		float slope;		// percentage
		float elevation;	// meters
		float vo2max;		// ml/kg/min
		float hr_intensity;	// 
		float hr_zone;		//
	} biking;
} sensor_res_t;

/* Sensor event handler */
typedef void (*sensor_handler_t)(sensor_evt_t *evt);

/* Sensor os api */
typedef struct {
	/* Printf */
  int (*dbgOutput)(const char *fmt, ...);
	
	/* Get current timestamp */
  uint64_t (*get_timestamp_ns)(void);
	
	/* User handler */
	void (*user_handler)(sensor_evt_t *evt);
	
	/* I2C write func */
	int (*i2c_write)(uint8_t addr, uint16_t reg, uint8_t *buf, uint16_t len);
	
	/* I2C read func */
	int (*i2c_read)(uint8_t addr, uint16_t reg, uint8_t *buf, uint16_t len);
	
} sensor_os_api_t;

/* Sensor algo api */
typedef struct {
	/* Init sensor algo */
	int (*init)(const sensor_os_api_t *api);
	
	/* Get os api */
	sensor_os_api_t* (*get_os_api)(void);
	
	/* Set os api */
	int (*set_os_api)(const sensor_os_api_t *api);
	
	/* Control sensor algo */
	int (*control)(uint32_t ctl_id, sensor_ctl_t *ctl);
	
	/* Enable sensor id */
	int (*enable)(uint32_t id, uint32_t func);
	
	/* Disable sensor id */
	int (*disable)(uint32_t id, uint32_t func);
	
	/* Input sensor raw data */
	void (*input)(sensor_raw_t* dat);
	
	/* Input sensor fifo init */
	void (*input_fifo_init)(int num);
	
	/* Input sensor fifo start */
	int (*input_fifo_start)(uint32_t id, uint64_t timestamp, uint64_t delta);
	
	/* Input sensor fifo end */
	void (*input_fifo_end)(uint32_t id);
	
	/* Get the duration time of the next event */
	int64_t (*get_next_duration)(void);
	
	/* Read sensor data and output through sensor call-back function */
	int (*process)(uint32_t id);
	
	/* Read sensor algorithm result */
	sensor_res_t* (*get_result)(void);
	
	/* Dump sensor algorithum event */
	void (*dump_event)(sensor_evt_t *evt);
	
	/* Dump sensor algorithum result */
	void (*dump_result)(sensor_res_t *res);
	
} sensor_algo_api_t;


/******************************************************************************/
//function
/******************************************************************************/

/* Init sensor algo */
extern int sensor_algo_init(const sensor_os_api_t *api);

/* Get os api */
extern sensor_os_api_t* sensor_algo_get_os_api(void);

/* Set os api */
extern int sensor_algo_set_os_api(const sensor_os_api_t *api);

/* Control sensor algo */
extern int sensor_algo_control(uint32_t ctl_id, sensor_ctl_t *ctl);

/* Enable sensor id */
extern int sensor_algo_enable(uint32_t id, uint32_t func);

/* Disable sensor id */
extern int sensor_algo_disable(uint32_t id, uint32_t func);

/* Input sensor raw data */
extern void sensor_algo_input(sensor_raw_t* dat);

/* Input sensor fifo init */
extern void sensor_algo_input_fifo_init(int num);

/* Input sensor fifo start */
extern int sensor_algo_input_fifo_start(uint32_t id, uint64_t timestamp, uint64_t delta);

/* Input sensor fifo end */
extern void sensor_algo_input_fifo_end(uint32_t id);

/* Get the duration time of the next event */
extern int64_t sensor_algo_get_next_duration(void);

/* Read sensor data and output through sensor call-back function */
extern int sensor_algo_process(uint32_t id);

/* Read sensor algorithm result */
extern sensor_res_t* sensor_algo_get_result(void);

/* Dump sensor algorithum event */
extern void sensor_algo_dump_event(sensor_evt_t *evt);

/* Dump sensor algorithum result */
extern void sensor_algo_dump_result(sensor_res_t *res);

/******************************************************************************/
//api
/******************************************************************************/

/* sensor rom variable */
extern char __sensor_algo_start[];
extern char __sensor_algo_end[];
#define __sensor_algo_size	(__sensor_algo_end - __sensor_algo_start)

extern char __sensor_algo_sleep_start[];
extern char __sensor_algo_sleep_end[];
#define __sensor_algo_sleep_size	(__sensor_algo_sleep_end - __sensor_algo_sleep_start)

/* sensor algo api */
#define p_sensor_algo_api	((const sensor_algo_api_t*)__sensor_algo_start)
#define p_sensor_algo_sleep_api	((const sensor_algo_api_t*)&__kernel_ram_start)

#endif  /* _SENSOR_ALGO_H */

