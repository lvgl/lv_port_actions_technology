#ifndef __AIC_NET_H__
#define __AIC_NET_H__

/* Ensure that standard C is used to process the API information. */
#ifdef __cplusplus
extern "C" {
#endif

/*
*********************************************************************
* Macro      : AIC_USE_PLATFORM_SOCKET
* Description: Define this macro if the mcu platform have the BSD
*              socket network component;Otherwise do not define it.
*********************************************************************
*/
//#define AIC_USE_PLATFORM_SOCKET

struct aic_network_stats
{
    unsigned int ul_bytes;
    unsigned int ul_packets;
    unsigned int dl_bytes;
    unsigned int dl_packets;
};

void aic_get_network_stats(struct aic_network_stats *stats);
void aic_reset_network_stats();

/* Ensure that standard C is used to process the API information. */
#ifdef __cplusplus
}
#endif

#endif/* __AIC_NET_H__ */

