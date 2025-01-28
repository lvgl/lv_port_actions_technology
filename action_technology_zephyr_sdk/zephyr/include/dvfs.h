/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file dynamic voltage and frequency scaling interface
 */

#ifndef	_DVFS_H_
#define	_DVFS_H_

#ifndef _ASMLANGUAGE

/*
 * dvfs levels
 */
#define DVFS_LEVEL_LOW                  (10)
#define DVFS_LEVEL_S2                   (20)
#define DVFS_LEVEL_NORMAL			    (30)
#define DVFS_LEVEL_PERFORMANCE		    (40)
#define DVFS_LEVEL_MID_PERFORMANCE		(50)
#define DVFS_LEVEL_HIGH_PERFORMANCE	    (60)

#define DVFS_EVENT_PRE_CHANGE		(1)
#define DVFS_EVENT_POST_CHANGE		(2)

struct dvfs_freqs {
	uint8_t state; /* DVFS_EVENT_PRE_CHANGE / DVFS_EVENT_POST_CHANGE */
	uint8_t old_level;
	uint8_t new_level;
};

struct dvfs_level {
	uint8_t level_id;
	uint8_t enable_cnt;
	uint16_t cpu_freq;
	uint16_t dsp_freq;
	uint16_t gpu_freq;
	uint16_t de_freq;
	uint16_t jpeg_freq;
	uint16_t vdd_volt;
};

#ifdef CONFIG_ACTS_DVFS_DYNAMIC_LEVEL

#ifdef CONFIG_DVFS_SECTION_CTX_IN_SRAM
#define __dvfs_notifier_data __in_section_unique(sleep.noinit)
#define __dvfs_notifier_func __in_section_unique(sleepfunc)
#else
#define __dvfs_notifier_data
#define __dvfs_notifier_func __ramfunc
#endif

struct dvfs_notifier {
	sys_dnode_t node;
	void (*dvfs_notify_func_t)(void *user_data, struct dvfs_freqs *dvfs_freq);
	void *user_data;
};

int dvfs_set_freq_table(struct dvfs_level *dvfs_level_tbl, int level_cnt);
int dvfs_get_current_level(void);
int dvfs_set_level(int level_id, const char *user_info);
int dvfs_unset_level(int level_id, const char *user_info);
struct dvfs_level *dvfs_get_info_by_level_id(int level_id);
int dvfs_register_notifier(struct dvfs_notifier *notifier);
int dvfs_unregister_notifier(struct dvfs_notifier *notifier);
int dvfs_force_set_level(int level_id, const char *user_info);
int dvfs_force_unset_level(int level_id, const char *user_info);
int dvfs_lock(void);
int dvfs_unlock(void);
#else

#define dvfs_set_freq_table(dvfs_level_tbl, level_cnt)		({ 0; })
#define dvfs_get_current_level()							({ 0; })
#define dvfs_set_level(level_id, user_info)					({ 0; })
#define dvfs_unset_level(level_id, user_info)				({ 0; })
#define dvfs_get_info_by_level_id(level_id)					({ 0; })
#define dvfs_register_notifier(notifier)					({ 0; })
#define dvfs_unregister_notifier(notifier)					({ 0; })
#define dvfs_force_set_level(level_id, user_info)({ 0; })
#define dvfs_force_unset_level(level_id, user_info)({ 0; })
#define dvfs_lock(void)({ 0; })
#define dvfs_unlock(void)({ 0; })
#endif	/* CONFIG_ACTS_DVFS_DYNAMIC_LEVEL */


#endif /* _ASMLANGUAGE */

#endif /* _DVFS_H_	*/
