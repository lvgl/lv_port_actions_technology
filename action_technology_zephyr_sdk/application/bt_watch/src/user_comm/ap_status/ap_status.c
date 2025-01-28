/*!
 * \file      ap_status.c
 * \brief     
 * \details   
 * \author 
 * \date  
 * \copyright Actions
 */

#include <user_comm/ap_status.h>

static ap_status_t ap_status = AP_STATUS_NONE;

void ap_status_set(ap_status_t status)
{
	ap_status = status;
}

ap_status_t ap_status_get(void)
{
	return ap_status;
}

