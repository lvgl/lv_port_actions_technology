/*
 * Copyright (c) 2021 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _SOC_PSRAM_H_
#define _SOC_PSRAM_H_

#define SPI1_DELAYCHAIN_CLKOUT  14

void psram_self_refresh_control(bool low_refresh_en);
void psram_power_control(bool low_power_en);
void psram_delay_chain_set(uint8_t dqs, uint8_t dqs1, uint8_t clkout);

#endif /* _SOC_PSRAM_H_ */

