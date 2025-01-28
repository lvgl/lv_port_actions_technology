/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file pinmux interface for Actions SoC
 */

#ifndef	_ACTIONS_SOC_PINMUX_H_
#define	_ACTIONS_SOC_PINMUX_H_

#ifndef _ASMLANGUAGE

#include <devicetree.h>

#define GPIO_PIN_DEFAULT_VAL 0x1000

struct acts_pin_config {
	unsigned int pin_num;
	unsigned int mode;
};

int acts_pinmux_set(unsigned int pin, unsigned int mode);
int acts_pinmux_get(unsigned int pin, unsigned int *mode);
void acts_pinmux_setup_pins(const struct acts_pin_config *pinconf, int pins);
void rom_pinmux_setup_pins(const struct acts_pin_config *pinconf, int pins);
void acts_pinctl_reg_setup_pins(const struct acts_pin_config *pinconf, int pins);

#define NODE_ID_PINCTRL_0(inst, i) \
	DT_INST_PHANDLE_BY_IDX(inst, pinctrl_0, i)

#if 0
#define ACTS_PIN_PROP_PINNUM(inst, i) \
		DT_PROP(NODE_ID_PINCTRL_0(inst,i), pinnum)
#define ACTS_PIN_PROP_MODES(inst, i) \
		DT_PROP(NODE_ID_PINCTRL_0(inst,i), modes)	

#define ACTS_DT_PIN(inst, i)				\
	{							\
		ACTS_PIN_PROP_PINNUM(inst, i),			\
		DT_PROP(NODE_ID_PINCTRL_0(inst,i), modes), \
	}
#endif

#define PIN_MFP_CONFIG(node)				\
	{.pin_num = DT_PROP(node, pinnum),		\
	 .mode = DT_PROP(node, modes) },


#define FOREACH_PIN_MFP(n) IF_ENABLED(DT_INST_NODE_HAS_PROP(n, pinctrl_0), (DT_FOREACH_CHILD(NODE_ID_PINCTRL_0(n, 0), PIN_MFP_CONFIG)))


#endif /* !_ASMLANGUAGE */

#endif /* _ACTIONS_SOC_PINMUX_H_	*/
