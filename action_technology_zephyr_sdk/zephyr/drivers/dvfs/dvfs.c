/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file dynamic voltage and frequency scaling interface
 */

#include <kernel.h>
#include <init.h>
#include <device.h>
#include <soc.h>
#include <dvfs.h>

#include <logging/log.h>

LOG_MODULE_REGISTER(dvfs0, CONFIG_LOG_DEFAULT_LEVEL);

#define CONFIG_DVFS_FADE_STEP

#ifdef CONFIG_ACTS_DVFS_DYNAMIC_LEVEL

struct dvfs_manager {
	struct k_sem lock;
	uint8_t cur_dvfs_idx;
	uint8_t default_dvfs_idx;
	uint8_t dvfs_level_cnt;
	uint8_t asrc_limit_clk_mhz;
	uint8_t spdif_limit_clk_mhz;
	struct dvfs_level *dvfs_level_tbl;
};

struct dvfs_level_max {
	uint16_t cpu_freq;
	uint16_t dsp_freq;
	uint16_t gpu_freq;
	uint16_t de_freq;
	uint16_t jpeg_freq;
	uint16_t vdd_volt;
};

static sys_dlist_t __dvfs_notifier_data dvfs_notify_list = SYS_DLIST_STATIC_INIT(&dvfs_notify_list);

/* NOTE: DON'T modify max_soc_dvfs_table except actions ic designers */
#define CPU_FREQ_MAX  128
#define VDD_VOLT_MAX  1200

/* vd12 and vdd should satisfy the need for voltage difference */
#define DIFF_VD12_VDD_VOLT_MV   50

static const struct dvfs_level_max max_soc_dvfs_table1[] = {
	/* cpu_freq,  dsp_freq, gpu_freq, de_freq, jpeg_freq, vodd_volt  */
	{16,           16,         16,      16,        16,       900},
	{70,           70,         70,      70,        70,       950},
	{94,           94,         94,     	94,       140,       1000},
	{140,          140,         140,     140,      187,      1100},
	{187,          187,         187,     187,      280,      1200},
};

static const struct dvfs_level_max max_soc_dvfs_table0[] = {
	/* cpu_freq,  dsp_freq, gpu_freq, de_freq, jpeg_freq, vodd_volt  */
	{64,           64,         86,     86,      128,      	1000},
	{86,           128,        128,     128,      171,      1100},
	{128,          171,        171,     171,      256,      1200},
};

static uint16_t dvfs_get_optimal_volt(uint16_t cpu_freq, uint16_t dsp_freq,
                                          uint16_t gpu_freq, uint16_t de_freq,
                                          uint16_t jpeg_freq,
                                          const struct dvfs_level_max *max_soc_dvfs_table,
                                          int level_cnt)
{
	uint16_t volt;
	int i;

	volt = max_soc_dvfs_table[level_cnt - 1].vdd_volt;
	for (i = 0; i < level_cnt; i++) {
		if ((cpu_freq <= max_soc_dvfs_table[i].cpu_freq)
			&& (dsp_freq <= max_soc_dvfs_table[i].dsp_freq)
			&& (gpu_freq <= max_soc_dvfs_table[i].gpu_freq)
			&& (de_freq <= max_soc_dvfs_table[i].de_freq)
			&& (jpeg_freq <= max_soc_dvfs_table[i].jpeg_freq)) {
			volt = max_soc_dvfs_table[i].vdd_volt;
			break;
		}
	}

	return volt;
}

static struct dvfs_level default_soc_dvfs_table1[] = {
	/* level                   enable_cnt cpu_freq, dsp_freq, gpu_freq, de_freq, jpeg_freq, vodd_volt */
	{DVFS_LEVEL_S2,                0,         70,        70,     70,      70,       70,         0},
	{DVFS_LEVEL_NORMAL,            0,         94,        94,     94,      94,       140,        0},
	{DVFS_LEVEL_PERFORMANCE,   	   0,         94,       112,     94,      94,       140,        0},
	{DVFS_LEVEL_MID_PERFORMANCE,   0,        140,       112,     140,     140,      187,        0},
	{DVFS_LEVEL_HIGH_PERFORMANCE,  0,        187,       187,     187,     187,      280,        0},
};

static struct dvfs_level default_soc_dvfs_table0[] = {
	/* level                   enable_cnt cpu_freq, dsp_freq, gpu_freq, de_freq, jpeg_freq, vodd_volt */
	{DVFS_LEVEL_S2,                0,        64,        64,      86,      86,       128,        0},
	{DVFS_LEVEL_NORMAL,            0,        86,        128,     128,     128,      171,        0},
	{DVFS_LEVEL_PERFORMANCE,   	   0,        86,        128,     128,     128,      171,        0},
	{DVFS_LEVEL_MID_PERFORMANCE,   0,        96,        128,     128,     128,      171,        0},
	{DVFS_LEVEL_HIGH_PERFORMANCE,  0,        128,       171,     171,     171,      256,        0},
};

static struct dvfs_manager g_dvfs;
#if 0
static int level_id_to_vdd(int level_id)
{
	int i;

	for (i = 0; i < g_dvfs.dvfs_level_cnt; i++)
		if (g_dvfs.dvfs_level_tbl[i].level_id == level_id) {
			return g_dvfs.dvfs_level_tbl[i].vdd_volt;
		}

	return -1;
}
#endif
static int level_id_to_tbl_idx(int level_id)
{
	int i;

	for (i = 0; i < g_dvfs.dvfs_level_cnt; i++) {
		if (g_dvfs.dvfs_level_tbl[i].level_id == level_id) {
			return i;
		}
	}

	return -1;
}

static int dvfs_get_max_idx(void)
{
	int i;

	for (i = (g_dvfs.dvfs_level_cnt - 1); i >= 0; i--) {
		if (g_dvfs.dvfs_level_tbl[i].enable_cnt > 0) {
			return i;
		}
	}

	return -1;
}

static void dvfs_dump_tbl(void)
{
	const struct dvfs_level *dvfs_level = &g_dvfs.dvfs_level_tbl[0];
	int i;

	printk("idx   level_id   dsp   cpu   gpu   de    jpeg  vdd   enable_cnt\n");
	for (i = 0; i < g_dvfs.dvfs_level_cnt; i++, dvfs_level++) {
		printk("%-6d%-11d%-6d%-6d%-6d%-6d%-6d%-6d%-6d\n", i,
			dvfs_level->level_id,
			dvfs_level->dsp_freq,
			dvfs_level->cpu_freq,
			dvfs_level->gpu_freq,
			dvfs_level->de_freq,
			dvfs_level->jpeg_freq,
			dvfs_level->vdd_volt,
			dvfs_level->enable_cnt);
	}
}

__dvfs_notifier_func static void dvfs_changed_notify(int state, uint8_t old_level_index, uint8_t new_level_index)
{
	struct dvfs_notifier *obj, *next;

	struct dvfs_freqs dvfs_freqs_info = {0};
	dvfs_freqs_info.state = state;
	dvfs_freqs_info.old_level = old_level_index;
	dvfs_freqs_info.new_level = new_level_index;

	SYS_DLIST_FOR_EACH_CONTAINER_SAFE(&dvfs_notify_list, obj, next, node) {
		LOG_DBG("dvfs notify state:%d func:%p\n", state, obj->dvfs_notify_func_t);
		if (obj->dvfs_notify_func_t)
			obj->dvfs_notify_func_t(obj->user_data, &dvfs_freqs_info);
	}
}

static void dvfs_sync(void)
{
	struct dvfs_level *dvfs_level, *old_dvfs_level;
	int old_idx, new_idx;
	uint32_t old_dsp_freq, old_volt, volt;
	uint8_t __old_idx, __new_idx, cur_idx;

	old_idx = g_dvfs.cur_dvfs_idx;

	/* get current max dvfs level */
	new_idx = dvfs_get_max_idx();
	if (new_idx == old_idx) {
		/* same level, no need sync */
		LOG_INF("max idx %d\n", new_idx);
		return;
	}

	/* if all dvfs levels are not enabled to use the default level */
	if (-1 == new_idx) {
		LOG_INF("dvfs use default level:%d", g_dvfs.default_dvfs_idx);
		new_idx = g_dvfs.default_dvfs_idx;
	}

	cur_idx = old_idx;
	while (cur_idx != new_idx) {
		__old_idx = cur_idx;

#ifdef CONFIG_DVFS_FADE_STEP
		if (cur_idx > new_idx)
			cur_idx--;
		else
			cur_idx++;
#else
		cur_idx = new_idx;
#endif

		__new_idx = cur_idx;

		dvfs_level = &g_dvfs.dvfs_level_tbl[__new_idx];

		old_dvfs_level = &g_dvfs.dvfs_level_tbl[__old_idx];
		old_volt = soc_pmu_get_vdd_voltage();
		old_dsp_freq = soc_freq_get_dsp_freq();

		LOG_INF("level_id [%d] -> [%d]", old_dvfs_level->level_id,
			dvfs_level->level_id);

		/* send notify before clock setting */
		dvfs_changed_notify(DVFS_EVENT_PRE_CHANGE, __old_idx, __new_idx);

		/* set vdd voltage before clock setting if new vdd is up */
		if (dvfs_level->vdd_volt > old_volt) {
			/*check dcdc or ldo mode, if ldo mode ,need add 100mv, dcdc mode add 50mv */
			if((sys_read32(VOUT_CTL0) & BIT(17))) {
				soc_pmu_set_vd12_voltage(dvfs_level->vdd_volt + DIFF_VD12_VDD_VOLT_MV);
			} else {
				soc_pmu_set_vd12_voltage(dvfs_level->vdd_volt + DIFF_VD12_VDD_VOLT_MV * 2);
			}
			volt = old_volt;
			while(volt < dvfs_level->vdd_volt) {
				volt += 50 ; // step 0.05v, for psram/
				soc_pmu_set_vdd_voltage(volt);
			}
		}

		printk("new dsp freq %d, cpu freq %d, gpu freq %d, de freq %d, jpeg freq %d, vdd volt %d\n",
			dvfs_level->dsp_freq, dvfs_level->cpu_freq,
			dvfs_level->gpu_freq, dvfs_level->de_freq,
			dvfs_level->jpeg_freq, dvfs_level->vdd_volt);

		/* adjust core/dsp/cpu clock */
		soc_freq_set_cpu_clk(dvfs_level->dsp_freq, dvfs_level->cpu_freq);

		/* adjust gpu/de/jpeg clock.
		 * freq=0 will be ignored for the case of not used condition,
		 * such as lark not use jpeg, so jpeg_mhz can be set 0.
		 */
		soc_freq_set_gpu_clk(dvfs_level->gpu_freq,
                             dvfs_level->de_freq,
                             dvfs_level->jpeg_freq);

		/* set vdd voltage after clock setting if new vdd is down */
		if (dvfs_level->vdd_volt < old_volt) {
			soc_pmu_set_vdd_voltage(dvfs_level->vdd_volt);
			/*check dcdc or ldo mode, if ldo mode ,need add 100mv, dcdc mode add 50mv */
			if((sys_read32(VOUT_CTL0) & BIT(17))) {
				soc_pmu_set_vd12_voltage(dvfs_level->vdd_volt + DIFF_VD12_VDD_VOLT_MV);
			} else {
				soc_pmu_set_vd12_voltage(dvfs_level->vdd_volt + DIFF_VD12_VDD_VOLT_MV * 2);
			}
		}

		/* send notify after clock setting */
		dvfs_changed_notify(DVFS_EVENT_POST_CHANGE, __old_idx, __new_idx);

	}

	g_dvfs.cur_dvfs_idx = new_idx;
}

static int dvfs_update_freq(int level_id, bool is_set, const char *user_info)
{
	struct dvfs_level *dvfs_level;
	int tbl_idx;

	LOG_INF("level %d, is_set %d %s", level_id, is_set, user_info);

	tbl_idx = level_id_to_tbl_idx(level_id);
	if (tbl_idx < 0) {
		LOG_ERR("%s: invalid level id %d\n", __func__, level_id);
		return -EINVAL;
	}

	dvfs_level = &g_dvfs.dvfs_level_tbl[tbl_idx];

	k_sem_take(&g_dvfs.lock, K_FOREVER);

	if (is_set) {
	    if(dvfs_level->enable_cnt < 255){
            dvfs_level->enable_cnt++;
	    }else{
            LOG_WRN("max dvfs level count");
	    }
	} else {
		if (dvfs_level->enable_cnt > 0) {
			dvfs_level->enable_cnt--;
		}else{
            LOG_WRN("min dvfs level count");
		}
	}

	dvfs_sync();

	k_sem_give(&g_dvfs.lock);

	return 0;
}

int dvfs_force_set_level(int level_id, const char *user_info)
{
	return dvfs_update_freq(level_id, 1, user_info);
}

int dvfs_force_unset_level(int level_id, const char *user_info)
{
	return dvfs_update_freq(level_id, 0, user_info);
}

static bool dvfs_lock_flag = false;

int dvfs_lock(void)
{
	dvfs_lock_flag = true;
	return 0;
}

int dvfs_unlock(void)
{
	dvfs_lock_flag = false;
	return 0;
}

int dvfs_set_level(int level_id, const char *user_info)
{
	if (!dvfs_lock_flag) {
		return dvfs_update_freq(level_id, 1, user_info);
	} else {
		printk("dvfs locked \n");
		return 0;
	}
}

int dvfs_unset_level(int level_id, const char *user_info)
{
	if (!dvfs_lock_flag) {
		return dvfs_update_freq(level_id, 0, user_info);
	} else {
		printk("dvfs locked \n");
		return 0;
	}	
}

int dvfs_get_current_level(void)
{
	int idx;

	if (!g_dvfs.dvfs_level_tbl)
		return -1;

	idx = g_dvfs.cur_dvfs_idx;
	if (idx < 0) {
		idx = 0;
	}

	return g_dvfs.dvfs_level_tbl[idx].level_id;
}

int dvfs_set_freq_table(struct dvfs_level *dvfs_level_tbl, int level_cnt)
{
	int i;
	//uint32_t vdd;

	if ((!dvfs_level_tbl) || (level_cnt <= 0))
		return -EINVAL;

	if (soc_dvfs_opt()) {
		for (i = 0; i < level_cnt; i++) {
			dvfs_level_tbl[i].vdd_volt =
			dvfs_get_optimal_volt(dvfs_level_tbl[i].cpu_freq,
                                  dvfs_level_tbl[i].dsp_freq,
                                  dvfs_level_tbl[i].gpu_freq,
                                  dvfs_level_tbl[i].de_freq,
                                  dvfs_level_tbl[i].jpeg_freq,
                                  max_soc_dvfs_table1,
                                  ARRAY_SIZE(max_soc_dvfs_table1));
		}
	}
	else {
		for (i = 0; i < level_cnt; i++) {
			dvfs_level_tbl[i].vdd_volt =
			dvfs_get_optimal_volt(dvfs_level_tbl[i].cpu_freq,
                                  dvfs_level_tbl[i].dsp_freq,
                                  dvfs_level_tbl[i].gpu_freq,
                                  dvfs_level_tbl[i].de_freq,
                                  dvfs_level_tbl[i].jpeg_freq,
                                  max_soc_dvfs_table0,
                                  ARRAY_SIZE(max_soc_dvfs_table0));
		}
	}

	g_dvfs.dvfs_level_cnt = level_cnt;
	g_dvfs.dvfs_level_tbl = dvfs_level_tbl;

	/* current dvfs id set to DVFS_LEVEL_HIGH_PERFORMANCE - 1 */
	/* make sure we can init DVFS_LEVEL_HIGH_PERFORMANCE first time */
#if 0
	/* default level/vdd */
	vdd = soc_pmu_get_vdd_voltage();

	/* search for current vdd level */
	for (i = 0; i < g_dvfs.dvfs_level_cnt; i++) {
		if (g_dvfs.dvfs_level_tbl[i].vdd_volt >= vdd) {
			break;
		}
	}

	if (i != g_dvfs.dvfs_level_cnt)
		g_dvfs.cur_dvfs_idx = i;
	else
		g_dvfs.cur_dvfs_idx = 0;
#endif

	g_dvfs.cur_dvfs_idx = level_id_to_tbl_idx(DVFS_LEVEL_MID_PERFORMANCE);

	g_dvfs.default_dvfs_idx = level_id_to_tbl_idx(DVFS_LEVEL_S2);

	printk("current dvfs level_id:%d default_dvfs_idx %d \n", g_dvfs.cur_dvfs_idx, g_dvfs.default_dvfs_idx);

	dvfs_dump_tbl();

	return 0;
}

struct dvfs_level *dvfs_get_info_by_level_id(int level_id)
{
	return &g_dvfs.dvfs_level_tbl[level_id];
}

int dvfs_register_notifier(struct dvfs_notifier *notifier)
{
	struct dvfs_notifier *obj;

	if (!notifier)
		return -EINVAL;

    SYS_DLIST_FOR_EACH_CONTAINER(&dvfs_notify_list, obj, node) {
		if (obj == notifier) {
			LOG_ERR("dvfs notifier:%p has already registered", notifier);
			return -EEXIST;
		}
    }

	sys_dlist_append(&dvfs_notify_list, &notifier->node);

	LOG_DBG("dvfs register notifier:%p func:%p\n",
			notifier, notifier->dvfs_notify_func_t);

	return 0;
}

int dvfs_unregister_notifier(struct dvfs_notifier *notifier)
{
	if (!notifier)
		return -EINVAL;

	sys_dlist_remove(&notifier->node);

	return 0;
}

#endif /* CONFIG_ACTS_DVFS_DYNAMIC_LEVEL */

static int dvfs_init(const struct device *arg)
{
	ARG_UNUSED(arg);

	LOG_INF("default dsp freq:%dHz cpu freq:%dHz vdd:%dmV",
		soc_freq_get_dsp_freq(), soc_freq_get_cpu_freq(),
		soc_pmu_get_vdd_voltage());

#ifdef CONFIG_ACTS_DVFS_DYNAMIC_LEVEL
	if (soc_dvfs_opt()) {
		dvfs_set_freq_table(default_soc_dvfs_table1,
			ARRAY_SIZE(default_soc_dvfs_table1));
	}
	else {
		dvfs_set_freq_table(default_soc_dvfs_table0,
			ARRAY_SIZE(default_soc_dvfs_table0));
	}

	k_sem_init(&g_dvfs.lock, 1, 1);

	dvfs_set_level(DVFS_LEVEL_HIGH_PERFORMANCE, "init");
#endif

	return 0;
}

SYS_INIT(dvfs_init, PRE_KERNEL_1, 20);
