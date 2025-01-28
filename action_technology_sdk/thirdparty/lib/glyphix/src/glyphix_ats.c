/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */

#include "gx_lvgx.h"

int glyphix_ats_init() {
  return 0;
}

int glyphix_ats_start() {
#ifdef CONFIG_BLUETOOTH
  extern int mas_start();
  mas_start();
  extern void gx_time_sync_init();
  gx_time_sync_init();
#endif

  return 0;
}

int glyphix_ats_finish() {
#ifdef CONFIG_BLUETOOTH

#endif

  return 0;
}
