/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#include "gx_flashdb.h"
#include "flashdb.h"

namespace gx {
fdb_kvdb *fdb_kvdb_new() { return new fdb_kvdb(); }

void fdb_kvdb_delete(fdb_kvdb *db) {
    fdb_kvdb_deinit(db);
    delete db;
}
} // namespace gx
