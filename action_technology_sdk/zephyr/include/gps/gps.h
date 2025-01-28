/*
 * Copyright (c) 2022 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef _GPS_H_
#define _GPS_H_

#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>
#include <zephyr/types.h>
#include <device.h>
#include <drivers/cfg_drv/dev_config.h>
#include <minmea.h>

#ifdef __cplusplus
extern "C" {
#endif

struct gps_value {
    uint8_t *gps_nmea_data;
};

enum GPS_MSG_ID {
    MSG_GPS_ENABLE,
    MSG_GPS_DISABLE,
	MSG_GPS_NMEA_DATA,
    MSG_GPS_ADD_CB,
    MSG_GPS_REMOVE_CB,
};

typedef struct {
#ifdef CONFIG_GPS_PARSE_RMC_ENABLE
    /* minmea_sentence_rmc */
    struct minmea_sentence_rmc rmc_data;
#endif 

#ifdef CONFIG_GPS_PARSE_GGA_ENABLE
    /* minmea_sentence_gga */
    struct minmea_sentence_gga gga_data;
#endif 

#ifdef CONFIG_GPS_PARSE_GST_ENABLE
    /* minmea_sentence_gst */
    struct minmea_sentence_gst gst_data;
#endif 

#ifdef CONFIG_GPS_PARSE_GSV_ENABLE
    /* minmea_sentence_gsv */
    struct minmea_sentence_gsv gsv_data;
#endif 

#ifdef CONFIG_GPS_PARSE_VTG_ENABLE
    /* minmea_sentence_vtg */
    struct minmea_sentence_vtg vtg_data;
#endif 

#ifdef CONFIG_GPS_PARSE_ZDA_ENABLE
    /* minmea_sentence_zda */
    struct minmea_sentence_zda zda_data;
#endif 

    /* reserve */
}gps_res_t;

typedef void (*gps_notify_t) (struct device *dev, struct gps_value *val);

struct gps_dev_driver_api {
	void (*enable)(const struct device *dev);
	void (*disable)(const struct device *dev);
	void (*inquiry)(const struct device *dev, struct gps_value *val);
	void (*register_notify)(const struct device *dev, gps_notify_t notify);
	void (*unregister_notify)(const struct device *dev, gps_notify_t notify);
};

static inline void gps_dev_enable(const struct device *dev)
{
	const struct gps_dev_driver_api *api = dev->api;

	if (api->enable)
		api->enable(dev);
}

static inline void gps_dev_disable(const struct device *dev)
{
	const struct gps_dev_driver_api *api = dev->api;

	if (api->disable)
		api->disable(dev);
}

static inline void gps_dev_inquiry(const struct device *dev,
					     struct gps_value *val)
{
	const struct gps_dev_driver_api *api = dev->api;

	if (api->inquiry)
		api->inquiry(dev, val);
}

static inline void gps_dev_register_notify(const struct device *dev,
					     gps_notify_t notify)
{
	const struct gps_dev_driver_api *api = dev->api;

	if (api->register_notify)
		api->register_notify(dev, notify);
}

static inline void gps_dev_unregister_notify(const struct device *dev,
					       gps_notify_t notify)
{
	const struct gps_dev_driver_api *api = dev->api;

	if (api->unregister_notify)
		api->unregister_notify(dev, notify);
}


#ifdef __cplusplus
}
#endif

#endif  
