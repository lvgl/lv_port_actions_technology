/*
 * Copyright (c) 2016 Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief bt inquiry/page scan enable manager.
 *   this is manager device discoverable/connectable.
 */

#define SYS_LOG_DOMAIN "btsrv_scan"
#include "btsrv_os_common.h"
#include "btsrv_inner.h"

#define BTSRV_SCAN_DELAY_SET		0
#define BTSRV_SCAN_DELAY_RUN		200			/* 200ms */

enum {
	INQUIRY_GIAC,
	INQUIRY_LIAC,
};

enum {
	BT_NO_SCANS_ENABLE = 0,
	BT_INQUIRY_EANBEL_PAGE_DISABLE = 1,
	BT_INQUIRY_DISABLE_PAGE_ENABLE = 2,
	BT_INQUIRY_ENABLE_PAGE_ENABLE = 3,
};

enum {
	BT_STANDARD_SCAN = 0,
	BT_INTERLACED_SCAN = 1,
};

struct btsrv_scan_priv {
	uint8_t user_discoverable:1;
	uint8_t user_connectable:1;
	uint8_t curr_discoverable:1;
	uint8_t curr_connectable:1;
	uint8_t curr_iac:2;
	uint8_t srv_discoverable:1;
	uint8_t srv_connectable:1;
	struct bt_scan_parameter default_param;
	struct bt_scan_parameter user_param;
	struct bt_scan_parameter curr_param;

#if BTSRV_SCAN_DELAY_SET
	uint8_t scan_timer_running:1;
	struct thread_timer scan_timer;
#endif
};

static const struct bt_scan_parameter default_scan_param = {
	.tws_limit_inquiry = 0,
	.idle_extend_windown = 0,
	.inquiry_interval = 0x1000,
	.inquiry_windown = 0x0012,
	.page_interval = 0x0800,
	.page_windown = 0x0012,
};

static struct btsrv_scan_priv *p_scan;

static uint8_t btsrv_scan_discoverable_connectable_to_mode(uint8_t discoverable, uint8_t connectable)
{
	uint8_t mode = 0;

	if (discoverable) {
		mode |= BT_INQUIRY_EANBEL_PAGE_DISABLE;
	}

	if (connectable) {
		mode |= BT_INQUIRY_DISABLE_PAGE_ENABLE;
	}

	return mode;
}

static void btsrv_scan_set_scan_mode(uint8_t discoverable, uint8_t connectable, uint8_t iac_mode,
								struct bt_scan_parameter *param)
{
	uint8_t req_scan_mode, curr_scan_mode;
	uint8_t update_scan_mode = 0;
	uint8_t update_inquiry_param = 0;
	uint8_t update_page_param = 0;
	uint8_t update_iac = 0;

	req_scan_mode = btsrv_scan_discoverable_connectable_to_mode(discoverable, connectable);
	curr_scan_mode = btsrv_scan_discoverable_connectable_to_mode(p_scan->curr_discoverable, p_scan->curr_connectable);
	if (req_scan_mode != curr_scan_mode) {
		update_scan_mode = 1;
	}

	if (iac_mode != p_scan->curr_iac) {
		update_iac = 1;
	}

	if ((param->inquiry_interval != p_scan->curr_param.inquiry_interval) ||
		(param->inquiry_windown != p_scan->curr_param.inquiry_windown)) {
		update_inquiry_param = 1;
	}

	if ((param->page_interval != p_scan->curr_param.page_interval) ||
		(param->page_windown != p_scan->curr_param.page_windown)) {
		update_page_param = 1;
	}

	if (!(update_scan_mode || update_inquiry_param ||
			update_page_param || update_iac)) {
		return;
	}

	if (update_iac) {
		SYS_LOG_INF("iac_mode: %d", iac_mode);
		hostif_bt_br_write_iac(((iac_mode == INQUIRY_LIAC) ? true : false));
		p_scan->curr_iac = iac_mode;
	}

	if (update_inquiry_param || update_page_param) {
		/* Disable scan mode before update scan parameter */
		if (curr_scan_mode != BT_NO_SCANS_ENABLE) {
			SYS_LOG_INF("discoverable 0, connectable 0(Update parem)");
			if (hostif_bt_br_write_scan_enable(BT_NO_SCANS_ENABLE) == 0) {
				curr_scan_mode = BT_NO_SCANS_ENABLE;
				p_scan->curr_discoverable = 0;
				p_scan->curr_connectable = 0;
			} else {
				SYS_LOG_ERR("Failed to write_scan_enable !!!");
			}
		}

		if (curr_scan_mode != req_scan_mode) {
			update_scan_mode = 1;
		} else {
			update_scan_mode = 0;
		}
	}

	if (update_inquiry_param) {
		SYS_LOG_INF("inquiry param: 0x%x, 0x%x", param->inquiry_interval, param->inquiry_windown);
		hostif_bt_br_write_inquiry_scan_activity(param->inquiry_interval, param->inquiry_windown);
		if ((param->inquiry_interval == p_scan->default_param.inquiry_interval) &&
			(param->inquiry_windown == p_scan->default_param.inquiry_windown)) {
			hostif_bt_br_write_inquiry_scan_type(BT_STANDARD_SCAN);
		} else {
			hostif_bt_br_write_inquiry_scan_type(BT_INTERLACED_SCAN);
		}
	}

	if (update_page_param) {
		SYS_LOG_INF("page param: 0x%x, 0x%x", param->page_interval, param->page_windown);
		hostif_bt_br_write_page_scan_activity(param->page_interval, param->page_windown);
		if ((param->page_interval == p_scan->default_param.page_interval) &&
			(param->page_windown == p_scan->default_param.page_windown)) {
			hostif_bt_br_write_page_scan_type(BT_STANDARD_SCAN);
		} else {
			hostif_bt_br_write_page_scan_type(BT_INTERLACED_SCAN);
		}
	}

	if (update_inquiry_param || update_page_param) {
		memcpy(&p_scan->curr_param, param, sizeof(struct bt_scan_parameter));
	}

	if (update_scan_mode) {
		SYS_LOG_INF("discoverable %d, connectable %d", discoverable, connectable);
		if (hostif_bt_br_write_scan_enable(req_scan_mode) == 0) {
			p_scan->curr_discoverable = discoverable;
			p_scan->curr_connectable = connectable;
		} else {
			SYS_LOG_ERR("Failed to write_scan_enable !!!");
		}
	}
}

static void btsrv_scan_check_state(void)
{
	bool reconnect_runing;
	bool tws_pairing = 0;
	bool tws_connecting = 0;
	int connected_dev_cnt;
	int dev_role;
	uint8_t exp_discoverable = 0;
	uint8_t ext_connectable = 0;
	uint8_t iac_mode = INQUIRY_GIAC;
	uint8_t max_phone_num;
	struct bt_scan_parameter param;

#if BTSRV_SCAN_DELAY_SET
	if (thread_timer_is_running(&p_scan->scan_timer)) {
		thread_timer_stop(&p_scan->scan_timer);
	}
#endif

	max_phone_num = btsrv_max_phone_num();
	reconnect_runing = btsrv_autoconn_is_reconnecting();

#ifdef CONFIG_SUPPORT_TWS
	btsrv_tws_get_pairing_connecting_state(&tws_pairing, &tws_connecting);
#endif

	connected_dev_cnt = btsrv_rdm_get_connected_dev(NULL, NULL);
	dev_role = btsrv_rdm_get_dev_role();
	memcpy(&param, &p_scan->default_param, sizeof(struct bt_scan_parameter));

	if (!p_scan->user_discoverable || !p_scan->srv_discoverable) {
		exp_discoverable = 0;
	} else if (connected_dev_cnt == btsrv_max_conn_num()) {
		exp_discoverable = 0;
	} else if (dev_role == BTSRV_TWS_SLAVE) {
		exp_discoverable = 0;
	} else if (tws_pairing) {
		if (connected_dev_cnt == max_phone_num) {
			exp_discoverable = 0;
		} else {
			exp_discoverable = 1;
		}

		if (p_scan->user_param.tws_limit_inquiry) {
			iac_mode = INQUIRY_LIAC;
		} else {
			iac_mode = INQUIRY_GIAC;
		}

		if (connected_dev_cnt == 0) {
			param.inquiry_interval = p_scan->user_param.inquiry_interval;
			param.inquiry_windown = p_scan->user_param.inquiry_windown;
		}
	} else if (reconnect_runing || tws_connecting) {
		exp_discoverable = 0;
	} else if ((dev_role == BTSRV_TWS_NONE) &&
#ifdef CONFIG_BT_A2DP_TRS
				(btsrv_rdm_get_dev_trs_mode() == 0) &&
#endif
				(connected_dev_cnt == max_phone_num)) {
		exp_discoverable = 0;
	} else {
		if ((connected_dev_cnt == 0) && (p_scan->user_param.idle_extend_windown)) {
			param.inquiry_interval = p_scan->user_param.inquiry_interval;
			param.inquiry_windown = p_scan->user_param.inquiry_windown;
		}
		exp_discoverable = 1;
	}

	if (!p_scan->user_connectable || !p_scan->srv_connectable) {
		ext_connectable = 0;
	} else if (connected_dev_cnt == btsrv_max_conn_num()) {
		ext_connectable = 0;
	} else if (dev_role == BTSRV_TWS_SLAVE) {
		ext_connectable = 0;
	} else if (tws_pairing) {
		ext_connectable = 1;
		if (connected_dev_cnt == 0) {
			param.page_interval = p_scan->user_param.page_interval;
			param.page_windown = p_scan->user_param.page_windown;
		}
	} else if (reconnect_runing || tws_connecting) {
		if (connected_dev_cnt == 0) {
			ext_connectable = 1;
			param.page_interval = p_scan->user_param.page_interval;
			param.page_windown = p_scan->user_param.page_windown;
		} else {
			ext_connectable = 0;
		}
	} else {
		if ((connected_dev_cnt == 0) && (p_scan->user_param.idle_extend_windown)) {
			param.page_interval = p_scan->user_param.page_interval;
			param.page_windown = p_scan->user_param.page_windown;
		}

		ext_connectable = 1;
	}

	btsrv_scan_set_scan_mode(exp_discoverable, ext_connectable, iac_mode, &param);
}

#if BTSRV_SCAN_DELAY_SET
static void btsrv_scan_timer_handler(struct thread_timer *ttimer, void *expiry_fn_arg)
{
	p_scan->scan_timer_running = 1;
	btsrv_scan_check_state();
	p_scan->scan_timer_running = 0;
}

static void btsrv_scan_restart_timer(void)
{
	if (thread_timer_is_running(&p_scan->scan_timer)) {
		thread_timer_stop(&p_scan->scan_timer);
	}

	thread_timer_start(&p_scan->scan_timer, BTSRV_SCAN_DELAY_RUN, 0);
}
#endif

void btsrv_scan_set_param(struct bt_scan_parameter *param, bool enhance_param)
{
	SYS_LOG_INF("%s param %d %d, 0x%x(0x%x), 0x%x(0x%x)", enhance_param ? "Enhance" : "Default",
		param->tws_limit_inquiry, param->idle_extend_windown,
		param->inquiry_interval, param->inquiry_windown,
		param->page_interval, param->page_windown);

	if (enhance_param) {
		memcpy(&p_scan->user_param, param, sizeof(struct bt_scan_parameter));
	} else {
		memcpy(&p_scan->default_param, param, sizeof(struct bt_scan_parameter));
	}
}

void btsrv_scan_set_user_discoverable(bool enable, bool immediate)
{
	p_scan->user_discoverable = enable ? 1 : 0;

#if BTSRV_SCAN_DELAY_SET
	if (immediate) {
		btsrv_scan_check_state();
	} else {
		btsrv_scan_restart_timer();
	}
#else
	btsrv_scan_check_state();
#endif
}

void btsrv_scan_set_user_connectable(bool enable, bool immediate)
{
	p_scan->user_connectable = enable ? 1 : 0;

#if BTSRV_SCAN_DELAY_SET
	if (immediate) {
		btsrv_scan_check_state();
	} else {
		btsrv_scan_restart_timer();
	}
#else
	btsrv_scan_check_state();
#endif
}

void btsrv_inner_set_scan_enable(bool discoverable, bool connectable)
{
	p_scan->srv_discoverable = discoverable ? 1 : 0;
	p_scan->srv_connectable = connectable ? 1 : 0;

	btsrv_scan_check_state();
}

void btsrv_scan_update_mode(bool immediate)
{
#if BTSRV_SCAN_DELAY_SET
	if (immediate) {
		btsrv_scan_check_state();
	} else {
		btsrv_scan_restart_timer();
	}
#else
	btsrv_scan_check_state();
#endif
}

uint8_t btsrv_scan_get_inquiry_mode(void)
{
	return p_scan->user_param.tws_limit_inquiry;
}

static struct btsrv_scan_priv btsrv_scan;
int btsrv_scan_init(void)
{
	p_scan = &btsrv_scan;

	memset(p_scan, 0, sizeof(struct btsrv_scan_priv));

	p_scan->user_discoverable = 1;		/* Enable discoverable after start */
	p_scan->user_connectable = 1;		/* Enable connectable after start */
	p_scan->srv_discoverable = 1;		/* Enable connectable after start */
	p_scan->srv_connectable = 1;		/* Enable connectable after start */
	memcpy(&p_scan->default_param, &default_scan_param, sizeof(struct bt_scan_parameter));
	memcpy(&p_scan->curr_param, &default_scan_param, sizeof(struct bt_scan_parameter));
	memcpy(&p_scan->user_param, &default_scan_param, sizeof(struct bt_scan_parameter));

#if BTSRV_SCAN_DELAY_SET
	thread_timer_init(&p_scan->scan_timer, btsrv_scan_timer_handler, NULL);
#endif
	return 0;
}

void btsrv_scan_deinit(void)
{
	if (p_scan == NULL) {
		return;
	}

#if BTSRV_SCAN_DELAY_SET
	while (p_scan->scan_timer_running) {
		os_sleep(10);
	}

	if (thread_timer_is_running(&p_scan->scan_timer)) {
		thread_timer_stop(&p_scan->scan_timer);
	}
#endif

	p_scan = NULL;
}

void btsrv_scan_dump_info(void)
{
	if (p_scan == NULL) {
		printk("Btsrv scan info p_scan not init\n");
		return;
	} else {
		printk("Btsrv scan info\n");
	}

	printk("\t user_discoverable %d, user_connectable %d\n", p_scan->user_discoverable, p_scan->user_connectable);
	printk("\t srv_discoverable %d, srv_connectable %d\n", p_scan->srv_discoverable, p_scan->srv_connectable);
	printk("\t curr_discoverable %d, curr_connectable %d\n", p_scan->curr_discoverable, p_scan->curr_connectable);
	printk("\t inquiry 0x%x-0x%x, page 0x%x-0x%x\n", p_scan->curr_param.inquiry_interval,  p_scan->curr_param.inquiry_windown,
												 p_scan->curr_param.page_interval,  p_scan->curr_param.page_windown);
}
