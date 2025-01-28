/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_framework.h"

struct fdb_kvdb;

namespace gx {
fdb_kvdb *fdb_kvdb_new();
void fdb_kvdb_delete(fdb_kvdb *db);
} // namespace gx
