/******************************************************************************/
/*                                                                            */
/*    Copyright 2023 by AICXTEK TECHNOLOGIES CO.,LTD. All rights reserved.    */
/*                                                                            */
/******************************************************************************/

/**
 * @file aic_srv_net.h
 *
 */

#ifndef __AIC_SRV_NET_H__
#define __AIC_SRV_NET_H__

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include <stdint.h>
#include <aic_type.h>
#include "aic_srv_bus.h"

/*********************
 *      DEFINES
 *********************/
#define ANET_PDP_ACTIVATED 1
#define ANET_PDP_DEACTIVATED 0

#define ANET_DEFAULT_PDP_CID 1

#define ANET_MAX_NET_INFO_CBS 5

#define ANET_IPV4_DNS_SERVER1_BACKUP "114.114.114.114"
#define ANET_IPV4_DNS_SERVER2_BACKUP "8.8.8.8"

#define ANET_ERROR_PDP_MGR_BASE 100
#define ANET_ERROR_NET_CBS_OVERFLOW ANET_ERROR_PDP_MGR_BASE

/**********************
 *      TYPEDEFS
 **********************/
typedef struct WAN_NET_STRUCT
{
    u8 pdp_cid;
    u8 pdp_activated;      /* PDP is activated or not */
    u32 ipv4_addr;
    u32 ipv4_dns1_addr;
    u32 ipv4_dns2_addr;
} WAN_NET;

typedef void (*aic_net_info_change_cb)(WAN_NET *wan_net_ptr);


/**********************
 * GLOBAL PROTOTYPES
 **********************/
void aic_net_mgr_init(void);
u32 aic_register_net_info_cb(aic_net_info_change_cb cb);
u32 aic_unregister_net_info_cb(aic_net_info_change_cb cb);

int32_t aic_srv_tcpip_net_init();
int32_t aic_srv_at_net_init();

int32_t aic_srv_net_deinit(void);


size_t aic_srv_net_read(void *buffer, size_t size, int32_t timeout);

size_t aic_srv_net_write(const void *buffer, size_t size, int32_t timeout);

int aic_at_network_init(void);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /* __AIC_SRV_NET_H__ */

