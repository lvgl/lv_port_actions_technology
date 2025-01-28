/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#ifndef APP_RUNTIME_VAR_EXPORT_H
#define APP_RUNTIME_VAR_EXPORT_H

#include "gx_metadefs.h"

#include <limits.h>
#include <stdint.h>

#define VE_NOT_FOUND 0 /* not found */

typedef struct VarExport ve_exporter_t;

/* module object */
struct ve_module {
    const ve_exporter_t *const *begin; /* the first module of the same name */
    const ve_exporter_t *const *end;   /* the last module of the same */
};

typedef struct ve_module ve_module_t;

/* iterator object */
struct ve_iterator {
    const ve_exporter_t *const *exp_index; /* iterator index */
    const ve_exporter_t *const *exp_end;   /* iterate over exporter */
};

typedef struct ve_iterator ve_iterator_t;

/* initialize module */
int ve_module_init(ve_module_t *mod, const char *module);

/* iterate backward */
const ve_exporter_t *ve_iter_next(ve_iterator_t *iter);

/* initialize iterator */
void ve_iter_init(ve_module_t *mod, ve_iterator_t *iter);

/* get the value by identifier */
intptr_t ve_value_get(ve_module_t *mod, const char *identifier);

#endif // APP_RUNTIME_VAR_EXPORT_H
