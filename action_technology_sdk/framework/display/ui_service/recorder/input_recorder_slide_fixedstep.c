/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <input_recorder.h>

extern int16_t view_manager_get_disp_xres(void);
extern int16_t view_manager_get_disp_yres(void);

typedef struct inputrec_slide_data {
	int16_t scrl_min;
	int16_t scrl_max;
	int16_t scrl_cur;
	int16_t scrl_step;

	bool is_vscrl;
} inputrec_slide_ctx_t;

static inputrec_slide_ctx_t inputrec_slide_ctx;

static int _input_data_fixedstep_read(input_rec_data_t *data, void *user_data)
{
	inputrec_slide_ctx_t *ctx = user_data;

	data->timestamp = 0;
	data->data.state = INPUT_DEV_STATE_PR;

	if (ctx->is_vscrl) {
		data->data.scroll_dir = (ctx->scrl_step > 0) ? GESTURE_DROP_DOWN : GESTURE_DROP_UP;
		data->data.point.y = ctx->scrl_cur;
		data->data.point.x =  view_manager_get_disp_xres() / 2;
	} else {
		data->data.scroll_dir = (ctx->scrl_step > 0) ? GESTURE_DROP_RIGHT : GESTURE_DROP_LEFT;
		data->data.point.x = ctx->scrl_cur;
		data->data.point.y =  view_manager_get_disp_yres() / 2;
	}

	ctx->scrl_cur += ctx->scrl_step;

	if (ctx->scrl_step > 0 && ctx->scrl_cur >= ctx->scrl_max) {
		ctx->scrl_cur = ctx->scrl_max;
		ctx->scrl_step = -ctx->scrl_step;
	} else if (ctx->scrl_step < 0 && ctx->scrl_cur <= ctx->scrl_min) {
		ctx->scrl_cur = ctx->scrl_min;
		ctx->scrl_step = -ctx->scrl_step;
	}

	return 0;
}

int input_playback_slide_fixstep_start(int16_t start, int16_t stop, int16_t step, bool is_vert)
{
	inputrec_slide_ctx_t *ctx = &inputrec_slide_ctx;

	if (start < stop) {
		ctx->scrl_min = start;
		ctx->scrl_max = stop;
	} else {
		ctx->scrl_min = stop;
		ctx->scrl_max = start;
	}

	ctx->scrl_cur = start;
	ctx->scrl_step = step;
	ctx->is_vscrl = is_vert;

	return input_playback_start(_input_data_fixedstep_read, NULL, -1, ctx);
}

