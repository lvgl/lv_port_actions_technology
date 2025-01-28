/**************************************************************************/
/*                                                                        */
/* Copyright 2024 by AICXTEK TECHNOLOGIES CO.,LTD. All rights reserved.   */
/*                                                                        */
/**************************************************************************/
#include <shell/shell.h>
#include "aic_srv_version.h"
#if 0
#include "aic_srv_tele.h"
#else
#include "aic_srv_tele_radio.h"
#include "aic_srv_tele_sim.h"
#include "aic_srv_tele_data.h"
#include "aic_srv_tele_call.h"
#include "aic_srv_tele_sms.h"
#endif
#include "aic_srv_voice.h"
#include "aic_ctrl.h"
#include "aic_portable.h"
#include "aic_network_speed.h"
#include "aic_call_player.h"
#include "aic_srv_wifi_scan.h"

void printfspeed(int errcode,int dlspeed)
{
    alog_info("[%s],enter\r\n", __func__);
    if(errcode == SPEED_NO_ERROR)
        alog_info("speed[%d]\r\n", dlspeed);
}

static int32_t aic_run_network_speed_download_test(const struct shell *shelll, size_t argc, char **argv)
{

    alog_info("[%s],enter\r\n", __func__);
    aic_network_speed_download_test(printfspeed);
    return 0;
}
SHELL_CMD_ARG_REGISTER(aic_speed_dl, NULL, NULL, aic_run_network_speed_download_test, 1,0);

static int32_t aic_run_network_speed_upload_test(const struct shell *shelll, size_t argc, char **argv)
{

    alog_info("[%s],enter\r\n", __func__);
    aic_network_speed_upload_test(printfspeed);
    return 0;
}
SHELL_CMD_ARG_REGISTER(aic_speed_ul, NULL, NULL, aic_run_network_speed_upload_test, 1,0);

static ts_call_info_list_t s_call_info_list = {0};

static int32_t handleRadioCallback(void *p_param, uint32_t size)
{
    ts_event_radio_t *p_radio_event = NULL;

    if(NULL == p_param) {
        alog_error("%s,p_param is null",__func__);
        return -1;
    }

    p_radio_event = (ts_event_radio_t *)p_param;

    switch(p_radio_event->event_id) {
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

static int32_t handleCallCallback(void *p_param, uint32_t size)
{
    ts_event_call_t *p_call_event = NULL;
    //int ret_audio = 0;

    if(NULL == p_param) {
        alog_error("handleCallCallback,p_param is null");
        return -1;
    }

    p_call_event = (ts_event_call_t *)p_param;

    switch (p_call_event->event_id) {
        case TS_EVENT_CALL_STATUS_UPDATED: {
            ts_call_status_updated_t *p_call_status = {0};
            ts_call_info_list_t call_info_list = {0};

            p_call_status = (ts_call_status_updated_t *)p_param;
            //alog_info("handleCallCallback,p_call_status status = %d",  p_call_status->call_status);
            aic_srv_tele_call_get_call_info_list(0, &call_info_list);
            memcpy(&s_call_info_list, &call_info_list, sizeof(ts_call_info_list_t));

            switch(p_call_status->call_status) {
                case TS_CALL_STATUS_ACTIVE:/* remote accept call,setup call success,display connected call win(include,phone number and call duration) */
                    /* 1.stop play local ring */

                    /* 2.stop play local ringback */

                    /* 3.start voice */
                    aic_call_voice_start();

                    /* 4.enter connected win and display call time*/
                    break;

                case TS_CALL_STATUS_INCOMING: {/* incoming a call,phone recieve a call */
                    alog_info("handleCallCallback,call_active: = %d call_number = %d", p_call_status->call_status,call_info_list.call_number);
                    if (1 < call_info_list.call_number) {
                        int ret = -1;
                        uint8_t cur_card_id = TS_CARD_ID_MAX;
                        aic_srv_tele_sim_get_current_card(&cur_card_id);
                        ret = aic_srv_tele_call_release(cur_card_id, p_call_status->call_idx, NULL);
                        alog_info("handleCallCallback,aic_srv_tele_call_release return,ret = %d.\r\n", ret);
                        return -1;
                    } else {
                        /* play local ring */
                    }

                    /* 2. to ui task,display incoming call win */
                    break;
                }

                case TS_CALL_STATUS_RELEASED: {/*hung up call,user hungup and remote hungup,this message is coming */
                    /* if there is other call index, do not need to stop voice */
                    alog_info("handleCallCallback,TS_CALL_STATUS_RELEASED call_number = %d.\r\n", call_info_list.call_number);
                    if (0 == call_info_list.call_number) {
                        /* 1. stop voice */
                        aic_call_voice_stop();

                    } else {
                        alog_info("handleCallCallback,Release the second call, do nothing and return only.\r\n");
                        return -1;
                    }

                    /* 2.release call,
                       ui task:close call win */
                    break;
                }

                default:
                    break;
            }
            break;
        }
        case TS_EVENT_CALL_VOLTE_REG_STATUS_UPDATED:/* ims register state*/
        {
            ts_call_volte_reg_status_updated_t *p_volte_status = NULL;
            alog_info("handleCallCallback,TS_EVENT_CALL_VOLTE_REG_STATUS_UPDATED");
            p_volte_status = (ts_call_volte_reg_status_updated_t *)p_param;

            alog_info("handleCallCallback,volte status = %d", p_volte_status->volte_reg_status);
            if(true == p_volte_status->volte_reg_status) {
                /* 1.ims register success
                   UI task:refresh volte icon. */

            } else {
                /* 1.ims register fail
                   UI task:refresh volte icon. */

            }
            break;
        }

        case TS_EVENT_CALL_RINGBACK_VOICE: {/* play tone,maybe customed ringback by operator from network,maybe play local ringback settinged by user*/
            ts_call_ringback_voice_t *p_ringback_voice = NULL;

            p_ringback_voice = (ts_call_ringback_voice_t *)p_param;
            /* voice_type: 2-network ringback voice, 0-local ringback voice */
            /* voice_state: 1-start play, 0-stop play */
            alog_info("handleCallCallback,TS_EVENT_CALL_RINGBACK_VOICE arrived(voice_type = %d, voice_state = %d).\r\n",
                     p_ringback_voice->voice_type,
                     p_ringback_voice->voice_state);

            if (1 == p_ringback_voice->voice_state) {/* if voice_state is 1, start play ringback voice */
                /* TODO: open audio codec and start transform audio data to play ringback voice */
                aic_call_voice_start();

            } else {/* if voice_state is 0, no need to do things currently */
                alog_info("handleCallCallback,currently no need to do things for stop play ringback voice.\r\n");
            }
            break;
        }

        case TS_EVENT_CALL_SEND_DTMF_RESP: {
            ts_call_send_dtmf_resp_t *p_send_dtmf_resp = NULL;

            p_send_dtmf_resp = (ts_call_send_dtmf_resp_t *)p_param;
            alog_info("[%s][Info]p_send_dtmf_resp->ret_value = %d.", __func__, p_send_dtmf_resp->ret_value);
            break;
        }

        default:
            break;
    }
    return 0;
}

static int aic_power_on_cmd(const struct shell *shell, size_t argc, char **argv)
{
    alog_info("[%s],enter\r\n", __func__);
    aic_ctrl_poweron();
    return 0;
}
SHELL_CMD_ARG_REGISTER(aic_poweron, NULL, NULL, aic_power_on_cmd, 1,0);

static int aic_power_off_cmd(const struct shell *shell, size_t argc, char **argv)
{
    alog_info("[%s],enter\r\n", __func__);
    aic_ctrl_poweroff();
    return 0;
}
SHELL_CMD_ARG_REGISTER(aic_poweroff, NULL, NULL, aic_power_off_cmd, 1,0);

static int aic_reset_cmd(const struct shell *shell, size_t argc, char **argv)
{
    alog_info("[%s],enter\r\n", __func__);
    aic_ctrl_reset();
    return 0;
}
SHELL_CMD_ARG_REGISTER(aic_reset, NULL, NULL, aic_reset_cmd, 1,0);

static int aic_assert_cmd(const struct shell *shell, size_t argc, char **argv)
{
    alog_info("[%s],enter\r\n", __func__);
    aic_ctrl_assert();
    return 0;
}
SHELL_CMD_ARG_REGISTER(aic_assert, NULL, NULL, aic_assert_cmd, 1,0);

static int aic_radio_cmd(const struct shell *shell, size_t argc, char **argv)
{
    char *is_on = argv[1];
    alog_info("[%s],enter\r\n", __func__);

    if (0 == strcmp(is_on, "1")) {
        aic_srv_tele_radio_on(0, true, NULL);
        aic_srv_tele_radio_register(NULL, handleRadioCallback);
        aic_srv_tele_call_register(NULL, handleCallCallback);
    } else if (0 == strcmp(is_on, "0")) {
        aic_srv_tele_radio_on(0, false, NULL);
    }

    return 0;
}
SHELL_CMD_ARG_REGISTER(aic_radio, NULL, NULL, aic_radio_cmd, 2,0);

static int aic_set_log_level_cmd(const struct shell *shell, size_t argc, char **argv)
{
    alog_level_e level = ALOG_LEVEL_MAX;
    alog_info("[%s],enter\r\n", __func__);

    level = atoi(argv[1]);
    alog_set_level(level);

    return 0;
}
SHELL_CMD_ARG_REGISTER(aic_set_log_level, NULL, NULL, aic_set_log_level_cmd, 2,0);

static int aic_get_log_level_cmd(const struct shell *shell, size_t argc, char **argv)
{
    alog_level_e level = ALOG_LEVEL_MAX;
    alog_info("[%s],enter\r\n", __func__);

    level = alog_get_level();
    alog_info("[%s], the level is %d\n", __func__, level);

    return 0;
}
SHELL_CMD_ARG_REGISTER(aic_get_log_level, NULL, NULL, aic_get_log_level_cmd, 1,0);

static int aic_get_version_cmd(const struct shell *shell, size_t argc, char **argv)
{
    char sw_version[250] = {0};
    alog_info("[%s],enter\r\n", __func__);

    aic_get_version(sw_version, 250);
    alog_info("[%s],sw_version:%s\r\n", __func__, sw_version);

    return 0;

}
SHELL_CMD_ARG_REGISTER(aic_version, NULL, NULL, aic_get_version_cmd, 1,0);

static int aic_get_imei_cmd(const struct shell *shelll, size_t argc, char **argv)
{
    char imei_number[18] = {0};
    alog_info("[%s],enter\r\n", __func__);

    aic_srv_tele_radio_get_imei(0, imei_number);
    alog_info("[%s],IMEI:%s\r\n", __func__, imei_number);

    return 0;
}
SHELL_CMD_ARG_REGISTER(aic_get_imei, NULL, NULL, aic_get_imei_cmd, 1,0);

static int aic_check_cali_cmd(const struct shell *shelll, size_t argc, char **argv)
{
    bool is_cali = FALSE;
    alog_info("[%s],enter\r\n", __func__);

    is_cali = aic_srv_tele_radio_have_cali();
    alog_info("[%s],is_cali:%d\r\n", __func__, is_cali);

    return 0;
}
SHELL_CMD_ARG_REGISTER(aic_check_cali, NULL, NULL, aic_check_cali_cmd, 1,0);
static int32_t aic_get_sim_status_cmd(const struct shell *shelll, size_t argc, char **argv)
{
    ts_sim_status_e sim_status;
    alog_info("[%s],enter\r\n", __func__);

    aic_srv_tele_sim_get_status(0, &sim_status);
    alog_info("[%s],get sim status:%d\r\n", __func__, sim_status);

    return 0;
}
SHELL_CMD_ARG_REGISTER(aic_get_sim_status, NULL, NULL, aic_get_sim_status_cmd, 1,0);

static int32_t aic_get_sim_imsi_cmd(const struct shell *shelll, size_t argc, char **argv)
{
    char imsi[20] = {0};
    alog_info("[%s],enter\r\n", __func__);

    aic_srv_tele_sim_get_imsi(0, imsi);
    alog_info("[%s],get sim imsi:%s\r\n", __func__, imsi);

    return 0;
}
SHELL_CMD_ARG_REGISTER(aic_get_sim_imsi, NULL, NULL, aic_get_sim_imsi_cmd, 1,0);

static int32_t aic_get_sim_msisdn_cmd(const struct shell *shelll, size_t argc, char **argv)
{
    char msisdn[15] = {0};
    alog_info("[%s],enter\r\n", __func__);

    aic_srv_tele_sim_get_msisdn(0, msisdn);
    alog_info("[%s],get sim msisdn:%s\r\n", __func__, msisdn);

    return 0;
}
SHELL_CMD_ARG_REGISTER(aic_get_sim_msisdn, NULL, NULL, aic_get_sim_msisdn_cmd, 1,0);

static int32_t aic_get_signal_info_cmd(const struct shell *shelll, size_t argc, char **argv)
{
    ts_radio_signal_info_t signinfo;
    alog_info("[%s],enter\r\n", __func__);

    aic_srv_tele_radio_get_signal_info(0, &signinfo);
    alog_info("[%s]signal_type = %d, signal_strength = %d, signal_level = %d, signal_level_val = %d.\r\n",
              __func__,
              signinfo.signal_type,
              signinfo.signal_strength,
              signinfo.signal_level,
              signinfo.signal_level_val);

    return 0;
}
SHELL_CMD_ARG_REGISTER(aic_get_signal_info, NULL, NULL, aic_get_signal_info_cmd, 1,0);

static int32_t aic_get_cell_info_cmd(const struct shell *shelll, size_t argc, char **argv)
{
    alog_info("[%s],enter\r\n", __func__);

    aic_srv_tele_radio_get_cell_info(0, handleRadioCallback);

    return 0;
}
SHELL_CMD_ARG_REGISTER(aic_get_cell, NULL, NULL, aic_get_cell_info_cmd, 1,0);

static int32_t aic_get_neighbor_cell_info_list_cmd(const struct shell *shelll, size_t argc, char **argv)
{
    alog_info("[%s],enter\r\n", __func__);

    aic_srv_tele_radio_get_neighbor_cell_info_list(0, handleRadioCallback);

    return 0;
}
SHELL_CMD_ARG_REGISTER(aic_get_neighbor_cell, NULL, NULL, aic_get_neighbor_cell_info_list_cmd, 1,0);

static int32_t aic_get_reg_status_cmd(const struct shell *shelll, size_t argc, char **argv)
{
    ts_radio_reg_status_e reg_status;

    alog_info("[%s],enter\r\n", __func__);

    aic_srv_tele_radio_get_reg_status(0, &reg_status);
    alog_info("[%s]reg_status = %d.\r\n", __func__, reg_status);

    return 0;
}
SHELL_CMD_ARG_REGISTER(aic_get_reg_status, NULL, NULL, aic_get_reg_status_cmd, 1,0);

static int32_t aic_get_operator_info_cmd(const struct shell *shelll, size_t argc, char **argv)
{
    ts_radio_operator_info_t operator_info;

    alog_info("[%s],enter\r\n", __func__);

    aic_srv_tele_radio_get_operator_info(0, &operator_info);
    alog_info("[%s]operator_id = %d, operator_long_name = %s, operator_short_name = %s, plmn_mcc = %s, plmn_mnc = %s  \r\n",
              __func__,
              operator_info.operator_id,
              operator_info.operator_long_name,
              operator_info.operator_short_name,
              operator_info.plmn_mcc,
              operator_info.plmn_mnc);

    return 0;
}
SHELL_CMD_ARG_REGISTER(aic_get_operator, NULL, NULL, aic_get_operator_info_cmd, 1,0);

static int32_t aic_check_ims_reg_cmd(const struct shell *shelll, size_t argc, char **argv)
{
    bool is_ims_reg = false;

    alog_info("[%s],enter\r\n", __func__);

    is_ims_reg = aic_srv_tele_radio_have_ims_reg(0);
    alog_info("[%s]is_ims_reg = %d \r\n", __func__, is_ims_reg);

    return 0;
}
SHELL_CMD_ARG_REGISTER(aic_check_ims_reg, NULL, NULL, aic_check_ims_reg_cmd, 1,0);

static int32_t aic_get_pdp_info_cmd(const struct shell *shelll, size_t argc, char **argv)
{
    ts_data_pdp_info_t pdp_info;

    alog_info("[%s],enter\r\n", __func__);

    aic_srv_tele_data_get_internet_pdp_info(0, &pdp_info);
    alog_info("[%s]p_pdp_info  pdp status = %d, cid = %d, type = %s, apn_name = %s, gatway = %s, ipv4_addr = %s, ipv4_prim_dns = %s, ipv4_sec_dns = %s, ipv6_addr = %s, ipv6_prim_dns = %s, ipv6_sec_dns = %s.",
              __func__,
              pdp_info.pdp_status,
              pdp_info.link_info.cid,
              pdp_info.link_info.type,
              pdp_info.link_info.apn_name,
              pdp_info.link_info.gateway,
              pdp_info.link_info.ipv4_addr,
              pdp_info.link_info.ipv4_prim_dns,
              pdp_info.link_info.ipv4_sec_dns,
              pdp_info.link_info.ipv6_addr,
              pdp_info.link_info.ipv6_prim_dns,
              pdp_info.link_info.ipv6_sec_dns);

    return 0;
}
SHELL_CMD_ARG_REGISTER(aic_get_pdp_info, NULL, NULL, aic_get_pdp_info_cmd, 1,0);

static int32_t aic_radio_search_cmd(const struct shell *shelll, size_t argc, char **argv)
{
    alog_info("[%s],enter\r\n", __func__);

    aic_srv_tele_radio_start_search(0, handleRadioCallback);

    return 0;
}
SHELL_CMD_ARG_REGISTER(aic_radio_search, NULL, NULL, aic_radio_search_cmd, 1,0);

static int aic_dial_cmd(const struct shell *shell, size_t argc, char **argv)
{
    char *p_number = NULL;
    ts_call_dial_info_t dial_info = {0};
    alog_info("[%s],enter\r\n", __func__);

    p_number = argv[1];
    memcpy(dial_info.phone_number, p_number, strlen(p_number));
    int32_t ret = aic_srv_tele_call_dial(&dial_info, handleCallCallback);
    if (ret)
    {
        alog_info("aic_srv_tele_call_dial err:%d \n", ret);
    }
    return ret;
}
SHELL_CMD_ARG_REGISTER(aic_dial, NULL, NULL, aic_dial_cmd, 2,0);

static int aic_release_cmd(const struct shell *shell, size_t argc, char **argv)
{
    alog_info("[%s],enter\r\n", __func__);

    aic_srv_tele_call_release(0, 1, handleCallCallback);

    return 0;
}
SHELL_CMD_ARG_REGISTER(aic_call_release, NULL, NULL, aic_release_cmd, 1,0);

static int aic_call_answer_cmd(const struct shell *shell, size_t argc, char **argv)
{
    alog_info("[%s],enter\r\n", __func__);

    aic_srv_tele_call_answer(handleCallCallback);

    return 0;
}
SHELL_CMD_ARG_REGISTER(aic_call_answer, NULL, NULL, aic_call_answer_cmd, 1,0);

static int aic_set_auto_answer_cmd(const struct shell *shell, size_t argc, char **argv)
{
    alog_info("[%s],enter\r\n", __func__);

    if (0 == strcmp(argv[1], "1")) {
        aic_srv_tele_call_set_auto_answer(1);
    } else if (0 == strcmp(argv[1], "0")) {
        aic_srv_tele_call_set_auto_answer(0);
    }

    return 0;
}
SHELL_CMD_ARG_REGISTER(aic_set_auto_answer, NULL, NULL, aic_set_auto_answer_cmd, 2,0);

static int aic_get_auto_answer_cmd(const struct shell *shell, size_t argc, char **argv)
{
    bool is_auto_answer = false;
    alog_info("[%s],enter\r\n", __func__);

    is_auto_answer = aic_srv_tele_call_is_auto_answer();
    alog_info("[%s]is_auto_answer = %d \r\n", __func__, is_auto_answer);

    return 0;
}
SHELL_CMD_ARG_REGISTER(aic_get_auto_answer, NULL, NULL, aic_get_auto_answer_cmd, 1,0);

static int aic_check_volte_reg_cmd(const struct shell *shell, size_t argc, char **argv)
{
    bool is_volte_reg = false;
    is_volte_reg = aic_srv_tele_call_have_volte_reg(0);
    alog_info("[%s]is_volte_reg = %d \r\n", __func__, is_volte_reg);

    return 0;
}
SHELL_CMD_ARG_REGISTER(aic_check_volte_reg, NULL, NULL, aic_check_volte_reg_cmd, 1,0);

static int aic_set_voice_enable_loopback_cmd(const struct shell *shell, size_t argc, char **argv)
{
    alog_info("[%s],enter\r\n", __func__);

    if (0 == strcmp(argv[1], "1")) {
        aic_srv_voice_enable_loopback(1);
    } else if (0 == strcmp(argv[1], "0")) {
        aic_srv_voice_enable_loopback(0);
    }

    return 0;
}
SHELL_CMD_ARG_REGISTER(aic_set_voice_loopback, NULL, NULL, aic_set_voice_enable_loopback_cmd, 2, 0);

static int aic_get_voice_enable_loopback_cmd(const struct shell *shell, size_t argc, char **argv)
{
    bool is_voice_enable_loopback = false;

    alog_info("[%s],enter\r\n", __func__);

    is_voice_enable_loopback = aic_srv_voice_is_enable_loopback();
    alog_info("[%s]is_voice_enable_loopback = %d \r\n", __func__, is_voice_enable_loopback);

    return 0;
}
SHELL_CMD_ARG_REGISTER(aic_get_voice_loopback, NULL, NULL, aic_get_voice_enable_loopback_cmd, 1, 0);

static int aic_set_enable_volte_cmd(const struct shell *shell, size_t argc, char **argv)
{
    alog_info("[%s],enter\r\n", __func__);

    if (0 == strcmp(argv[1], "1")) {
        aic_srv_tele_call_volte_enable(0, true, handleCallCallback);
    } else if (0 == strcmp(argv[1], "0")) {
        aic_srv_tele_call_volte_enable(0, false, handleCallCallback);
    }

    return 0;
}
SHELL_CMD_ARG_REGISTER(aic_set_volte, NULL, NULL, aic_set_enable_volte_cmd, 2, 0);

static int aic_get_enable_volte_cmd(const struct shell *shell, size_t argc, char **argv)
{
    bool is_volte_enable = false;

    alog_info("[%s],enter\r\n", __func__);

    is_volte_enable = aic_srv_tele_call_is_volte_enable(0);
    alog_info("[%s] is_volte_enable = %d\r\n", __func__, is_volte_enable);

    return 0;
}
SHELL_CMD_ARG_REGISTER(aic_get_volte, NULL, NULL, aic_get_enable_volte_cmd, 1, 0);

static int32_t handleSmsCallback(void *p_param, uint32_t size)
{
    int32_t ret = -1;
    uint32_t event = TS_EVENT_SMS_MAX;

    if(NULL == p_param) {
        alog_error("[%s][Error]p_param is NULL.", __func__);
        return -1;
    }

    event = *(uint32_t *)p_param;
    alog_info("[%s][Info]event id is 0x%x.", __func__, event);
    switch(event) {
        case TS_EVENT_SMS_RECEIVE_NEW: /* new sms */
        {
            ts_sms_receive_new_t *p_ind = (ts_sms_receive_new_t *)p_param;

            alog_info("[%s][Info]TS_EVENT_SMS_RECEIVE_NEW, index:%d, total_count:%d, p_ind->is_full_in_sim = %d.\n", __func__, p_ind->index, p_ind->total_count, p_ind->is_full_in_sim);
            alog_info("[%s][Info]TS_EVENT_SMS_RECEIVE_NEW, sms_class(%d), ymd(%d-%d-%d), hms(%d:%d:%d), number(%s), content(%s).",
                      __func__,
                      p_ind->sms_class,
                      p_ind->time_stamp.year,
                      p_ind->time_stamp.month,
                      p_ind->time_stamp.day,
                      p_ind->time_stamp.hour,
                      p_ind->time_stamp.min,
                      p_ind->time_stamp.sec,
                      p_ind->number,
                      (char *)&p_ind->p_content);
            break;
        }

        case TS_EVENT_SMS_READ_IN_SIM_RESP:
            {
                ts_sms_read_in_sim_resp_t *p_sim = (ts_sms_read_in_sim_resp_t *)p_param;

                alog_info("[%s][Info]TS_EVENT_SMS_READ_IN_SIM_RESP, index:%d, textstart:0x%x \n", __func__, p_sim->index, (uint32_t)p_sim->p_content);
                if (p_sim->base_info.err_code == 0) {
                    alog_info("[%s][Info]index(%d), sms_class(%d), ymd(%d-%d-%d), hms(%d:%d:%d), number(%s), content(%s)",
                              __func__,
                              p_sim->index,
                              p_sim->sms_class,
                              p_sim->time_stamp.year,
                              p_sim->time_stamp.month,
                              p_sim->time_stamp.day,
                              p_sim->time_stamp.hour,
                              p_sim->time_stamp.min,
                              p_sim->time_stamp.sec,
                              p_sim->number,
                              (char *)&p_sim->p_content);
                }
            }
            break;

        case TS_EVENT_SMS_GET_SMSC_RESP:
            {
                ts_sms_get_smsc_resp_t* p_sms_smsc = (ts_sms_get_smsc_resp_t*)(p_param);

                alog_info("[%s][Info]TS_EVENT_SMS_GET_SMSC_RESP, p_sms_smsc->smsc is %s. \n", __func__, p_sms_smsc->smsc);
            }
            break;

        case TS_EVENT_SMS_SEND_RESP:
            {
                ts_sms_send_resp_t* p_sms_send = (ts_sms_send_resp_t*)(p_param);

                alog_info("[%s][Info]TS_EVENT_SMS_SEND_RESP, p_sms_send->base_info.err_code = %d.", __func__, p_sms_send->base_info.err_code);
                if (p_sms_send->base_info.err_code == 0){/* sms send successfully */
                    alog_info("[%s][Info]TS_EVENT_SMS_SEND_RESP, p_sms_send->index = %d.", __func__, p_sms_send->index);
                }
            }
            break;

        case TS_EVENT_SMS_DEL_IN_SIM_RESP:
            {
                ts_sms_del_in_sim_resp_t *p_sms_del = (ts_sms_del_in_sim_resp_t *)p_param;

                alog_info("[%s][Info]TS_EVENT_SMS_DEL_IN_SIM_RESP, p_sms_del->base_info.err_code = %d.", __func__, p_sms_del->base_info.err_code);
            }
            break;

        case TS_EVENT_SMS_LOAD_IN_SIM_RESP: {
            ts_sms_load_in_sim_resp_t *p_load_sms = (ts_sms_load_in_sim_resp_t *)p_param;
            uint16_t index = 0;

            alog_info("[%s]TS_EVENT_SMS_LOAD_IN_SIM_RESP, card_id = %d, err_code = %d, is_full_in_sim = %d, total_count = %d, count_in_sim = %d, have_reported_count = %d, cur_reported_count = %d, index_start = %d.",
                      __func__,
                      p_load_sms->base_info.card_id,
                      p_load_sms->base_info.err_code,
                      p_load_sms->is_full_in_sim,
                      p_load_sms->total_count,
                      p_load_sms->count_in_sim,
                      p_load_sms->have_reported_count,
                      p_load_sms->cur_reported_count,
                      p_load_sms->index_start);
            if (0 == p_load_sms->base_info.err_code) {
                /* load sim sms success. */
                if (true == p_load_sms->is_full_in_sim) {
                    /* If the sms in sim is full, user could do something here.(such as pop up some UI tips or delete some messages.) */
                }
                /* when user receive this response event, call this api to get sms content */
                for (index = p_load_sms->index_start; index <= p_load_sms->total_count; index++) {
                    ret = aic_srv_tele_sms_read_in_sim(p_load_sms->base_info.card_id, index, handleSmsCallback);
                    alog_info("[%s]aic_srv_tele_sms_read_in_sim[index = %d] return %d.", __func__, index, ret);
                }
            } else {
                /* load sim sms failed, and user could pop up some UI tips. */
            }
            break;
        }

        default:
            break;
    }

    return 0;
}
static int aic_sms_register_cmd(const struct shell *shell, size_t argc, char **argv)
{
    alog_info("[%s],enter\r\n", __func__);

    aic_srv_tele_sms_register(NULL, handleSmsCallback);

    return 0;
}
SHELL_CMD_ARG_REGISTER(aic_sms_register, NULL, NULL, aic_sms_register_cmd, 1,0);

static int aic_sms_send_cmd(const struct shell *shell, size_t argc, char **argv)
{
    char *p_number= NULL;
    alog_info("[%s],enter\r\n", __func__);
    p_number = argv[1];

    const ts_sms_send_info_t sms_send_info = {
        .p_smsc = NULL,
        .p_number = p_number,
        .p_text = "hello world",
        .is_write_to_sim = false,
    };
    aic_srv_tele_sms_send(0, &sms_send_info, handleSmsCallback);

    return 0;
}
SHELL_CMD_ARG_REGISTER(aic_sms, NULL, NULL, aic_sms_send_cmd, 2,0);

static int aic_sms_del_cmd(const struct shell *shell, size_t argc, char **argv)
{
    alog_info("[%s],enter\r\n", __func__);

    int index[1] = {0};
    index[0] = atoi(argv[1]);
    alog_info("[%s] delete sms(%d)", __func__, index[0]);
    aic_srv_tele_sms_del_in_sim(0, index, 1, handleSmsCallback);

    return 0;
}
SHELL_CMD_ARG_REGISTER(aic_sms_del, NULL, NULL, aic_sms_del_cmd, 2,0);

static int aic_sms_load_cmd(const struct shell *shell, size_t argc, char **argv)
{
    alog_info("[%s],enter\r\n", __func__);

    aic_srv_tele_sms_load_in_sim(0, handleSmsCallback);

    return 0;
}
SHELL_CMD_ARG_REGISTER(aic_sms_load, NULL, NULL, aic_sms_load_cmd, 1,0);

static int aic_sms_get_smsc_cmd(const struct shell *shell, size_t argc, char **argv)
{
    alog_info("[%s],enter\r\n", __func__);

    aic_srv_tele_sms_get_smsc(0, handleSmsCallback);

    return 0;
}
SHELL_CMD_ARG_REGISTER(aic_get_smsc, NULL, NULL, aic_sms_get_smsc_cmd, 1,0);
/*Wifi Scan test start*/
static int32_t wfs_scan_result_handler(void *p_param, uint32_t size)
{
    uint32_t i = 0;
    wfs_scan_result_t *p_wfs_result = NULL;
    uint32_t total_ap_num = 0;
    alog_info("[%s],enter\r\n", __func__);

    p_wfs_result = (wfs_scan_result_t *)p_param;
    total_ap_num = p_wfs_result->total_ap_num;

    if (-1 == p_wfs_result->ret_value) {
        alog_error("[%s]wfs_scan fail!", __func__);
        return -1;
    }

    if ((NULL == p_wfs_result->p_ap_array) || (0 == total_ap_num)) {
        alog_error("[%s]wfs_scan success but scan 0 ap info!", __func__);
        return -1;
    }

    for (i = 0; i < total_ap_num; i++ ) {
        /* package scan result at string */
        /* urc eg. +WIFISCAN:1,1,1A:2B:3C:4D:5E:6F,-30,-,-*/
        alog_info("[%s] channel (%d), mac (%s), rssi (%d)", __func__, p_wfs_result->p_ap_array[i].channel,p_wfs_result->p_ap_array[i].mac,p_wfs_result->p_ap_array[i].rssi);
    }

    return 0;
}

static int32_t wfs_scan_stop_handler(void *p_param, uint32_t size)
{
    int32_t ret_value = 0;
    wfs_scan_stop_resp_t *p_wfs_stop = NULL;

    p_wfs_stop = (wfs_scan_stop_resp_t *)p_param;
    ret_value = p_wfs_stop->ret_value;

    alog_info("[%s] ret_value (%d)", __func__, ret_value);

    return ret_value;
}

static int aic_start_wifi_scan(const struct shell *shell, size_t argc, char **argv)
{
    wfs_scan_config_t wfs_cfg = {0};
    alog_info("[%s],enter \r\n", __func__);

    wfs_cfg.scan_channel[0] = atoi(argv[1]);
    if(wfs_cfg.scan_channel[0] > 13) {
        alog_error("[%s] the input param of channel (%d) is error, return directly!", __func__, wfs_cfg.scan_channel[0]);
        return -1;
    }

    wfs_cfg.time_out = atoi(argv[2]);
    if(wfs_cfg.time_out < 10) {
        alog_warn("[%s] the input param of timeout (%d) is below of range, modify it to 10!", __func__, wfs_cfg.time_out);
        wfs_cfg.time_out = 10;
    } else if(wfs_cfg.time_out > 60) {
        alog_warn("[%s] the input param of timeout (%d) is over of range, modify it to 60!", __func__, wfs_cfg.time_out);
        wfs_cfg.time_out = 60;
    }

    wfs_cfg.round = atoi(argv[3]);
    if(wfs_cfg.round > 3) {
        alog_warn("[%s] the input param of round (%d) is over of range, modify it to 3!", __func__, wfs_cfg.round);
        wfs_cfg.round = 3;
    }

    wfs_cfg.max_ap_num = atoi(argv[4]);
    if(wfs_cfg.max_ap_num > 100) {
        alog_warn("[%s] the input param of max_ap_num (%d) is over of range, modify it to 100!", __func__, wfs_cfg.max_ap_num);
        wfs_cfg.max_ap_num = 100;
    }

    alog_debug("[%s] max_ap_num is %d, round is %d,timeout is %d, channel is %d", __func__,
         wfs_cfg.max_ap_num, wfs_cfg.round, wfs_cfg.time_out, wfs_cfg.scan_channel[0]);

    aic_srv_wifi_scan_start(&wfs_cfg, (srv_func_callback)wfs_scan_result_handler);

    return 0;
}
SHELL_CMD_ARG_REGISTER(aic_wfs_start, NULL, NULL, aic_start_wifi_scan, 5,0);

static int aic_stop_wifi_scan(const struct shell *shell, size_t argc, char **argv)
{
    alog_info("[%s],enter\r\n", __func__);
    aic_srv_wifi_scan_stop((srv_func_callback)wfs_scan_stop_handler);

    return 0;
}
SHELL_CMD_ARG_REGISTER(aic_wfs_stop, NULL, NULL, aic_stop_wifi_scan, 1,0);
/*Wifi Scan test end*/
