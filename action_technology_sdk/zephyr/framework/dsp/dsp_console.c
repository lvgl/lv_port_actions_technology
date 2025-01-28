/*
 * Copyright (c) 1997-2015, Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <shell/shell.h>
#include <assert.h>
#include <errno.h>
#include <devicetree.h>
#include <string.h>
#include <mem_manager.h>
#include "dsp_inner.h"
#include <stdlib.h>

extern int dsp_console_set_cmd(uint16_t id, uint32_t param1, uint32_t param2);

int shell_cmd_dsp_console(const struct shell *shell_p, size_t argc, char **argv)
{
	unsigned long writeval;
	unsigned long addr, count;
	int ret;

	if (argc < 2) {
		return -EINVAL;
	}

	if (!strcmp(argv[1], "help")) {
		printk("Dsp Console Commands:\n");
		printk("dsp dspload       :show dsp load statistic preriodically\n");
		printk("dsp mdw           :display memory by word: mdw address [,count]\n");
		printk("dsp mdh           :display memory by half-word: mdh address [,count]\n");
		printk("dsp mww           :memory write (fill) by word: mww address value [,count]\n");
		printk("dsp mwh           :memory write (fill) by half-word: mwh address value [,count]\n");
		printk("dsp rin           :register in: rin address\n");
		printk("dsp rout          :register out: rout address value\n");
		return 0;
	}

	if (!strcmp(argv[1], "dspload")) {
		int start_f = true;
		int interval_ms = 2000;
		
		if (argc < 3) {
			return -EINVAL;
		}
		
		if (!strcmp(argv[2], "start")) {
			if (argc > 3)
				interval_ms = strtoul(argv[2], NULL, 0);
			printk("Start dsp load statistic, interval %d ms\n", interval_ms);
		} else if (!strcmp(argv[2], "stop")) {
			printk("Stop cpu load statistic\n");
			start_f = false;
		} else {
			printk("usage:\n");
			printk("  dspload start [2000]\n");
			printk("  dspload stop\n");

			return -EINVAL;
		}
		
		ret = dsp_console_set_cmd(DSP_CMD_CONSOLE_DSPLOAD, start_f, interval_ms);
	} else if (!strcmp(argv[1], "mdw")) {
		if (argc < 4) {
			return -EINVAL;
		}
		
		addr = strtoul(argv[2], NULL, 16);
		if (!is_valid_dsp_data_address(addr) || (addr & 0x01)) {
			printk("invalid dsp data addr:%lx\n", addr);
			return -EINVAL;
		}
		count = strtoul(argv[3], NULL, 0);
		if (count > 16) {
			count = 16;
		}
		if (count == 0) {
			return -EINVAL;
		}

		ret = dsp_console_set_cmd(DSP_CMD_CONSOLE_MDW, addr, count);
	} else if (!strcmp(argv[1], "mdh")) {
		if (argc < 4) {
			return -EINVAL;
		}
		
		addr = strtoul(argv[2], NULL, 16);
		if (!is_valid_dsp_data_address(addr)) {
			printk("invalid dsp data addr:%lx\n", addr);
			return -EINVAL;
		}
		count = strtoul(argv[3], NULL, 0);
		if (count > 16) {
			count = 16;
		}
		if (count == 0) {
			return -EINVAL;
		}

		ret = dsp_console_set_cmd(DSP_CMD_CONSOLE_MDH, addr, count);
	} else if (!strcmp(argv[1], "mww")) {
		if (argc < 4) {
			return -EINVAL;
		}
		
		addr = strtoul(argv[2], NULL, 16);
		if (!is_valid_dsp_data_address(addr) || (addr & 0x1)) {
			printk("invalid dsp data addr:%lx\n", addr);
			return -EINVAL;
		}
		writeval = strtoul(argv[3], NULL, 16);
		
		ret = dsp_console_set_cmd(DSP_CMD_CONSOLE_MWW, addr, writeval);
	} else if (!strcmp(argv[1], "mwh")) {
		if (argc < 4) {
			return -EINVAL;
		}
		
		addr = strtoul(argv[2], NULL, 16);
		if (!is_valid_dsp_data_address(addr)) {
			printk("invalid dsp data addr:%lx\n", addr);
			return -EINVAL;
		}
		writeval = strtoul(argv[3], NULL, 16);
		
		ret = dsp_console_set_cmd(DSP_CMD_CONSOLE_MWH, addr, writeval);
	} else if (!strcmp(argv[1], "rin")) {
		if (argc < 3) {
			return -EINVAL;
		}
		
		addr = strtoul(argv[2], NULL, 16);
		if (addr & 0x03) {
			printk("invalid dsp register addr:%lx\n", addr);
			return -EINVAL;
		}

		ret = dsp_console_set_cmd(DSP_CMD_CONSOLE_RIN, addr, 0);
	} else if (!strcmp(argv[1], "rout")) {
		if (argc < 4) {
			return -EINVAL;
		}
		
		addr = strtoul(argv[2], NULL, 16);
		if (addr & 0x3) {
			printk("invalid dsp register addr:%lx\n", addr);
			return -EINVAL;
		}
		writeval = strtoul(argv[3], NULL, 16);
		
		ret = dsp_console_set_cmd(DSP_CMD_CONSOLE_ROUT, addr, writeval);
	} else {
		return -EINVAL;
	}
	
	return 0;
}

