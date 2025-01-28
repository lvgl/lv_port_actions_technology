#ifndef _LAUNCHER_UI_SIM_H_
#define _LAUNCHER_UI_SIM_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <app_ui.h>
#include "aic_srv_version.h"
#include "aic_srv_tele_radio.h"
#include "aic_srv_tele_sim.h"
#include "aic_srv_tele_data.h"
#include "aic_srv_tele_call.h"
#include "aic_srv_tele_sms.h"
#include "aic_srv_voice.h"
#include "aic_network_speed.h"
#include "aic_ctrl.h"
typedef enum {
    CALL_DIALING_TYPE,
    CALL_MESSAGES_TYPE,
}call_dial_e;

typedef struct ui_sim_data {
    call_dial_e dial_num_type;
}ui_sim_data_t;

extern ui_sim_data_t sim_ui_data;

typedef struct ui_sim_presenter {
    int32_t (*radio_on)(uint8_t card_id, bool radio_on, srv_func_callback _response_callback);
    int32_t (*radio_register)(aic_srv_bus_info_t *p_bus_info, srv_func_callback _srv_callback);
    int32_t (*call_register)(aic_srv_bus_info_t *p_bus_info, srv_func_callback _srv_callback);
	void (*get_version)(char *buf, size_t size);    //软件版本号
    int32_t (*get_imei)(uint8_t card_id, char *p_imei); //IMEI设备识别码
    bool (*radio_have_cali)(void);  //是否已校准
	int32_t (*sim_get_status)(uint8_t card_id, ts_sim_status_e *p_sim_status);  //Sim是否在位
    int32_t (*sim_get_imsi)(uint8_t card_id, char *p_sim_imsi); //Sim imsi
    int32_t (*sim_get_msisdn)(uint8_t card_id, char *p_sim_msisdn); //Sim 本机号码
    int32_t (*get_signal_info)(uint8_t card_id, ts_radio_signal_info_t *p_signal_info); //信号强度
    int32_t (*get_cell_info)(uint8_t card_id, srv_func_callback _response_callback);    //当区信息
    int32_t (*get_neighbor_cell_info_list)(uint8_t card_id, srv_func_callback _response_callback);  //邻区信息
    int32_t (*get_reg_status)(uint8_t card_id, ts_radio_reg_status_e *p_reg_status);    //是否驻网
    int32_t (*get_operator_info)(uint8_t card_id, ts_radio_operator_info_t *p_operator_info);   //运营商名称
    bool (*have_ims_reg)(uint8_t card_id);  //是否IMS(VoLTE)驻网
    int32_t (*get_internet_pdp_info)(uint8_t card_id, ts_data_pdp_info_t *p_pdp_info);  //是否PDP驻网，显示当前IP地址
    int32_t (*radio_start_search)(uint8_t card_id, srv_func_callback _response_callbak);    //手动搜网，并显示可选网络
    int (*get_network_dl_speed)(aic_network_speed_callback callback);
    int (*get_network_ul_speed)(aic_network_speed_callback callback);
    int32_t (*set_auto_answer)(bool is_auto);   //自动接听开关设置
    bool (*get_auto_answer)(void);  //自动接听开关获取
    bool (*call_have_volte_reg)(uint8_t card_id);   //是否IMS(VoLTE)驻网
    int32_t (*set_voice_enable_loopback)(bool enable);  //设置电话语音Loopback
    bool (*get_voice_enable_loopback)(void);    //获取电话语音Loopback
    int32_t (*set_enable_volte)(uint8_t card_id, bool enable, srv_func_callback _response_callback);    //设置VoLTE开关
    bool (*get_enable_volte)(uint8_t card_id);    //获取VoLTE开关状态
    int32_t (*tele_call_dail)(ts_call_dial_info_t *p_dial_info, srv_func_callback _response_callback);  //拨打电话
    int32_t (*tele_sms_register)(aic_srv_bus_info_t *p_bus_info, srv_func_callback _srv_callback);  //收单条短信
    int32_t (*tele_sms_send)(uint8_t card_id, const ts_sms_send_info_t *p_sms_info, srv_func_callback _response_callback);  //发单条短信
    int32_t (*tele_sms_load_list)(uint8_t card_id, srv_func_callback _response_callback);  //加载SIM短信列表
    int32_t (*tele_sms_get_smsc)(uint8_t card_id, srv_func_callback _response_callback);    //显示短信中心号码
    int (*ctrl_power_on)(bool on);
    int (*ctrl_reset)(void);
    int (*ctrl_assert)(void);
    int32_t (*set_log_level)(alog_level_e log_level);
    alog_level_e (*get_log_level)(void);
} ui_sim_presenter_t;

extern const ui_sim_presenter_t sim_data;

#ifdef __cplusplus
}
#endif
#endif
