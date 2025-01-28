#ifndef __GPS_MANAGER_H__
#define __GPS_MANAGER_H__

#include "minmea.h"
#include <gps/gps.h>

#define GPS_SERVICE_NAME    "gps_service"


typedef int (*gps_res_cb_t)(int evt_id, gps_res_t *res);


int gps_manager_init(void);
int gps_manager_enable(uint32_t id, uint32_t func);
int gps_manager_disable(uint32_t id, uint32_t func);
int gps_manager_add_callback(gps_res_cb_t cb);
int gps_manager_remove_callback(gps_res_cb_t cb);

#endif
