/********************************************************************************
 *                            USDK(ZS283A)
 *                            Module: SYSTEM
 *                 Copyright(c) 2003-2017 Actions Semiconductor,
 *                            All Rights Reserved.
 *
 * History:
 *      <author>      <time>                      <version >          <desc>
 *      wuyufan   2019-2-18-3:20:35             1.0             build this file
 ********************************************************************************/
/*!
 * \file     temperature_compensation.c
 * \brief
 * \author
 * \version  1.0
 * \date  2019-2-18-3:20:35
 *******************************************************************************/
#include <kernel.h>
#include <init.h>
#include <soc.h>
#include "temp_comp.h"
#include <soc_atp.h>
#include <compensation.h>
#include <string.h>
#include <stdlib.h>

#define SYS_LOG_DOMAIN "cap comp"

#include <logging/log.h>
LOG_MODULE_REGISTER(cap_comp, CONFIG_LOG_DEFAULT_LEVEL);

extern void ipmsg_btc_soc_set_hosc_cap(int cap);

static cap_temp_comp_ctrl_t cap_temp_comp_ctrl;

#if 0		/* Sample rable relate to crystal oscillator */
// <"temperature compensation", CFG_CATEGORY_BLUETOOTH>
static const CFG_Struct_Cap_Temp_Comp CFG_Cap_Temp_Comp_Tbl =
{
	// <"enable temperature compensation", CFG_TYPE_BOOL>
	.Enable_Cap_Temp_Comp = true,
	// <"temperature compensation", CFG_Type_Cap_Temp_Comp>
	.Table =
	{
		{ CAP_TEMP_N_40, (0x100 - (1.6f * 10))},			/* HOSONIC crystal oscillator */
		{ CAP_TEMP_N_30, (0x100 - (0.6f * 10))},
		{ CAP_TEMP_N_20, (0.3f * 10)},
		{ CAP_TEMP_0,	 (0.7f * 10)},
		{ CAP_TEMP_P_25, (0.0f * 10)},
		{ CAP_TEMP_P_45, (0x100 - (0.6f * 10))},
		{ CAP_TEMP_P_65, (0x100 - (0.6f * 10))},
		{ CAP_TEMP_P_80, (0.0f * 10)},

		{ CAP_TEMP_NA,   0.0f * 10 },
		{ CAP_TEMP_NA,   0.0f * 10 }
	},
};
#elif 0
// <"temperature compensation", CFG_CATEGORY_BLUETOOTH>
static const CFG_Struct_Cap_Temp_Comp CFG_Cap_Temp_Comp_Tbl =
{
	// <"enable temperature compensation", CFG_TYPE_BOOL>
	.Enable_Cap_Temp_Comp = true,
	// <"temperature compensation", CFG_Type_Cap_Temp_Comp>
	.Table =
	{
		{ CAP_TEMP_N_40, (0x100 - (2.0f * 10))},			/* SEIKO crystal oscillator */
		{ CAP_TEMP_N_20, (0x100 - (0.6f * 10))},
		{ CAP_TEMP_N_10, (0x100 - (0.2f * 10))},
		{ CAP_TEMP_0,	 (0x100 - (0.1f * 10))},
		{ CAP_TEMP_P_10, (0x100 - (0.2f * 10))},
		{ CAP_TEMP_P_40, (0x100 - (0.8f * 10))},
		{ CAP_TEMP_P_60, (0x100 - (0.9f * 10))},
		{ CAP_TEMP_P_80, (0x100 - (0.1f * 10))},

		{ CAP_TEMP_NA,   0.0f * 10 },
		{ CAP_TEMP_NA,   0.0f * 10 }
	},
};
#else
static const CFG_Struct_Cap_Temp_Comp CFG_Cap_Temp_Comp_Tbl =
{
	// <"enable temperature compensation", CFG_TYPE_BOOL>
	.Enable_Cap_Temp_Comp = true,
	// <"temperature compensation", CFG_Type_Cap_Temp_Comp>
	.Table =
	{
		{ CAP_TEMP_N_40, 0.0f * 10 },
		{ CAP_TEMP_N_20, 0.0f * 10 },
		{ CAP_TEMP_P_5,  0.0f * 10 },
		{ CAP_TEMP_P_25, 0.0f * 10 },
		{ CAP_TEMP_P_45, 0.0f * 10 },
		{ CAP_TEMP_P_60, 0.0f * 10 },
		{ CAP_TEMP_P_80, 0.0f * 10 },
		{ CAP_TEMP_NA,   0.0f * 10 },
		{ CAP_TEMP_NA,   0.0f * 10 },
		{ CAP_TEMP_NA,   0.0f * 10 }
	},
};
#endif

/*temperature compensation process */
static void cap_temp_comp_proc(int temp)
{
	cap_temp_comp_ctrl_t *p = &cap_temp_comp_ctrl;
	int  comp = 0;

	LOG_DBG("cur temp %d, last temp %d", temp, p->last_temp);

	if (abs(temp - p->last_temp) <= 2) {
		return;
	}

	p->last_temp = temp;
	if (temp <= (s8_t)p->configs.Table[0].Cap_Temp) {
		comp = (s8_t)p->configs.Table[0].Cap_Comp;
	} else if (temp >= (s8_t)p->configs.Table[p->table_size - 1].Cap_Temp) {
		comp = (s8_t)p->configs.Table[p->table_size - 1].Cap_Comp;
	} else {
		int  i;

		for (i=0; i<(p->table_size - 1); i++) {
			CFG_Type_Cap_Temp_Comp*  t1 = &p->configs.Table[i];
			CFG_Type_Cap_Temp_Comp*  t2 = &p->configs.Table[i + 1];

			if (temp >= (s8_t)t1->Cap_Temp && temp <  (s8_t)t2->Cap_Temp) {
				comp = (temp - (s8_t)t1->Cap_Temp) *
				((s8_t)t2->Cap_Comp - (s8_t)t1->Cap_Comp) /
				((s8_t)t2->Cap_Temp - (s8_t)t1->Cap_Temp) +
				(s8_t)t1->Cap_Comp;
				break;
			}
		}
	}

	LOG_INF("temp %d, cap base %d comp %s%d.%d", temp, p->normal_cap,
				((comp < 0) ? "-" : ""), abs(comp) / 10, abs(comp) % 10);
	ipmsg_btc_soc_set_hosc_cap((int)p->normal_cap + comp);
}

void cap_temp_do_comp(int temp)
{
	if (!cap_temp_comp_ctrl.enabled) {
		return;
	}

	cap_temp_comp_proc(temp);
}

/* Hosc cap temperature compensate.
 * base_cap : base cap
 */
int cap_temp_comp_init(uint8_t base_cap)
{
	int  i;

	memset(&cap_temp_comp_ctrl, 0, sizeof(cap_temp_comp_ctrl));
	cap_temp_comp_ctrl.normal_cap = base_cap;
	memcpy(&cap_temp_comp_ctrl.configs, &CFG_Cap_Temp_Comp_Tbl, sizeof(CFG_Struct_Cap_Temp_Comp));

	for (i=0; i<CFG_MAX_CAP_TEMP_COMP; i++) {
		if (cap_temp_comp_ctrl.configs.Table[i].Cap_Temp == CAP_TEMP_NA) {
			break;
		}
		cap_temp_comp_ctrl.table_size += 1;
	}

	cap_temp_comp_ctrl.last_temp = CAP_TEMP_NA;
	if (cap_temp_comp_ctrl.table_size == 0) {
		return -1;
	}

	if (!cap_temp_comp_ctrl.configs.Enable_Cap_Temp_Comp) {
		return -1;
	}

	cap_temp_comp_ctrl.enabled = true;
	return 0;
}
