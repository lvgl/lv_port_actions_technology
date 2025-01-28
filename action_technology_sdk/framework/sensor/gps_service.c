/*
 * Copyright (c) 2022 Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief gps service interface
 */

#include <mem_manager.h>
#include <app_manager.h>
#include <srv_manager.h>
#include <msg_manager.h>
#include <sys_manager.h>
#include <thread_timer.h>
#include <gps_manager.h>
#include <device.h>
#include <stdio.h>
#include <string.h>

#include <gps/gps.h>
#include <minmea.h>

#define CONFIG_GPS_DEV_NAME	"gps"
#define CONFIG_SENSORSRV_STACKSIZE 2048
#define CONFIG_GPSSVR_PRIORITY 6


static gps_res_cb_t gps_res_cb = { NULL };
static gps_res_t gps_res;

void gps_event_report_nmea(uint8_t* gps_nmea_data)
{
	struct app_msg  msg = {0};

	msg.type = MSG_SENSOR_EVENT;
	msg.cmd = MSG_GPS_NMEA_DATA;
	msg.ptr = gps_nmea_data;
	send_async_msg(GPS_SERVICE_NAME, &msg);
}

static void gps_notify_callback(struct device *dev,struct gps_value *val)
{
	gps_event_report_nmea(val->gps_nmea_data);
}

static void _gps_service_init(void)
{
    const struct device *gps_dev = device_get_binding("gps");
	if (!gps_dev) {
		SYS_LOG_ERR("cannot found key dev gps\n");
	}
	memset(&gps_res, 0, sizeof(gps_res));
    gps_dev_register_notify(gps_dev, gps_notify_callback);
}

static void gps_enable(void)
{
	const struct device *gps_dev = device_get_binding(CONFIG_GPS_DEV_NAME);
	if (!gps_dev) {
		SYS_LOG_ERR("cannot found key dev gps\n");
	}
	gps_dev_enable(gps_dev);
}

static void gps_disable(void)
{
	const struct device *gps_dev = device_get_binding(CONFIG_GPS_DEV_NAME);
	if (!gps_dev) {
		SYS_LOG_ERR("cannot found key dev gps\n");
	}
	gps_dev_disable(gps_dev);
}

static void gps_minmea_parse(const char *sentence)
{
	uint8_t parse_state = false;
	
	SYS_LOG_INF("gps sentence: %s\n", sentence);

	switch (minmea_sentence_id(sentence, false)) 
	{
#ifdef CONFIG_GPS_PARSE_RMC_ENABLE
		case MINMEA_SENTENCE_RMC: {
			struct minmea_sentence_rmc frame;
			if (minmea_parse_rmc(&frame, sentence)) {
				parse_state	= true;
				memcpy(&gps_res.rmc_data, &frame, sizeof(frame));
				SYS_LOG_INF("$xxRMC sentence parse ok\n");
			}
			else {
				SYS_LOG_INF("$xxRMC sentence is not parsed\n");
			}
		} break;
#endif

#ifdef CONFIG_GPS_PARSE_GGA_ENABLE
		case MINMEA_SENTENCE_GGA: {
			struct minmea_sentence_gga frame;
			if (minmea_parse_gga(&frame, sentence)) {
				parse_state	= true;
				memcpy(&gps_res.gga_data, &frame, sizeof(frame));
				// printk( "$xxGGA: hours: %d\n", frame.time.hours);
				// printk( "$xxGGA: minutes: %d\n", frame.time.minutes);
				// printk( "$xxGGA: seconds: %d\n", frame.time.seconds);
				// printk( "$xxGGA: microseconds: %d\n", frame.time.microseconds);
				// printk( "$xxGGA: latitude: %f\n", minmea_tofloat(&frame.latitude));
				// printk( "$xxGGA: longitude: %f\n", minmea_tofloat(&frame.longitude));
				// printk( "$xxGGA: hdop: %f\n", minmea_tofloat(&frame.hdop));
				// printk( "$xxGGA: altitude: %f\n", minmea_tofloat(&frame.altitude));
				// printk( "$xxGGA: height: %f\n", minmea_tofloat(&frame.height));
				// printk( "$xxGGA: dgps_age: %f\n", minmea_tofloat(&frame.dgps_age));
				SYS_LOG_INF("$xxGGA sentence parse ok\n");
			}
			else {
				SYS_LOG_INF( "$xxGGA sentence is not parsed\n");
			}
		} break;
#endif

#ifdef CONFIG_GPS_PARSE_GST_ENABLE
		case MINMEA_SENTENCE_GST: {
			struct minmea_sentence_gst frame;
			if (minmea_parse_gst(&frame, sentence)) {
				parse_state	= true;
				memcpy(&gps_res.gst_data, &frame, sizeof(frame));
				SYS_LOG_INF( "$xxGST sentence parse ok\n");
			}
			else {
				SYS_LOG_INF( "$xxGST sentence is not parsed\n");
			}
		} break;
#endif

#ifdef CONFIG_GPS_PARSE_GSV_ENABLE
		case MINMEA_SENTENCE_GSV: {
			struct minmea_sentence_gsv frame;
			if (minmea_parse_gsv(&frame, sentence)) {
				parse_state	= true;
				memcpy(&gps_res.gsv_data, &frame, sizeof(frame));
				SYS_LOG_INF( "$xxGSV sentence parse ok\n");
			}
			else {
				SYS_LOG_INF( "$xxGSV sentence is not parsed\n");
			}
		} break;
#endif

#ifdef CONFIG_GPS_PARSE_VTG_ENABLE
		case MINMEA_SENTENCE_VTG: {
			struct minmea_sentence_vtg frame;
			if (minmea_parse_vtg(&frame,sentence)) {
				parse_state	= true;
				memcpy(&gps_res.vtg_data, &frame, sizeof(frame));
				SYS_LOG_INF( "$xxVTG sentence parse ok");
			}
			else {
				SYS_LOG_INF( "$xxVTG sentence is not parsed\n");
			}
		} break;
#endif

#ifdef CONFIG_GPS_PARSE_ZDA_ENABLE
		case MINMEA_SENTENCE_ZDA: {
			struct minmea_sentence_zda frame;
			if (minmea_parse_zda(&frame, sentence)) {
				parse_state	= true;
				memcpy(&gps_res.zda_data, &frame, sizeof(frame));
				SYS_LOG_INF( "$xxZDA sentence parse ok\n");
			}
			else {
				SYS_LOG_INF( "$xxZDA sentence is not parsed\n");
			}
		} break;
#endif
		case MINMEA_INVALID: {
			SYS_LOG_INF( "$xxxxx sentence is not valid\n");
		} break;

		default: {
			SYS_LOG_INF( "$xxxxx sentence is not parsed\n");
		} break;
	}

	if ((parse_state == true) && (gps_res_cb != NULL)) {
		gps_res_cb(0, &gps_res);
	}
}

static void _gps_service_proc(struct app_msg *msg)
{
	switch (msg->cmd)
	{
		case MSG_GPS_ENABLE: 
			gps_enable();
			break;

		case MSG_GPS_DISABLE: 
			gps_disable();
			break;	

		case MSG_GPS_NMEA_DATA:
			gps_minmea_parse(msg->ptr);
			break;

		case MSG_GPS_ADD_CB:
			gps_res_cb = (gps_res_cb_t)msg->ptr;
			break;

		case MSG_GPS_REMOVE_CB:
			gps_res_cb = NULL;
			break;
	
		default:
			break;
	}
}

static void _gps_service_main_loop(void *parama1, void *parama2, void *parama3)
{
	struct app_msg msg = {0};
	bool terminaltion = false;
	bool suspended = false;
	int timeout;
	int result = 0;

	printk("gps_service enter");

	while (!terminaltion) {
		timeout = suspended ? OS_FOREVER : thread_timer_next_timeout();
		if (receive_msg(&msg, timeout)) {
			SYS_LOG_INF("gps_service %d %d",msg.type, msg.cmd);
			switch (msg.type) {
			case MSG_INIT_APP:
				_gps_service_init();
				break;
			// case MSG_EXIT_APP:
			// 	_sensor_service_exit();
			// 	terminaltion = true;
			// 	break;
			case MSG_SENSOR_EVENT:
				_gps_service_proc(&msg);
				break;
			// case MSG_SUSPEND_APP:
			// 	SYS_LOG_INF("SUSPEND_APP");
			// 	sensor_sleep_suspend();
			// 	suspended = true;
			// 	break;
			// case MSG_RESUME_APP:
			// 	SYS_LOG_INF("RESUME_APP");
			// 	sensor_sleep_resume();
			// 	suspended = false;
			// 	break;
			default:
				break;
			}
			if (msg.callback) {
				msg.callback(&msg, result, NULL);
			}
		}
		thread_timer_handle_expired();
	}
}


char __aligned(ARCH_STACK_PTR_ALIGN) gpssrv_stack_area[CONFIG_SENSORSRV_STACKSIZE];
SERVICE_DEFINE(gps_service, \
				gpssrv_stack_area, sizeof(gpssrv_stack_area), \
				CONFIG_GPSSVR_PRIORITY, BACKGROUND_APP, \
				NULL, NULL, NULL, \
				_gps_service_main_loop);
