/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_variant.h"

namespace gx {
class SignalBase;
class PrimitiveObject;

struct MetaProperty {
    const char *name;
    Variant::type_t type;
    Variant (*get)(PrimitiveObject *);
    void (*set)(PrimitiveObject *, const Variant &);
    SignalBase *(*signal)(PrimitiveObject *);
    uint32_t masks;
};

struct MetaMethod {
    const char *name;
    Variant::type_t type;
    int argc;
    Variant (*call)(PrimitiveObject *, const Variant *, int);
};
} // namespace gx
