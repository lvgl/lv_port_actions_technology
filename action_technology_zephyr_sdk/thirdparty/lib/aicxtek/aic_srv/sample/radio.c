/******************************************************************************/
/*                                                                            */
/*    Copyright 2023 by AICXTEK TECHNOLOGIES CO.,LTD. All rights reserved.    */
/*                                                                            */
/******************************************************************************/

/**
 *  DESCRIPTION
 *
 *    This file is enter of radio
 */

/*********************
 *      INCLUDES
 *********************/
#include "aic_type.h"
#include "aic_srv_tele.h"

/**********************
 *  STATIC VARIABLES
 **********************/
/**
 * handleRadioCallback.
 * @arg1: p_param;size:p_param data length
 * Return: int,-1,error,0 success.
 */
static int32_t handleRadioCallback(void *p_param, uint32_t size)
{
    ts_event_radio_t *p_radio_event = NULL;

    if(NULL == p_param) {
        alog_error("%s,p_param is null",__func__);
        return -1;
    }

    p_radio_event = (ts_event_radio_t *)p_param;

    switch(p_radio_event->event_id) {
        case TS_EVENT_RADIO_SIGNAL_INFO_UPDATED: /* signal strenth&level reported per 10s */
        {
            ts_radio_signal_info_updated_t *p_signal_info = NULL;
            alog_info("[%s],TS_EVENT_RADIO_SIGNAL_INFO_UPDATED",__func__);

            p_signal_info = (ts_radio_signal_info_updated_t *)p_param;
            alog_info("[%s], signal_strength = %d, signal_level = %d.\r\n",
                     __func__,
                     p_signal_info->signal_strength,
                     p_signal_info->signal_level);

            /* signal level0: signal strenth <=-120dBm */
            /* signal level1: signal strenth -111~-119dBm */
            /* signal level2: signal strenth -106~-110dBm */
            /* signal level3: signal strenth -91~-105dBm */
            /* signal level4: signal strenth >=-90dBm */

            /* update signal
               to ui task: update signal level */
            break;
        }
        case TS_EVENT_RADIO_REG_STATUS_UPDATED: /* register net status reported */
        {
            ts_radio_reg_status_updated_t *p_reg_status = NULL;
            alog_info("[%s],TS_EVENT_RADIO_REG_STATUS_UPDATED",__func__);

            p_reg_status = (ts_radio_reg_status_updated_t *)p_param;
            alog_info("[%s],reg status = %d.\r\n",__func__,p_reg_status->reg_status);
            if(TS_RADIO_REG_STATUS_IN_SERVICE == p_reg_status->reg_status) {
                /* register successfual
                  to ui task: update reg status icon */
            } else {
                /* No register
                  to ui task: update reg status icon */
            }
            break;
        }
        case TS_EVENT_RADIO_IMS_REG_STATUS_UPDATED: /* ims status reported */
        {
            ts_radio_ims_reg_status_updated_t *p_ims_status = NULL;
            alog_info("[%s],TS_EVENT_RADIO_IMS_REG_STATUS_UPDATED",__func__);

            p_ims_status = (ts_radio_ims_reg_status_updated_t *)p_param;
            alog_info("[%s],ims status = %d.\r\n",__func__,p_ims_status->is_ims_on);
            if(true == p_ims_status->is_ims_on) {
                /* ims open
                  to ui task: update ims icon */
            } else {
                /* ims close
                  to ui task: update ims icon */
            }
            break;
        }
        case TS_EVENT_RADIO_OPERATOR_INFO_UPDATED: /* openator name(eg,cmcc,cucc,ctcc) reported*/
        {
            ts_radio_operator_info_updated_t *p_operator_name = NULL;
            alog_info("[%s],TS_EVENT_RADIO_OPERATOR_INFO_UPDATED",__func__);
            p_operator_name = (ts_radio_operator_info_updated_t *)p_param;
            alog_info("[%s],operator short name = %s, long name = %s,",__func__,
                p_operator_name->operator_info.operator_short_name,/*cmcc/cucc/ctcc*/
                p_operator_name->operator_info.operator_long_name);/*China Mobile/China Unicom/china Telecom*/

            /* operator name
               to ui task: display operator name */
            break;
        }

        case TS_EVENT_RADIO_GET_CELL_INFO_RESP: {
            ts_radio_get_cell_info_resp_t *p_cell_info = NULL;

            alog_info("[%s],TS_EVENT_RADIO_GET_CELL_INFO_RESP",__func__);
            p_cell_info = (ts_radio_get_cell_info_resp_t *)p_param;
            alog_info("[%s]ret_value = %d, rat_type = %d, plmn_mcc = %s, plmn_mnc = %s, tac = %d, earfcn = %d, cellid = %d, rsrp = %d, rsrq = %d, pcid = %d, is_roaming = %d, band = %d, ul_bandwidth = %d, dl_bandwidth = %d, sinr = %d, srxlev = %d, rssi = %d.",
                      __func__,
                      p_cell_info->ret_value,
                      p_cell_info->cell_info.rat_type,
                      p_cell_info->cell_info.plmn_mcc,
                      p_cell_info->cell_info.plmn_mnc,
                      p_cell_info->cell_info.tac,
                      p_cell_info->cell_info.earfcn,
                      p_cell_info->cell_info.cellid,
                      p_cell_info->cell_info.rsrp,
                      p_cell_info->cell_info.rsrq,
                      p_cell_info->cell_info.pcid,
                      p_cell_info->cell_info.is_roaming,
                      p_cell_info->cell_info.band,
                      p_cell_info->cell_info.ul_bandwidth,
                      p_cell_info->cell_info.dl_bandwidth,
                      p_cell_info->cell_info.sinr,
                      p_cell_info->cell_info.srxlev,
                      p_cell_info->cell_info.rssi);
            break;
        }

        case TS_EVENT_RADIO_GET_NEIGHBOR_CELL_INFO_LIST_RESP: {
            ts_radio_get_neighbor_cell_info_list_resp_t *p_nb_cell_info_list = NULL;
            uint8_t num = 0;

            alog_info("[%s],TS_EVENT_RADIO_GET_NEIGHBOR_CELL_INFO_LIST_RESP",__func__);
            p_nb_cell_info_list = (ts_radio_get_neighbor_cell_info_list_resp_t *)p_param;
            alog_info("[%s]ret_value = %d.", __func__, p_nb_cell_info_list->ret_value);
            alog_info("[%s]ncell_num = %d.", __func__, p_nb_cell_info_list->ncell_info_list.ncell_num);
            for (num = 0; num < p_nb_cell_info_list->ncell_info_list.ncell_num; num++) {
                alog_info("[%s]index = %d, rat_type = %d, plmn_mcc = %s, plmn_mnc = %s, tac = %d, cellid = %d, earfcn = %d, rsrp = %d, rsrq = %d, pcid = %d, sinr = %d, srxlev = %d, rssi = %d.",
                       __func__,
                       num,
                       p_nb_cell_info_list->ncell_info_list.ncell_info[num].rat_type,
                       p_nb_cell_info_list->ncell_info_list.ncell_info[num].plmn_mcc,
                       p_nb_cell_info_list->ncell_info_list.ncell_info[num].plmn_mnc,
                       p_nb_cell_info_list->ncell_info_list.ncell_info[num].tac,
                       p_nb_cell_info_list->ncell_info_list.ncell_info[num].cellid,
                       p_nb_cell_info_list->ncell_info_list.ncell_info[num].earfcn,
                       p_nb_cell_info_list->ncell_info_list.ncell_info[num].rsrp,
                       p_nb_cell_info_list->ncell_info_list.ncell_info[num].rsrq,
                       p_nb_cell_info_list->ncell_info_list.ncell_info[num].pcid,
                       p_nb_cell_info_list->ncell_info_list.ncell_info[num].sinr,
                       p_nb_cell_info_list->ncell_info_list.ncell_info[num].srxlev,
                       p_nb_cell_info_list->ncell_info_list.ncell_info[num].rssi);
            }
            break;
        }

        case TS_EVENT_RADIO_SET_IMEI_RESP: {
            ts_radio_set_imei_resp_t *p_set_imei = NULL;

            p_set_imei = (ts_radio_set_imei_resp_t *)p_param;
            alog_info("[%s]ret_value = %d.", __func__, p_set_imei->ret_value);
            break;
        }

        case TS_EVENT_RADIO_START_SEARCH_RESP: {
            uint32_t index = 0;
            ts_radio_start_search_resp_t *p_resp = NULL;
            ts_radio_operator_info_t *p_operator_info = NULL;

            p_resp = (ts_radio_start_search_resp_t *)p_param;
            alog_info("[%s]Receive TS_EVENT_RADIO_START_SEARCH_RESP.", __func__);
            alog_info("[%s]event_id = 0x%x, ret_value = %d, operator_num = %d.",
                   __func__,
                   p_resp->event_id,
                   p_resp->ret_value,
                   p_resp->operator_num);
            for (index = 0; index < p_resp->operator_num; index++) {
                p_operator_info = (ts_radio_operator_info_t *)(&(p_resp->data_header)) + index;
                alog_info("[%s][%d]operator_short_name = %s, operator_long_name = %s, plmn_mcc = %s, plmn_mnc = %s, operator_id = %d, network_type = %d, status = %d.",
                       __func__,
                       index,
                       p_operator_info->operator_short_name,
                       p_operator_info->operator_long_name,
                       p_operator_info->plmn_mcc,
                       p_operator_info->plmn_mnc,
                       p_operator_info->operator_id,
                       p_operator_info->network_type,
                       p_operator_info->status);

            }
            break;
        }

        default:
            break;
    }

    return 0;
}


/**********************
 *   GLOBAL FUNCTIONS
 **********************/
/* when power on, open radio,then start network */
void radio_init( void )
{
    int32_t radio_handle = 0;

    /* open network */
    aic_srv_tele_radio_on(0, true, NULL);

    /* 1.register radio callback,this callback receive global msg(eg.signal quality,operator name,ims status)*/
    radio_handle = aic_srv_tele_radio_register(NULL, handleRadioCallback);
}
