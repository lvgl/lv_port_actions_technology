/*******************************************************************************
 * @file    sensor_algo.c
 * @author  MEMS Application Team
 * @version V1.0
 * @date    2021-5-25
 * @brief   sensor algorithm api
*******************************************************************************/

/******************************************************************************/
//includes
/******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sensor_algo.h>
#include <cwm_lib.h>
#include <cwm_lib_dml.h>
#include <sys/sys_heap.h>

/******************************************************************************/
//constants
/******************************************************************************/
#define DBG(...) printk(__VA_ARGS__)
//#define DBG(...)			do {} while (0)

#define PI (3.14159265f)

/* sensor acc read/write */
static int sensor_acc_read(uint16_t slaveAddr, uint16_t reg, int regLength, uint8_t *readData,
			   int readDataSize, int busIndex);
static int sensor_acc_write(uint16_t slaveAddr, uint16_t reg, int regLength, uint8_t *writeData,
			    int writeDataSize, int busIndex);

/******************************************************************************/
//variables
/******************************************************************************/

/* Sensor algorithm result */
static sensor_res_t sensor_algo_res = { 0 };

/* event handler */
static sensor_handler_t user_handler = NULL;

static int hr_mode = 0;

#ifdef CONFIG_SENSOR_ALGO_MOTION_CYWEE_DML
/* sensor odr */
static int sensor_period_ms = 20; //50hz -> 20ms
#endif

/* CWM os api */
const static os_api cwm_os_api = {
	.malloc = sensor_malloc,
	.free = sensor_free,
	.GetTimeNs = sensor_get_timestamp_ns,
	.uSleep = sensor_udelay,
	.dbgOutput = (int (*)(const char *, ...))printk,
	.i2cRead = sensor_acc_read,
	.i2cWrite = sensor_acc_write,
};

/******************************************************************************/
//functions
/******************************************************************************/
static int sensor_acc_read(uint16_t slaveAddr, uint16_t reg, int regLength, uint8_t *readData,
			   int readDataSize, int busIndex)
{
	return sensor_hal_read(ID_ACC, reg, readData, readDataSize);
}

static int sensor_acc_write(uint16_t slaveAddr, uint16_t reg, int regLength, uint8_t *writeData,
			    int writeDataSize, int busIndex)
{
	return sensor_hal_write(ID_ACC, reg, writeData, writeDataSize);
}

static void sensor_inactivity_data_save(sensor_evt_t *evt)
{
	sensor_res_t *res = &sensor_algo_res;
	uint32_t activity_mode = (uint32_t)evt->fData[0];
	res->activity_mode = (uint32_t)evt->fData[0];
	DBG("ALGO_INACTIVITY_OUTPUT:%d\n", activity_mode);
	DBG("ALGO_INACTIVITY_OUTPUT data:%d %d %d %d %d %d %d %d \n", (int)evt->fData[0],
	    (int)evt->fData[1], (int)evt->fData[2], (int)evt->fData[3], (int)evt->fData[8],
	    (int)evt->fData[9], (int)evt->fData[10], (int)evt->fData[15]);

	if (activity_mode == ACTIVITY_OUT_SEDENTARY) {
		struct sedentary_s *sed = &res->sedentary;
		sed->type = (uint8_t)evt->fData[2];
		sed->stand_count = (uint8_t)evt->fData[3];
	}
	if (activity_mode == ACTIVITY_OUT_SLEEPING) {
		struct sleeping_s *sleeping = &res->sleeping;
		sensor_datetime_t *dt = NULL;
		sleeping->status = (uint32_t)evt->fData[2];
		switch (sleeping->status) {
		case 0:
			dt = &sleeping->time_enter;
			break;
		case 1:
			dt = &sleeping->time_shallow;
			break;
		case 2:
			dt = &sleeping->time_deep;
			break;
		case 3:
			dt = &sleeping->time_awake;
			break;
		case 4:
			dt = &sleeping->time_exit;
			break;
		case 12:
			dt = &sleeping->time_rem;
			break;
		}
		if (dt) {
			dt->month = (uint8_t)evt->fData[3];
			dt->day = (uint8_t)evt->fData[4];
			dt->hour = (uint8_t)evt->fData[5];
			dt->minute = (uint8_t)evt->fData[6];
		}
	}
	if (activity_mode == ACTIVITY_OUT_SLEEPING_EXT) {
		struct sleeping_s *sleeping = &res->sleeping;
		sleeping->duration_total = (uint32_t)evt->fData[1];
		sleeping->duration_shallow = (uint32_t)evt->fData[2];
		sleeping->duration_deep = (uint32_t)evt->fData[3];
		sleeping->duration_awake = (uint32_t)evt->fData[4];
		sleeping->duration_rem = (uint32_t)evt->fData[5];
	}
	if (activity_mode == ACTIVITY_OUT_NAP) {
	}
}

static void sensor_activity_data_save(sensor_evt_t *evt)
{
	sensor_res_t *res = &sensor_algo_res;
	struct pedometer_s *pedometer = &res->pedometer;
	struct biking_s *biking = &res->biking;
	struct workout_machine_s *machine = &res->machine;
	struct swimming_s *swim = &res->swimming;

	uint32_t activity_mode = (uint32_t)evt->fData[0];
	res->activity_mode = (uint32_t)evt->fData[0];
	DBG("ALGO_ACTIVITY_OUTPUT:%d\n", activity_mode);

	// pedometer main&extra
	if (activity_mode >= ACTIVITY_NORMAL &&
	    activity_mode <= ACTIVITY_OUTDOOR_RUNNING_TRACKING) {
		pedometer->activity_type = (uint32_t)evt->fData[4];
		pedometer->total_steps = evt->fData[1];
		pedometer->total_distance = evt->fData[2];
		pedometer->total_calories = evt->fData[3];
		pedometer->cur_step_freq = evt->fData[5];
		pedometer->cur_step_len = evt->fData[6];
		pedometer->cur_pace = evt->fData[7];
		pedometer->slope = evt->fData[8];
		pedometer->elevation = evt->fData[9];
		pedometer->floors_up = evt->fData[10];
		pedometer->floors_down = evt->fData[11];
		pedometer->elevation_up = evt->fData[12];
		pedometer->elevation_down = evt->fData[13];
	}
	if (activity_mode == MAIN_EXTRA) {
		pedometer->avg_step_freq = evt->fData[1];
		pedometer->avg_step_len = evt->fData[2];
		pedometer->avg_pace = evt->fData[3];
		pedometer->max_step_freq = evt->fData[4];
		pedometer->max_pace = evt->fData[5];
		pedometer->vo2max = evt->fData[6];
		pedometer->hr_intensity = evt->fData[7];
		pedometer->hr_zone = evt->fData[8];
	}

	// swim_pool
	if (activity_mode == ACTIVITY_INDOOR_SWIMMING) {
		swim->strokes = evt->fData[1];
		swim->laps = evt->fData[3];
		swim->swim_time = evt->fData[4];
		swim->stroke_freq = evt->fData[5];
		swim->is_pause = evt->fData[10];

		swim->total_calories = evt->fData[6];
		swim->total_distance = evt->fData[9];
		DBG("ACTIVITY_INDOOR_SWIMMING:%d,%d\n", (uint32_t)swim->total_distance,
		    (uint32_t)swim->total_calories);
	}
	if (activity_mode == SWIM_POOL_LAP_INFO) {
	}
	if (activity_mode == SWIM_POOL_TOTAL_INFO) {
	}

	// swim_openwater
	if (activity_mode == ACTIVITY_OPEN_WATER_SWIMMING) {
		swim->strokes = evt->fData[1];
		swim->laps = evt->fData[3];
		swim->swim_time = evt->fData[4];
		swim->stroke_freq = evt->fData[5];
		swim->is_pause = evt->fData[10];

		swim->total_calories = evt->fData[6];
		swim->total_distance = evt->fData[9];
		DBG("ACTIVITY_OUTDOOR_SWIMMING:%d,%d\n", (uint32_t)swim->total_distance,
		    (uint32_t)swim->total_calories);
	}
	if (activity_mode == SWIM_OPEN_WATER_SEGMENT_INFO) {
	}
	if (activity_mode == SWIM_OPEN_WATER_TOTAL_INFO) {
	}

	// biking
	if (activity_mode == ACTIVITY_OUTDOOR_BIKING) {
		biking->latitude = evt->fData[1];
		biking->longitude = evt->fData[3];
		biking->distance = evt->fData[5];
		biking->cur_speed = evt->fData[6];
		biking->elevation = evt->fData[7];
		biking->slope = evt->fData[8];
		biking->calories = evt->fData[9];
		biking->vo2max = evt->fData[10];
		biking->hr_intensity = evt->fData[11];
		biking->hr_zone = evt->fData[12];
	}
	if (activity_mode == BIKING_EXTRA) {
		biking->avg_speed = evt->fData[1];
		biking->max_speed = evt->fData[2];
	}

	// free training
	if (activity_mode == ACTIVITY_FREE_TRAINING) {
	}

	// workout machine
	if (activity_mode == ACTIVITY_WORKOUT_MACHINE) {
		machine->counts = evt->fData[1];
		machine->calories = evt->fData[2];
		machine->freq = evt->fData[3];
		machine->avg_freq = evt->fData[4];
	}

	// AR Alert
	if (activity_mode == AR_ALERT) {
		res->activity_detect = (uint32_t)evt->fData[1];
		DBG("activity_detect:%d\n", res->activity_detect);
	}
}
/* sensor algo save result */
static void sensor_algo_save(sensor_evt_t *evt)
{
	sensor_res_t *res = &sensor_algo_res;
	struct gnss_s *gnss = &res->gnss;

	/* save result */
	switch (evt->id) {
	case RAW_MAG:
		res->orientation = atan2f(evt->fData[0], evt->fData[1]) * (180 / PI) + 180;
		break;

	case RAW_BARO:
		res->pressure = evt->fData[0];
		break;

	case RAW_TEMP:
		res->temperature = evt->fData[0];
		break;

	case RAW_HEARTRATE:
		res->heart_rate = evt->fData[0];
		break;

	case RAW_GNSS:
		gnss->latitude = evt->fData[1];
		gnss->longitude = evt->fData[3];
		gnss->altitude = evt->fData[5];
		gnss->bearing = evt->fData[6];
		gnss->speed = evt->fData[7];
		break;

	case RAW_OFFBODY:
		res->onbody = (uint32_t)evt->fData[0];
		break;

	case ALGO_SEDENTARY:
		res->sedentary.type = (uint8_t)evt->fData[2];
		res->sedentary.stand_count = (uint8_t)evt->fData[3];
		DBG("ALGO_SEDENTARY:%d,%d\n", res->sedentary.type, res->sedentary.stand_count);
		break;

	case ALGO_HANDUP:
		res->handup = (uint32_t)evt->fData[1];
		DBG("ALGO_HANDUP:%d\n", res->handup);
		break;

	case ALGO_ACTTYPEDETECT:
		res->activity_detect = (uint32_t)evt->fData[0];
		break;

	case ALGO_ACTIVITY_OUTPUT:
		sensor_activity_data_save(evt);
		break;
	case ALGO_INACTIVITY_OUTPUT:
		sensor_inactivity_data_save(evt);
		break;
	}
}

/* sensor algo event handler */
static void sensor_algo_handler(SensorEVT_t *event)
{
	sensor_evt_t *evt = (sensor_evt_t *)event;

	// save result
	sensor_algo_save(evt);

	// call user handler
	if (user_handler != NULL) {
		user_handler(evt);
	}
}

/* Init sensor algo */
int sensor_algo_init(const sensor_os_api_t *api)
{
	SettingControl_t scl;
	char chipInfo[64];

	// config os api
	if (api == NULL) {
		return -1;
	}
	user_handler = api->user_handler;

	// pre-init
	CWM_LibPreInit((os_api *)&cwm_os_api);

	//config chip vendor
	memset(&scl, 0, sizeof(scl));
	scl.iData[0] = 1;
	scl.iData[1] = 2; // 0: mcu_auto_detect 2: skip_mcu_auto_detect, 16010100
	scl.iData[2] = 0;
	scl.iData[3] = 0;
	CWM_SettingControl(SCL_CHIP_VENDOR_CONFIG, &scl);
	DBG("cus verify key:%d\n", scl.iData[2]);

	//Registering algorithmic event output callbacks
	CWM_LibPostInit(sensor_algo_handler);

	CWM_Sensor_Enable(IDX_REQUEST_SENSOR);

#ifdef CONFIG_SENSOR_ALGO_MOTION_CYWEE_DML
	//DML init
	CWM_Dml_LibInit();

	//get dml lib version information
	memset(&scl, 0, sizeof(scl));
	scl.iData[0] = 1;
	CWM_SettingControl(SCL_DML_GET_LIB_INFO, &scl);
	DBG("dml version:%d.%d.%d.%d\n", scl.iData[1], scl.iData[2], scl.iData[3], scl.iData[4]);
#endif

	//get lib version information
	memset(&scl, 0, sizeof(scl));
	scl.iData[0] = 1;
	CWM_SettingControl(SCL_GET_LIB_INFO, &scl);
	DBG("alg version:%d.%d.%d.%d product:%d model:%d\n", scl.iData[1], scl.iData[2],
	    scl.iData[3], scl.iData[4], scl.iData[5], scl.iData[6]);

	//get Chip Information
	memset(&scl, 0, sizeof(scl));
	memset(chipInfo, 0, sizeof(chipInfo));
	scl.iData[0] = 1;
	scl.iData[1] = 1; //2: skip_auto_detect
	scl.iData[2] = (int)chipInfo;
	scl.iData[3] = sizeof(chipInfo);
	scl.iData[4] = 0;
	scl.iData[5] = 0;
	scl.iData[6] = 0;
	CWM_SettingControl(SCL_GET_CHIP_INFO, &scl);
	DBG("have_security = %d.%d ret_buff_size = %d  chipInfo = %s\n", scl.iData[5], scl.iData[6],
	    scl.iData[4], chipInfo);

#if 0
	memset(&scl, 0, sizeof(scl));
	scl.iData[0] = 1;
	scl.iData[3] = 1 + 8;
	scl.iData[4] = 1 + 4;
	scl.iData[5] = 1 + 2 + 4;
	scl.iData[7] = -1;
	CWM_SettingControl(SCL_LOG, &scl);
#endif

	memset(&scl, 0, sizeof(scl));
	scl.iData[0] = 1;
	scl.iData[1] = 30; // age
	scl.iData[2] = 1; // gender, 0:female, 1:male
	scl.iData[3] = 70; // weight, 10-300kg
	scl.iData[4] = 170; // height, 30-300cm
	scl.iData[5] = 0; // wearing method, 0:left hand, 1:right
	CWM_SettingControl(SCL_USER_INFO, &scl);

	//Enable DT calibration
	memset(&scl, 0, sizeof(scl));
	scl.iData[0] = 1;
	scl.iData[1] = 1; //(0: default turn_off, 1: turn_on, 2: turn_off)
	CWM_SettingControl(SCL_INPUT_DT_CONFIG, &scl);

	//setting the algorithm ODR
	memset(&scl, 0, sizeof(scl));
	scl.iData[0] = 1;
	scl.iData[1] = 50;
	int period = sensor_hal_get_data_period(ID_ACC);
	if (period > 0) {
		scl.iData[1] = 1000 / sensor_hal_get_data_period(ID_ACC);
	}

	DBG("algorithm ODR: %d\n", scl.iData[1]);
	CWM_SettingControl(SCL_ALGO_PROC_CONFIG, &scl);

    // pedometer config
    memset(&scl, 0, sizeof(scl));
	scl.iData[0] = 1;
	scl.iData[1] = 380; //floor_Height (cm)(ÔO¶¨˜ÇŒÓ¸ß¶È)(default: 380cm = 3.8m)(0: default, 150 - 1000)
	scl.iData[2] = 4; //Anti-misoperation (Ò»°ãÄ£Ê½•rµÄ·ÀÕ`Ó‹²½µÈ¼‰) (default: 4)(0: default, 1: ×îÈÝÒ×Õ`Ó‹²½, 2, 3, 4:îAÔOµÈ¼‰, 5, 6, 7: ×î²»ÈÝÒ×Õ`Ó‹²½)
	scl.iData[3] = 4; //‘ôÍâÅÜ²½ºÍÒ»°ãÄ£Ê½•r¾àëx¡¢ÅäËÙºÍ²½¾àµÄÕ{Õû (default: 4)(0: default, 1: ¾àëx×î¶Ì, 2, 3, 4:îAÔO, 5, 6, 7:¾àëx×îéL)
	scl.iData[4] = 1; //enable_extra_packet (default: ²»°lËÍ)(0: default, 1: °lËÍ, 2: ²»°lËÍ)
	scl.iData[5] = 0; //Treadmill__steps (ÅÜ²½™CÐ£Õý…¢”µ)(steps) (default: ²»Ð£Õý)(0: ²»Ð£Õý)
	scl.iData[6] = 0; //Treadmill__average_step_frequency (ÅÜ²½™CÐ£Õý…¢”µ) (steps/minute) (default: ²»Ð£Õý)(0: ²»Ð£Õý)
	scl.iData[7] = 0; //Treadmill__uncalibration_distance (ÅÜ²½™CÐ£Õý…¢”µ)(meters) (default: ²»Ð£Õý)(0: ²»Ð£Õý)
	scl.iData[8] = 0; //Treadmill__ground_truth_distance (ÅÜ²½™CÐ£Õý…¢”µ: ÕæŒµÄÅÜ²½¾àëx)(meters) (default: ²»Ð£Õý)(0: ²»Ð£Õý)
	scl.iData[9] = 3; //floor_sensitivity (˜ÇŒÓÓ‹Ëã‡ÀÖ”¶È)(default: 3)(0: default, 1:Œ’ÈÝ, 2, 3, 4, 5, 7: ‡À¸ñ)
	scl.iData[10] = 1; //·À¸ßîlÕ`Ó| (default: é_†¢)(0: default, 1: é_†¢, 2: êPé])
	scl.iData[11] = 1; //¸ß²½îl×ßÂ·²½¾àÐÞÕý (default: é_†¢)(0: default, 1: é_†¢, 2: êPé])
	scl.iData[12] = 60; //»î„Ó³Ì¶È…^égµÄÖÐ¶È»î„Ó²½îl (default: 60)(0: default, 60 - 270)
	scl.iData[13] = 120; //»î„Ó³Ì¶È…^égµÄ¸ß¶È»î„Ó²½îl (default: 120)(0: default, 60 - 270)
	CWM_SettingControl(SCL_PEDO_CONFIG, &scl);

	//turn on Active Mode - General Pedometer
	memset(&scl, 0, sizeof(scl));
	scl.iData[0] = 1;
	scl.iData[1] = 1001;
	CWM_SettingControl(SCL_SET_ACTIVITY_MODE, &scl);

	memset(&scl, 0, sizeof(scl));
	scl.iData[0] = 1;
	scl.iData[5] = 1;
	scl.iData[10] = 1;
	CWM_SettingControl(SCL_ACTIVITY_CONFIG, &scl);

	//turn on hand updown
	memset(&scl, 0, sizeof(scl));
	scl.iData[0] = 1;
	scl.iData[1] = 5;
	scl.iData[2] = 5;
	scl.iData[3] = 5;
	CWM_SettingControl(SCL_HAND_UPDOWN_CONFIG, &scl);

	memset(&scl, 0, sizeof(scl));
	scl.iData[0] = 1;
	scl.iData[1] = 1; // 1:on,2:off
	scl.iData[4] = 5; // ride_time:5-15
	CWM_SettingControl(SCL_AR_ALERT_CONFIG, &scl);

#ifdef CONFIG_SENSOR_ALGO_MOTION_CYWEE_DML
	//bypass DML read sensor data
	memset(&scl, 0, sizeof(scl));
	scl.iData[0] = 1;
	scl.iData[2] = 2 + 16; //skip sensor input / skip cwm process
	CWM_SettingControl(SCL_DML_DEBUG, &scl);

	//hw id config
	memset(&scl, 0, sizeof(scl));
	scl.iData[0] = 1;
	scl.iData[1] = 2;
	scl.iData[2] = 1 + 4;
	//scl.iData[6] = 3001402;	//LIS2DW12
	scl.iData[6] = 6000902; //STK8321
	scl.iData[7] = 1 + 4;
	CWM_SettingControl(SCL_DML_DRV_HW_CONFIG, &scl);

	//AG config
	memset(&scl, 0, sizeof(scl));
	scl.iData[0] = 1;
	scl.iData[1] = 1;
	scl.iData[3] = 2; // 1:poll 2:fifo
	scl.iData[4] = 1; // 1:any 2:dt & fifo max 3:custom time
	scl.iData[5] = 0; // custom_data_period (default: 0 us)
	scl.iData[6] = 25; //fifo count
	scl.iData[7] = 0; //axis mode
	scl.iData[8] = 50; //odr (default: 50)
	scl.iData[9] = 16; //acc range (default: 16)
	//scl.iData[10] = 2000; // gyro range (default: 2000)
	scl.iData[11] = 1; // int1  1:data_ready 2:any_motion 4:watermark
	//scl.iData[12] = 4; // int2  1:data_ready 2:any_motion 4:watermark
	CWM_SettingControl(SCL_DML_DRV_AG_CONFIG, &scl);

	//DML drive init
	memset(&scl, 0, sizeof(scl));
	scl.iData[0] = 1;
	scl.iData[1] = 1; //1:auto, 2:CWM_process, 3:CWM_process2
	CWM_SettingControl(SCL_DML_DRV_INIT, &scl);
	DBG("find sensor count: %d\n", scl.iData[2]);

	//enable sensor
	memset(&scl, 0, sizeof(scl));
	scl.iData[0] = 1;
	scl.iData[1] = 2; //1: manual_enable, 2: auto_enable
	scl.iData[2] = 1; //0: manual_all_off, 1: acc, 2: gyro, 4: mag, 8: acc_temp
	//scl.iData[4] = 1; //1: acc_any_motion
	CWM_SettingControl(SCL_DML_DRV_ENABLE, &scl);

	//get AG config
	memset(&scl, 0, sizeof(scl));
	scl.iData[0] = 1;
	scl.iData[1] = 2;
	CWM_SettingControl(SCL_DML_DRV_AG_CONFIG, &scl);
	DBG("acc and gry odr:%d Hz, %d\n", scl.iData[8], scl.iData[9]);
	if (scl.iData[8] == 0) {
		DBG("fatal error: acc odr=%d Hz", scl.iData[8]);
	} else {
		sensor_period_ms = 1000 / scl.iData[8];
	}
#else
	memset(&scl, 0, sizeof(scl));
	scl.iData[0] = 1;
	scl.iData[1] = 2024;
	scl.iData[2] = 3;
	scl.iData[3] = 1;
	scl.iData[4] = 8;
	scl.iData[5] = 0;
	scl.iData[5] = 0;
	CWM_SettingControl(SCL_DATE_TIME, &scl);

	memset(&scl, 0, sizeof(scl));
	scl.iData[0] = 1;
	scl.iData[1] = 60; // minutes?
	scl.iData[2] = 1; // (default: turn_off)(0: default, 1: turn on, 2: turn off)
	scl.iData[3] = 5; //reminder_time(minutes)(ÌáÐÑ•rég)(default : 5)(0: default, 1 - 10)
	scl.iData[4] = 3; // ½â³ý¾Ã×ø î‘BµÄµÈ¼‰(default : 3)(1 : ÈÝÒ×½â³ý, 5 : ²»ÈÝÒ×½â³ý)
	scl.iData[5] = 1; // ½â³ý¾Ã×øµÄËã·¨Ä£Ê½(default: 1)(1 : ÐÐ×ßÄ£Ê½, 2: »î„ÓÁ¿Ä£Ê½)
	CWM_SettingControl(SCL_SEDENTARY, &scl);
#endif

	memset(&scl, 0, sizeof(scl));
	scl.iData[0] = 1;
	scl.iData[1] = 1; // mode_number, 0:none; 1:¾Ã×ø;2:Ë¯Ãß;3:¾Ã×øºÍÐ¡Ë¯;4:Ð¡Ë¯;5:Ë¯ÃßºÍÓÐÐ§Õ¾Á¢
	scl.iData[2] = 22; // auto_time_sleep_on_sed_off (hours)        (0: disable, 1 - 24)
	scl.iData[3] = 8; // auto_time_sleep_off_sed_on (hours)        (0: disable, 1 - 24)
	scl.iData[4] = 12; // auto_time_sed_off_nap_on (hours)        (0: disable, 1 - 24)
	scl.iData[5] = 14; // auto_time_nap_off_sed_on (hours)        (0: disable, 1 - 24)"
	CWM_SettingControl(SCL_SET_INACTIVITY_MODE, &scl);

	memset(&scl, 0, sizeof(scl));
	scl.iData[0] = 1;
	scl.iData[2] =
		2; // ÊÇ·ñÊ¹ÓÃÐÄÂÊíÅÐ”àË¯Ãß î‘B (default: ²»Ê¹ÓÃÐÄÂÊ)    (0: default, 1: Ê¹ÓÃÐÄÂÊ, 2: ²»Ê¹ÓÃÐÄÂÊ)
	scl.iData[3] =
		4; // ÈëË¯ëyÒ×¶È (default: 4)    (0: default, 1:²»ÈÝÒ×ÈëË¯, 2, 3, 4:˜ËœÊ, 5, 6, 7:ÈÝÒ×ÈëË¯)
	scl.iData[4] =
		4; // ³öË¯ëyÒ×¶È (default: 4)    (0: default, 1:²»ÈÝÒ×³öË¯, 2, 3, 4:˜ËœÊ, 5, 6, 7:ÈÝÒ×³öË¯)
	scl.iData[6] =
		4; // ÇåÐÑëyÒ×¶È (default: 4)    (0: default, 1:²»ÈÝÒ×ÇåÐÑ, 2, 3, 4:˜ËœÊ, 5, 6, 7:ÈÝÒ×ÇåÐÑ)
	scl.iData[7] =
		1; // Ó²ówÓÌ–µÈ¼‰ (default: 1)    (0: default, 1: µÍësÓ, 2, 3, 4, 5, 6, 7: ¸ßësÓ)
	scl.iData[9] = 60; // Ë¯Ãßég¾à (default: 60)    (0: default, 1 - 180)"
	CWM_SettingControl(SCL_SLEEP_CONFIG, &scl);

	memset(&scl, 0, sizeof(scl));
	scl.iData[0] = 1;
	scl.iData[1] = 1001;
	CWM_SettingControl(SCL_SET_ACTIVITY_MODE, &scl);

	//enable algorithm output
	CWM_Sensor_Enable(IDX_ALGO_ACTIVITY_OUTPUT);
	CWM_Sensor_Enable(IDX_ALGO_WATCH_HANDUP);
	CWM_Sensor_Enable(IDX_ALGO_INACTIVITY_OUTPUT);
	CWM_Sensor_Enable(IDX_OFFBODY_DETECT);

	return 0;
}

/* Control sensor algo */
int sensor_algo_control(uint32_t ctl_id, sensor_ctl_t *ctl)
{
	return CWM_SettingControl(ctl_id, (SettingControl_t *)ctl);
}

int sensor_get_hr_mode(void)
{
	return hr_mode;
}

/* Enable sensor id */
int sensor_algo_enable(uint32_t id, uint32_t func)
{
	DBG("sensor_algo_enable, id=%d\n", id);

	if (id == IN_HEARTRATE) {
		hr_mode = func;
	}

	return CWM_Sensor_Enable(id);
}

/* Disable sensor id */
int sensor_algo_disable(uint32_t id, uint32_t func)
{
	DBG("sensor_algo_disable, id=%d\n", id);
	return CWM_Sensor_Disable(id);
}

/* Input sensor raw data */
void sensor_algo_input(sensor_raw_t *dat)
{
	CWM_CustomSensorInput((CustomSensorData *)dat);
}

/* Input sensor fifo init */
void sensor_algo_input_fifo_init(int num)
{
	CWM_CustomSensorInput_Fifo_Init(num);
}

/* Input sensor fifo start */
int sensor_algo_input_fifo_start(uint32_t id, uint64_t timestamp, uint64_t delta)
{
	return CWM_CustomSensorInput_Fifo_Start(id, timestamp, delta);
}

/* Input sensor fifo end */
void sensor_algo_input_fifo_end(uint32_t id)
{
	CWM_CustomSensorInput_Fifo_End(id);
}

/* Get the duration time of the next event */
int64_t sensor_algo_get_next_duration(void)
{
	return CWM_GetNextActionDuration_ns();
}

/* Read sensor data and output through sensor call-back function */
int sensor_algo_process(uint32_t id)
{
	return CWM_process();
}

/* Read sensor algorithm result */
sensor_res_t *sensor_algo_get_result(void)
{
	return &sensor_algo_res;
}

/* Dump sensor event */
void sensor_algo_dump_event(sensor_evt_t *evt)
{
	switch (evt->id) {
	case REQ_SENSOR:
		DBG("%s: id: %d, on: %d\r\n", "req_sensor", (int)evt->fData[1], (int)evt->fData[2]);
		break;

	case RAW_ACCEL:
		DBG("%s: %.4f, %.4f, %.4f\r\n", "accel", evt->fData[0], evt->fData[1],
		    evt->fData[2]);
		break;

	case RAW_GYRO:
		DBG("%s: %.4f, %.4f, %.4f\r\n", "gyro", evt->fData[0], evt->fData[1],
		    evt->fData[2]);
		break;

	case RAW_MAG:
		DBG("%s: %.4f, %.4f, %.4f,%.4f\r\n", "mag", evt->fData[0], evt->fData[1],
		    evt->fData[2], evt->fData[3]);
		break;

	case RAW_BARO:
		DBG("%s: %.4f\r\n", "baro", evt->fData[0]);
		break;

	case RAW_TEMP:
		DBG("%s: %.4f\r\n", "temp", evt->fData[0]);
		break;

	case RAW_HEARTRATE:
		DBG("%s: %.4f, %.4f\r\n", "heart rate", evt->fData[0], evt->fData[1]);
		break;

	case RAW_GNSS:
		DBG("%s: %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f\r\n", "gnss",
		    evt->fData[0], evt->fData[1], evt->fData[2], evt->fData[3], evt->fData[4],
		    evt->fData[5], evt->fData[6], evt->fData[7], evt->fData[8]);
		break;

	case RAW_OFFBODY:
		DBG("%s: %.4f, %.4f, %.4f\r\n", "offbody", evt->fData[0], evt->fData[1],
		    evt->fData[2]);
		break;

	case ALGO_SEDENTARY:
		DBG("%s: %.4f, %.4f, %.4f\r\n", "sedentary", evt->fData[0], evt->fData[1],
		    evt->fData[2]);
		break;

	case ALGO_HANDUP:
		DBG("%s: %.4f, %.4f, %.4f\r\n", "watch handup", evt->fData[0], evt->fData[1],
		    evt->fData[2]);
		break;

	case ALGO_SLEEP:
		DBG("%s: %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f\r\n", "sleep", evt->fData[0],
		    evt->fData[1], evt->fData[2], evt->fData[3], evt->fData[4], evt->fData[5],
		    evt->fData[6]);
		break;

	case ALGO_ACTTYPEDETECT:
		DBG("%s: %.4f, %.4f\r\n", "act type", evt->fData[0], evt->fData[1]);
		break;

	case ALGO_SHAKE:
		DBG("%s: %.4f, %.4f\r\n", "shake", evt->fData[0], evt->fData[1]);
		break;

	case ALGO_ACTIVITY_OUTPUT:
		DBG("%s: %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, "
		    "%.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f\r\n",
		    "activity output", evt->fData[0], evt->fData[1], evt->fData[2], evt->fData[3],
		    evt->fData[4], evt->fData[5], evt->fData[6], evt->fData[7], evt->fData[8],
		    evt->fData[9], evt->fData[10], evt->fData[11], evt->fData[12], evt->fData[13],
		    evt->fData[14], evt->fData[15]);
		break;
	}
}

/* Dump sensor result */
void sensor_algo_dump_result(sensor_res_t *res)
{
	struct gnss_s *gnss = &res->gnss;
	struct sleeping_s *sleeping = &res->sleeping;
	struct pedometer_s *pedometer = &res->pedometer;
	struct biking_s *biking = &res->biking;

	DBG("\r\n");
	DBG("[base] orien=%.1f pres=%.1f alt=%.1f temp=%.1f hr=%.1f\r\n", res->orientation,
	    res->pressure, res->altitude, res->temperature, res->heart_rate);
	DBG("\r\n");

	DBG("[body] onbody=%d seden=%d handup=%d act_mode=%d act_detect=%d\r\n", res->onbody,
	    res->sedentary.type, res->handup, res->activity_mode, res->activity_detect);
	DBG("\r\n");

	DBG("[gnss] lat=%.1f lon=%.1f alt=%.1f bear=%.1f speed=%.1f\r\n", gnss->latitude,
	    gnss->longitude, gnss->altitude, gnss->bearing, gnss->speed);
	DBG("\r\n");

	DBG("[sleeping] sta=%d duration=%d enter=0x%x exit=0x%x\r\n", sleeping->status,
	    sleeping->duration_total, *(uint32_t *)(&sleeping->time_enter),
	    *(uint32_t *)(&sleeping->time_exit));
	DBG("  shallow: duration=%d time=0x%x\r\n", sleeping->duration_shallow,
	    *(uint32_t *)(&sleeping->time_shallow));
	DBG("     deep: duration=%d time=0x%x\r\n", sleeping->duration_deep,
	    *(uint32_t *)(&sleeping->time_deep));
	DBG("    awake: duration=%d time=0x%x\r\n", sleeping->duration_awake,
	    *(uint32_t *)(&sleeping->time_awake));
	DBG("      rem: duration=%d time=0x%x\r\n", sleeping->duration_rem,
	    *(uint32_t *)(&sleeping->time_rem));
	DBG("\r\n");

	DBG("[pedometer] steps=%.1f distance=%.1f calories=%.1f type=%d\r\n",
	    pedometer->total_steps, pedometer->total_distance, pedometer->total_calories,
	    pedometer->activity_type);
	DBG(" step_freq: cur=%.1f avg=%.1f max=%.1f\r\n", pedometer->cur_step_freq,
	    pedometer->avg_step_freq, pedometer->max_step_freq);
	DBG("  step_len: cur=%.1f avg=%.1f\r\n", pedometer->cur_step_len, pedometer->avg_step_len);
	DBG("      pace: cur=%.1f avg=%.1f max=%.1f\r\n", pedometer->cur_pace, pedometer->avg_pace,
	    pedometer->max_pace);
	DBG("    elevat: slope=%.1f elev=%.1f up=%.1f down=%.1f f-up=%.1f f-down=%.1f\r\n",
	    pedometer->slope, pedometer->elevation, pedometer->elevation_up,
	    pedometer->elevation_down, pedometer->floors_up, pedometer->floors_down);
	DBG("        hr: vo2max=%.1f hr_inten=%.1f hr_zone=%.1f\r\n", pedometer->vo2max,
	    pedometer->hr_intensity, pedometer->hr_zone);
	DBG("\r\n");

	DBG("[biking] lat=%.1f lon=%.1f distance=%.1f calories=%.1f\r\n", biking->latitude,
	    biking->longitude, biking->distance, biking->calories);
	DBG("  speed: cur=%.1f avg=%.1f max=%.1f\r\n", biking->cur_speed, biking->avg_speed,
	    biking->max_speed);
	DBG(" elevat: slope=%.1f elev=%.1f\r\n", biking->slope, biking->elevation);
	DBG("        hr: vo2max=%.1f hr_inten=%.1f hr_zone=%.1f\r\n", biking->vo2max,
	    biking->hr_intensity, biking->hr_zone);
	DBG("\r\n");
}

/* Convert sensor raw data (cywee DML) */
void sensor_algo_convert_data(sensor_cvt_ag_t *dat)
{
#ifdef CONFIG_SENSOR_ALGO_MOTION_CYWEE_DML
	CWM_Dml_custData_converter_ag((dml_cust_data_ag_t *)dat);
#endif
}

/* Get sensor data period(ms) (cywee DML) */
int sensor_algo_get_data_period(uint32_t id)
{
#ifdef CONFIG_SENSOR_ALGO_MOTION_CYWEE_DML
	return sensor_period_ms;
#else
	return sensor_hal_get_data_period(id);
#endif
}
