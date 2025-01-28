/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_signals.h"
#include "gx_variant.h"

namespace gx {
struct MetaProperty;
struct MetaMethod;

class Property {
public:
    enum Flags { URI = 0x0000001 };

    Property() : m_describe() {}
    Property(PrimitiveObject *object, const MetaProperty *describe);
    GX_NODISCARD bool isNull() const { return !m_describe || !m_object; }
    GX_NODISCARD Variant::type_t type() const;
    GX_NODISCARD const char *name() const;
    void set(const Variant &value) const;
    GX_NODISCARD Variant get() const;
    GX_NODISCARD Signal<> *signal() const;
    GX_NODISCARD int masks() const;
    GX_NODISCARD const MetaProperty *metaProperty() const { return m_describe; }

private:
    Pointer<PrimitiveObject> m_object;
    const MetaProperty *m_describe;
};

class Method {
public:
    Method() : m_describe() {}
    Method(PrimitiveObject *object, const MetaMethod *describe);
    GX_NODISCARD bool isNull() const { return !m_describe || !m_object; }
    GX_NODISCARD Variant::type_t type() const;
    GX_NODISCARD const char *name() const;
    Variant operator()(const Variant *args, int argc) const;
    GX_NODISCARD const MetaMethod *metaMethod() const { return m_describe; }

private:
    Pointer<PrimitiveObject> m_object;
    const MetaMethod *m_describe;
};
} // namespace gx
