/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */

#include "sys/printk.h"
#include <assert.h>
#include "gx_assert.h"

void gx_assert_handler(const char *message, const char *file, unsigned line)
{
    printk("Assertion failed: %s in %s:%d\n", message, file, line);
    assert(0);
}
