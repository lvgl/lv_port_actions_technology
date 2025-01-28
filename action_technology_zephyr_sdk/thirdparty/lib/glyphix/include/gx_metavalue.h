/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_variant.h"

namespace gx {
struct MetaEnum {
    struct NamePair {
        int id;
        const char *name;
    };
    Variant::type_t type;
    int (*idCast)(const Variant &);
    Variant (*enumCast)(int);
    int size;
    const NamePair *enumMap;
    const NamePair *nameMap;
};

struct MetaStruct {
    struct Field {
        const char *name;
        Variant::type_t type;
    };
    Variant::type_t type;
    Variant (*cast)(const Variant *);
    Variant (*get)(const Variant &, int);
    int fieldCount;
    const Field *fields;
};
} // namespace gx
