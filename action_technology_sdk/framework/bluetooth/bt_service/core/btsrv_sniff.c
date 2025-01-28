/*
 * Copyright (c) 2016 Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Manager conn sniff.
 *   this is manager conn sniff.
 */
#define SYS_LOG_DOMAIN "btsrv_sniff"
#include "btsrv_os_common.h"
#include "btsrv_inner.h"

#define SNIFF_CHECK_INTERVALE					100
#define SNIFF_CHECK_FAILED_ENTER_SNIFF_TIME		(3000)

struct btsrv_sniff_priv {
	uint8_t enable_check_sniff:1;
	struct thread_timer sniff_check_timer;
};

static struct btsrv_sniff_priv btsrv_sniff_priv_data;
static struct btsrv_sniff_priv *p_sniff_priv;

struct btsrv_sniff_tws_info {
	struct bt_conn *conn;
	uint8_t phone_in_active_mode:1;
};

#define SNIFF_CHECK_ENTER_SNIFF_TIME	5000		/* 5s */
#define PHONE_SNIFF_MIN_INTERVAL		798 //800		/* 798*0.625ms = 498.75ms */
#define PHONE_SNIFF_MAX_INTERVAL		798 //800		/* 798*0.625ms = 498.75ms */
#define TWS_SNIFF_MIN_INTERVAL		240		/* 240*0.625ms = 150ms */
#define TWS_SNIFF_MAX_INTERVAL		240		/* 240*0.625ms = 150ms */

static void connected_dev_cb_check_sniff(struct bt_conn *conn, uint8_t tws_dev, void *cb_param)
{
	struct rdm_sniff_info *info;
	struct btsrv_sniff_tws_info *tws_sniff_info = cb_param;
	uint32_t curr_time;
	uint16_t conn_rxtx_cnt;
	uint8_t controler_role;

	if (btsrv_rdm_get_controler_role(conn, &controler_role)) {
		return;
	}

	if (tws_dev && (controler_role == CONTROLER_ROLE_SLAVE)) {
		return;
	}

	info = btsrv_rdm_get_sniff_info(conn);
	if (!info) {
		return;
	}

	curr_time = os_uptime_get_32();
	if (info->sniff_mode == BT_ACTIVE_MODE && info->sniff_entering) {
		if ((curr_time - info->sniff_entering_time) > SNIFF_CHECK_FAILED_ENTER_SNIFF_TIME) {
			info->sniff_entering = 0;
		}
	}

	if (tws_dev) {
		tws_sniff_info->conn = conn;
	} else {
		if (info->sniff_mode != BT_SNIFF_MODE) {
			/* Phone not in sniff mode, update tws idle time */
			tws_sniff_info->phone_in_active_mode = 1;
		}

		if (!btsrv_rdm_is_security_changed(conn)) {
			/* Phone check enter sniff after security finish */
			info->idle_start_time = curr_time;
			return;
		}
	}

	conn_rxtx_cnt = hostif_bt_conn_get_rxtx_cnt(conn);
	if (info->conn_rxtx_cnt != conn_rxtx_cnt) {
		info->conn_rxtx_cnt = conn_rxtx_cnt;
		info->idle_start_time = curr_time;
		return;
	}

	if (btsrv_rdm_is_sco_connected(conn)) {
		info->idle_start_time = curr_time;
		return;
	}

	if (info->sniff_mode == BT_ACTIVE_MODE && !info->sniff_entering) {
		if ((curr_time - info->idle_start_time) > SNIFF_CHECK_ENTER_SNIFF_TIME) {
			info->sniff_entering = 1;
			info->idle_start_time = curr_time;
			info->sniff_entering_time = curr_time;
			SYS_LOG_INF("Check 0x%x enter sniff", hostif_bt_conn_get_handle(conn));
			if (tws_dev) {
				hostif_bt_conn_check_enter_sniff(conn, TWS_SNIFF_MIN_INTERVAL, TWS_SNIFF_MAX_INTERVAL);
			} else {
				hostif_bt_conn_check_enter_sniff(conn, PHONE_SNIFF_MIN_INTERVAL, PHONE_SNIFF_MAX_INTERVAL);
			}
		}
	}
}

static void btsrv_sniff_timer_handler(struct thread_timer *ttimer, void *expiry_fn_arg)
{
	struct btsrv_sniff_tws_info tws_sniff_info;
	struct rdm_sniff_info *info;

	if (!p_sniff_priv->enable_check_sniff || btsrv_is_pts_test()) {
		return;
	}

	memset(&tws_sniff_info, 0, sizeof(tws_sniff_info));
	btsrv_rdm_get_connected_dev(connected_dev_cb_check_sniff, &tws_sniff_info);

	/* Tws enter sniff after all phone enter sniff */
	if (tws_sniff_info.conn && tws_sniff_info.phone_in_active_mode) {
		info = btsrv_rdm_get_sniff_info(tws_sniff_info.conn);
		if (info) {
			info->idle_start_time = os_uptime_get_32();
			/* Phone enter active mode, tws exit sniff mode. */
			if (info->sniff_mode == BT_SNIFF_MODE) {
				hostif_bt_conn_check_exit_sniff(tws_sniff_info.conn);
			}
		}
	}
}

void btsrv_sniff_update_idle_time(struct bt_conn *conn)
{
	struct rdm_sniff_info *info;

	if (!p_sniff_priv->enable_check_sniff || btsrv_is_pts_test()) {
		return;
	}

	info = btsrv_rdm_get_sniff_info(conn);
	if (!info) {
		return;
	}

	info->idle_start_time = os_uptime_get_32();
}

void btsrv_sniff_mode_change(void *param)
{
	struct btsrv_mode_change_param *in_param = param;
	struct rdm_sniff_info *info;

	info = btsrv_rdm_get_sniff_info(in_param->conn);
	if (!info) {
		return;
	}

	info->sniff_mode = in_param->mode;
	info->sniff_interval = in_param->interval;
	info->sniff_entering = 0;
	info->sniff_exiting = 0;
	info->idle_start_time = os_uptime_get_32();
}

/* Just for btsrv_trs_a2dp_source start */
bool hostif_bt_conn_is_in_sniff(struct bt_conn *conn)
{
	return btsrv_sniff_in_sniff_mode(conn);
}

void hostif_bt_conn_force_exit_sniff(struct bt_conn *conn)
{
	hostif_bt_conn_check_exit_sniff(conn);
}
/* Just for btsrv_trs_a2dp_source end */

void btsrv_sniff_set_check_enable(bool enable)
{
	if (enable) {
		p_sniff_priv->enable_check_sniff = 1;
	} else {
		p_sniff_priv->enable_check_sniff = 0;
	}
}

bool btsrv_sniff_in_sniff_mode(struct bt_conn *conn)
{
	struct rdm_sniff_info *info;

	info = btsrv_rdm_get_sniff_info(conn);
	if (!info) {
		return false;
	}

	return (info->sniff_mode == BT_SNIFF_MODE) ? true : false;
}

void btsrv_sniff_init(void)
{
	p_sniff_priv = &btsrv_sniff_priv_data;

	memset(p_sniff_priv, 0, sizeof(struct btsrv_sniff_priv));
	p_sniff_priv->enable_check_sniff = 1;
	thread_timer_init(&p_sniff_priv->sniff_check_timer, btsrv_sniff_timer_handler, NULL);
	thread_timer_start(&p_sniff_priv->sniff_check_timer, SNIFF_CHECK_INTERVALE, SNIFF_CHECK_INTERVALE);
}

void btsrv_sniff_deinit(void)
{
	if (p_sniff_priv == NULL) {
		return;
	}

	if (thread_timer_is_running(&p_sniff_priv->sniff_check_timer)) {
		thread_timer_stop(&p_sniff_priv->sniff_check_timer);
	}

	p_sniff_priv = NULL;
}
