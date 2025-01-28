/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_prelude.h"

namespace gx {
class Object;
class Variant;
struct MetaEnum;
struct MetaStruct;
struct MetaMethod;
struct MetaProperty;

class MetaObject {
public:
    GX_NODISCARD const char *className() const { return metaData.className; }
    GX_NODISCARD const MetaObject *superClass() const { return metaData.superClass; }
    /**
     * Determine whether this class has a pure virtual function. Abstract classes cannot construct
     * objects.
     */
    GX_NODISCARD bool isAbstract() const { return !metaData.constructor; }
    /**
     * Constructs a new object, only used for non-abstract classes.
     * @param parent The parent object.
     * @return The new object, or nullptr when failure.
     */
    GX_NODISCARD Object *newObject(Object *parent) const;
    GX_NODISCARD int propertyCount() const { return metaData.propertyCount; }
    GX_NODISCARD int methodCount() const { return metaData.methodCount; }
    GX_NODISCARD const MetaProperty *property(int index) const;
    GX_NODISCARD const MetaProperty *property(const char *name) const;
    GX_NODISCARD const MetaMethod *method(int index) const;
    GX_NODISCARD const MetaMethod *method(const char *name) const;

    // private data
    const struct MetaData {
        const char *className;
        const MetaObject *superClass;
        const MetaProperty *properties;
        const MetaMethod *methods;
        const MetaEnum *enumerates;
        const MetaStruct *structs;
        Object *(*const constructor)(Object *);
        uint16_t propertyCount;
        uint16_t methodCount;
        uint16_t enumerateCount;
        uint16_t structCount;
    } metaData;
};

template<typename T> GX_NODISCARD Object *constructionObject(Object *parent) {
    return new T(parent);
}
} // namespace gx
