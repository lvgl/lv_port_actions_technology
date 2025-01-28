/******************************************************************************/
/*                                                                            */
/*    Copyright 2023 by AICXTEK TECHNOLOGIES CO.,LTD. All rights reserved.    */
/*                                                                            */
/******************************************************************************/

/**
 *  DESCRIPTION
 *
 *    This file declares the interfaces of SMS in telephony service.
 *    SMS service is managed to maintain SMS list, including concatecated cases, indexes, encoding and
 * decoding SMS protocol data units(PDU). It is based on tele sms with pdu service.
 */
#ifndef __AIC_SRV_TELE_SMS_H__
#define __AIC_SRV_TELE_SMS_H__

#ifdef __cplusplus
extern "C" {
#endif

/**********************
 * INCLUDES
 **********************/
#include "aic_srv_bus.h"
#include <aic_type.h>

/*********************
 *      DEFINES
 *********************/
#define TS_SMS_INVALID_HANDLE (-1)
/*10-octet BCD plus +*/
#define TS_SMS_NUM_MAX_LEN (21)
#define TS_SMS_PDU_MAX_LEN (176 * 2)

/**********************
 *      TYPEDEFS
 **********************/
typedef enum {
    TS_EVENT_SMS_WITH_PDU_BASE = (0x07 << 16),
    /* responses */
    TS_EVENT_SMS_SEND_WITH_PDU_RESP,
    TS_EVENT_SMS_SEND_ACK_WITH_PDU_RESP,
    TS_EVENT_SMS_WRITE_WITH_PDU_TO_SIM_RESP,
    TS_EVENT_SMS_DEL_IN_SIM_WITH_PDU_RESP,
    TS_EVENT_SMS_SET_SMSC_WITH_PDU_RESP,
    TS_EVENT_SMS_GET_SMSC_WITH_PDU_RESP,
    TS_EVENT_SMS_GET_LIST_IN_SIM_WITH_PDU_RESP,
    TS_EVENT_SMS_READ_IN_SIM_WITH_PDU_RESP,
    TS_EVENT_SMS_GET_STOR_STAT_IN_SIM_WITH_PDU_RESP,

    /* indicator */
    TS_EVENT_SMS_IND_WITH_PDU_START = (TS_EVENT_SMS_WITH_PDU_BASE + 100),
    /* NEW SMS ARRIVAL - type: ts_sms_new_with_idx_ind_t */
    TS_EVENT_SMS_NEW_WITH_IDX_IND,
    /* NEW SMS STATUS REPORT - type: ts_sms_status_report_with_pdu_ind_t */
    TS_EVENT_SMS_STATUS_REPORT_WITH_PDU_IND,
    /* SMS STORAGE STATUS CHANGED - type: ts_sms_storage_status_with_pdu_ind_t */
    TS_EVENT_SMS_STORAGE_STATUS_WITH_PDU_IND,
    /* NEW SMS ARRIVAL WITH PDU - type: ts_sms_new_with_pdu_ind_t */
    TS_EVENT_SMS_NEW_WITH_PDU_IND,

    TS_EVENT_SMS_WITH_PDU_MAX
} ts_event_sms_with_pdu_e;

/**
 * enum ts_sms_rec_status_e - sms record status
 * Note: the value must align with <stat> in 3GPP TS 27.005
 * @TS_SMS_REC_STATUS_UNREAD: unread record
 * @TS_SMS_REC_STATUS_READ: read record
 * @TS_SMS_REC_STATUS_UNSENT: unsent record
 * @TS_SMS_REC_STATUS_SENT: sent record
 * @TS_SMS_REC_STATUS_ALL: all records
 */
typedef enum {
    TS_SMS_REC_STATUS_UNREAD = 0,
    TS_SMS_REC_STATUS_READ = 1,
    TS_SMS_REC_STATUS_UNSENT = 2,
    TS_SMS_REC_STATUS_SENT = 3,
    TS_SMS_REC_STATUS_ALL = 4,
} ts_sms_rec_status_e;

typedef enum {
    TS_EVENT_SMS_BASE = TS_EVENT_SMS_WITH_PDU_MAX + 100,
    /* responses */
    TS_EVENT_SMS_SEND_RESP,
    TS_EVENT_SMS_DEL_IN_SIM_RESP,
    TS_EVENT_SMS_SET_SMSC_RESP,
    TS_EVENT_SMS_GET_SMSC_RESP,
    TS_EVENT_SMS_GET_LIST_IN_SIM_RESP,
    TS_EVENT_SMS_READ_IN_SIM_RESP,
    TS_EVENT_SMS_LOAD_IN_SIM_RESP,

    /* receive new sms - type: ts_sms_receive_new_t */
    TS_EVENT_SMS_RECEIVE_NEW,
    /* the status of write sms to sim(for send, receive and draft, etc.) - type: ts_sms_write_in_sim_status_updated_t */
    TS_EVENT_SMS_WRITE_IN_SIM_STATUS_UPDATED,
    /* NEW SMS STATUS REPORT - type: ts_sms_status_report_ind_t */
    TS_EVENT_SMS_STATUS_REPORT_IND,
    TS_EVENT_SMS_SIM_FULL,
    TS_EVENT_SMS_MAX
} ts_event_sms_e;

/**
 * enum - sms class. refer to 3GPP TS 23.038
 * @TS_SMS_CLASS_0: class 0
 * @TS_SMS_CLASS_1: class 1
 * @TS_SMS_CLASS_2: class 2
 * @TS_SMS_CLASS_3: class 3
 * @TS_SMS_CLASS_NO: no message class
 */
typedef enum {
    TS_SMS_CLASS_0 = 0,
    TS_SMS_CLASS_1,
    TS_SMS_CLASS_2,
    TS_SMS_CLASS_3,
    TS_SMS_CLASS_NO,
} ts_sms_class_e;

typedef enum
{
    TS_SMS_TYPE_SEND,
    TS_SMS_TYPE_RECEIVE,
    TS_SMS_TYPE_DRAFT,
    TS_SMS_TYPE_ALL,

    TS_SMS_TYPE_MAX
}ts_sms_type_e;

/**
 * struct ts_sms_time_stamp_t - SMS time stamp type descriptor
 * @year: year
 * @month: month: 1~12
 * @day: day: 1~31
 * @hour: hour: 0~23
 * @min: minute: 0~59
 * @sec: second: 0~59
 */
typedef struct {
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t min;
    uint8_t sec;
} ts_sms_time_stamp_t;

/**
 * struct ts_sms_entry_t - sms record entry
 * @index: index
 * @status: status of the item
 */
typedef struct {
    int32_t index;
    ts_sms_rec_status_e status;
} ts_sms_entry_t;

/**
 * struct ts_sms_send_info_t - info of sms sending
 * @p_smsc: sms center, use "" or NULL to set smsc as default one
 * @p_number: recipient number
 * @p_text: sms text to send, encoded as UTF8
 * @is_write_to_sim: set true to write the sms to SIM
 */
typedef struct {
    const char *p_smsc;
    const char *p_number;
    const char *p_text;
    bool is_write_to_sim;
} ts_sms_send_info_t;

/**
 * struct ts_sms_get_list_info_t
 * @card_id: sim slot
 * @sms_type: sms type
 */
typedef struct {
    uint8_t card_id;
    ts_sms_type_e sms_type;
}ts_sms_get_list_info_t;

/**
 * struct ts_sms_general_resp_t - general response of sms operation
 * @event_id: event id, refer to ts_event_sms_e
 * @card_id: sim slot
 * @err_code: error code. 0 for success, otherwise refers to HRilErrNumber
 */
typedef struct {
    ts_event_sms_e event_id;
    uint8_t card_id;
    int32_t err_code;
} ts_sms_general_resp_t;
/**
 * struct ts_sms_send_resp_t - response of sms sending
 * @base_info: basic response info
 * @index: index of the pdu written into SIM. -1 means not saved.
 */
typedef struct {
    ts_sms_general_resp_t base_info;
    int32_t index;
} ts_sms_send_resp_t;

/**
 * struct ts_sms_del_in_sim_resp_t - response of sms deleting
 * @base_info: basic response info
 */
typedef struct {
    ts_sms_general_resp_t base_info;
} ts_sms_del_in_sim_resp_t;

/**
 * struct ts_sms_set_smsc_resp_t - response of sms setting smsc
 * @base_info: basic response info
 */
typedef struct {
    ts_sms_general_resp_t base_info;
} ts_sms_set_smsc_resp_t;

/**
 * struct ts_sms_get_smsc_resp_t - response of sms getting smsc
 * @base_info: basic response info
 * @smsc: smsc of string type
 */
typedef struct {
    ts_sms_general_resp_t base_info;
    char smsc[TS_SMS_NUM_MAX_LEN + 1];
} ts_sms_get_smsc_resp_t;

/**
 * struct ts_sms_get_list_in_sim_resp_t - response of sms getting entry list in sim
 * @base_info: basic response info
 * @box: box type
 * @count: total count of info list
 * @list_start: place holder of list start
 * Note: the size of ts_sms_get_list_in_sim_resp_t is variable.
 * Suggestion to use:
 * ts_sms_get_list_in_sim_resp_t *resp = (ts_sms_get_list_in_sim_resp_t *)p_param;
 * ts_sms_entry_t *list = (ts_sms_entry_t *)(&resp->list_start);
 * for (uint16_t i = 0; i < resp->count; i++) {
 *    // try to use list[i] as each item
 * }
 */
typedef struct {
    ts_sms_general_resp_t base_info;
    ts_sms_type_e box;
    uint16_t count;
    uint32_t list_start;
} ts_sms_get_list_in_sim_resp_t;

/**
 * struct ts_sms_read_in_sim_resp_t - response of sms reading entry in sim
 * @base_info: basic response info
 * @index: index
 * @sms_class: sms class
 * @number: recipient(submit) / origination(deliver) number
 * @time_stamp: timestamp of the sms
 * @p_content: the sms content pointer, encoded as UTF8, ended with '\0'
 * Note: the size of ts_sms_read_in_sim_resp_t is variable.
 * Suggestion to use:
 * ts_sms_read_in_sim_resp_t *resp = (ts_sms_read_in_sim_resp_t *)p_param;
 * char *text = (char *)(&resp->p_content);
 */
typedef struct {
    ts_sms_general_resp_t base_info;
    int32_t index;
    ts_sms_class_e sms_class;
    char number[TS_SMS_NUM_MAX_LEN + 1];
    ts_sms_time_stamp_t time_stamp;
    char *p_content;
} ts_sms_read_in_sim_resp_t;

/**
 * struct ts_sms_load_in_sim_resp_t - response of load from sim
 * @base_info: basic response info
 *             err_code: 0 - success, -1 - sim is not present, -2 - sim is lock
 * @is_full_in_sim: true for full in sim, false for not full in sim
 * @total_count: the sms total count for client.
 * @count_in_sim: the sms count in sim.
 * @have_reported_count: the sms count have reported to client.
 * @cur_reported_count: the sms count currently reported to client except for the reported sms count.
 * @index_start: the sms index start value for current reported(from index_start to total_count.
 */
typedef struct {
    ts_sms_general_resp_t base_info;
    bool is_full_in_sim;
    uint16_t total_count;
    uint16_t count_in_sim;
    uint16_t have_reported_count;
    uint16_t cur_reported_count;
    uint32_t index_start;
}ts_sms_load_in_sim_resp_t;

/**
 * struct ts_sms_receive_new_t
 * @event_id: event id
 * @card_id: sim slot
 * @index: index (not record number in EFsms)
 * @total_count: the total count(including in sim and not in sim)
 * @is_full_in_sim: true for full in sim, false for not full in sim
 * @sms_class: sms class
 * @number: recipient(submit) / origination(deliver) number
 * @time_stamp: timestamp of the sms
 * @p_content: the sms content pointer, encoded as UTF8, ended with '\0'
 */
typedef struct {
    ts_event_sms_e event_id;
    uint8_t card_id;
    uint32_t index;
    uint16_t total_count;
    bool is_full_in_sim;
    ts_sms_class_e sms_class;
    char number[TS_SMS_NUM_MAX_LEN + 1];
    ts_sms_time_stamp_t time_stamp;
    char *p_content;
} ts_sms_receive_new_t;

/**
 * struct ts_sms_status_report_ind_t - indicator of sms status report
 * @event_id: event id, i.e. TS_EVENT_SMS_STATUS_REPORT_IND
 * @card_id: sim slot
 * @tp_st: refer to 3GPP TS 23.040 9.2.3.15 TP-Status (TP-ST)
 * @number: recipient(submit) / origination(deliver) number
 * @time_stamp: timestamp of the sms
 * @p_content: the sms content pointer, encoded as UTF8, ended with '\0'
 * Note: the size of ts_sms_read_in_sim_resp_t is variable.
 * Suggestion to use:
 * ts_sms_read_in_sim_resp_t *resp = (ts_sms_read_in_sim_resp_t *)p_param;
 * char *text = (char *)(&resp->p_content);
 */
typedef struct {
    ts_event_sms_e event_id;
    uint8_t card_id;
    uint8_t tp_st;
    char number[TS_SMS_NUM_MAX_LEN + 1];
    ts_sms_time_stamp_t time_stamp;
    char *p_content;
} ts_sms_status_report_ind_t;

/**
 * struct ts_sms_storage_status_ind_t - indicator of sms sim storage status
 * @event_id: event id, i.e. TS_EVENT_SMS_WRITE_IN_SIM_STATUS_UPDATED
 * @card_id: sim slot
 * @index: index (not record number in EFsms)
 * @type: the from type for write to sim
 * @write_result: the result value(0: write success, -1: response fail, -2: other fail value)
 * @is_full_in_sim: true for full in sim, false for not full in sim
 * @is_ready: true for ready, false for not exception(e.g. SIM is removed or PUK triggerred)
 * @count_in_sim: the sms count in sim.
 */
typedef struct {
    ts_event_sms_e event_id;
    uint8_t card_id;
    uint32_t index;
    ts_sms_type_e type;
    /* 0: write success; -1: write response fail; -2: return fail for call aic_srv_tele_sms_write_with_pdu_to_sim. */
    int32_t write_result;
    bool is_full_in_sim;
    uint16_t count_in_sim;
} ts_sms_write_in_sim_status_updated_t;

typedef struct{
    ts_event_sms_e event_id;
    uint8_t card_id;
    uint16_t count_in_sim;
}ts_sms_sim_full_t;

/**********************
 * GLOBAL PROTOTYPES
 **********************/
/**
 * aic_srv_tele_sms_load_in_sim
 * @card_id: [IN] sim slot
 * @_response_callback: [IN] callback to receive response
 * Remarks
 *   response event: TS_EVENT_SMS_LOAD_IN_SIM_RESP
 *   response struct: ts_sms_load_in_sim_resp_t
 *
 * Return: 0 if success, otherwise -1
 */
int32_t aic_srv_tele_sms_load_in_sim(uint8_t card_id, srv_func_callback _response_callback);

/**
 * aic_srv_tele_sms_register - register a new client to recevie notification from SMS service
 * @p_bus_info: [IN] client information. Reserved for future use
 * @_srv_callback: [IN] client callback
 *
 * Return: client handle if success, TS_SMS_INVALID_HANDLE for failure
 * Remarks
 *   _srv_callback will be invoked once any of the indication events listed in ts_event_sms_e occurs.
 */
int32_t aic_srv_tele_sms_register(aic_srv_bus_info_t *p_bus_info, srv_func_callback _srv_callback);

/**
 * aic_srv_tele_sms_unregister - deregister a client
 * @client_handle: [IN] client handle
 *
 * Return: 0 if success, otherwise -1
 */
int32_t aic_srv_tele_sms_unregister(int32_t client_handle);

/**
 * aic_srv_tele_sms_send - send a sms
 * @card_id: [IN] sim slot. Reserved for future use
 * @p_sms_info: [IN] pointer to sms info
 * @_response_callback: [IN] callback to receive response
 * Return: 0 if success, otherwise -1
 * Remarks
 *   response event: TS_EVENT_SMS_SEND_RESP
 *   response struct: ts_sms_send_resp_t
 *   editor note: Multi-SIM is not supported yet.
 */
int32_t aic_srv_tele_sms_send(uint8_t card_id, const ts_sms_send_info_t *p_sms_info,
                                 srv_func_callback _response_callback);

/**
 * aic_srv_tele_sms_del_in_sim - delete sms in sim
 * @card_id: [IN] sim slot. Reserved for future use
 * @p_indexes: [IN] pointer to an array of indexes
 * @count: [IN] count of indexes
 * @_response_callback: [IN] callback to receive response
 *
 * Return: 0 if success, otherwise -1
 * Remarks
 *   response event: TS_EVENT_SMS_DEL_IN_SIM_RESP
 *   response struct: ts_sms_del_in_sim_resp_t
 *   editor note: Multi-SIM is not supported yet.
 */
int32_t aic_srv_tele_sms_del_in_sim(uint8_t card_id, const int32_t *p_indexes, int32_t count,
                                       srv_func_callback _response_callback);

/**
 * aic_srv_tele_sms_set_smsc - set sms center address
 * @card_id: [IN] sim slot. Reserved for future use
 * @p_address: [IN] smsc of string type
 * @_response_callback: [IN] callback to receive response
 *
 * Return: 0 if success, otherwise -1
 * Remarks
 *   response event: TS_EVENT_SMS_SET_SMSC_RESP
 *   response struct: ts_sms_set_smsc_resp_t
 *   editor note: Multi-SIM is not supported yet.
 */
int32_t aic_srv_tele_sms_set_smsc(uint8_t card_id, const char *p_address, srv_func_callback _response_callback);

/**
 * aic_srv_tele_sms_get_smsc - get sms center address
 * @card_id: [IN] sim slot. Reserved for future use
 * @_response_callback: [IN] callback to receive response
 *
 * Return: 0 if success, otherwise -1
 * Remarks
 *   response event: TS_EVENT_SMS_GET_SMSC_RESP
 *   response struct: ts_sms_get_smsc_resp_t
 *   editor note: Multi-SIM is not supported yet.
 */
int32_t aic_srv_tele_sms_get_smsc(uint8_t card_id, srv_func_callback _response_callback);

/**
 * aic_srv_tele_sms_get_list_in_sim_with_pdu - get sms pdu list of specified box in sim
 * @p_get_list_info: [IN] pointer to get list info
 * @_response_callback: [IN] callback to receive response
 *
 * Return: 0 if success, otherwise -1
 * Remarks
 *   response event: TS_EVENT_SMS_GET_LIST_IN_SIM_RESP
 *   response struct: ts_sms_get_list_in_sim_resp_t
 *   editor note: Multi-SIM is not supported yet.
 */
int32_t aic_srv_tele_sms_get_list_in_sim(ts_sms_get_list_info_t *p_get_list_info,
                                            srv_func_callback _response_callback);

/**
 * aic_srv_tele_sms_read_in_sim - read an sms from sim
 * @card_id: [IN] sim slot. Reserved for future use
 * @index: [IN] index of the record in SIM
 * @_response_callback: [IN] callback to receive response
 *
 * Return: 0 if success, otherwise -1
 * Remarks
 *   response event: TS_EVENT_SMS_READ_IN_SIM_RESP
 *   response struct: ts_sms_read_in_sim_resp_t
 *   editor note: Multi-SIM is not supported yet.
 */
int32_t aic_srv_tele_sms_read_in_sim(uint8_t card_id, uint32_t index, srv_func_callback _response_callback);

/**
 * aic_srv_tele_sms_enable_status_report - enable status report
 * @card_id: [IN] sim slot. Reserved for future use
 * @enable: [IN] true to enable, false to disable
 *
 * Return: 0 if success, otherwise -1
 * Remarks
 *   editor note: Multi-SIM is not supported yet
 *   1. this interface is addressed to switch on/off the notifiation upon status report received rather than to enable
 * status report in sms sending
 */
int32_t aic_srv_tele_sms_enable_status_report(uint8_t card_id, bool enable);

/**
 * aic_srv_tele_sms_set_valid_period - set valid period
 * @card_id: [IN] sim slot. Reserved for future use
 * @is_present: [IN] tp-vp is present or not
 * @tp_vp: [IN] value options refer to 3GPP TS 23.040 9.2.3.12.1 TP-VP (Relative format)
 *
 * Return: 0 if success, otherwise -1
 * Remarks
 *   editor note: Multi-SIM is not supported yet
 */
int32_t aic_srv_tele_sms_set_valid_period(uint8_t card_id, bool is_present, uint8_t tp_vp);
#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /* __AIC_SRV_TELE_SMS_H__ */
