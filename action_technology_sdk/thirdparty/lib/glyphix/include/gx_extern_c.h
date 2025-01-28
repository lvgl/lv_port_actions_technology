/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include <stdlib.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif
size_t gx_interrupt_disable();          // NOLINT(*-identifier-naming)
void gx_interrupt_enable(size_t level); // NOLINT(*-identifier-naming)

uint32_t gx_get_cur_time_ms();
#ifdef __cplusplus
}
#endif
