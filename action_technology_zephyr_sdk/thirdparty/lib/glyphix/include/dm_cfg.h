/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#ifndef DM_CFG_H_
#define DM_CFG_H_

#include "stdio.h"
#include "stdint.h"
#include "gx_assert.h"

/* data manager using key-value db to storage data */
#define DM_USING_STORAGE

/* data manager using async io to storage data */
#define DM_USING_AIO

/* data manager using default assert interface */
#define DM_ASSERT   GX_ASSERT

#endif
