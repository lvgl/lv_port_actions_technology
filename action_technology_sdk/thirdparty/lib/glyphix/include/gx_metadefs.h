/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#define GX_OBJECT                                                                                  \
public:                                                                                            \
    struct MetaData__;                                                                             \
    virtual const ::gx::MetaObject *metaObject() const { return &staticMetaObject; }               \
    static const ::gx::MetaObject staticMetaObject;                                                \
                                                                                                   \
private:

#define GX_INCLUDE(...)
#define GX_PROPERTY(...)
#define GX_METHOD
#define GX_ENUM
#define GX_STRUCT
#define GX_ALIAS(...)
#define GX_HIDDEN

struct VarExport {
    const char *name;
    const void *value;
};

#define GX_VAR_EXPORT(name, value)
