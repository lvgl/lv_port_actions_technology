/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_jsvm.h"

namespace gx {
enum AppletDatabaseTable { ADBT_Applet, ADBT_Dial };

class PackageDatabase {
public:
    PackageDatabase();
    virtual ~PackageDatabase();

    GX_NODISCARD int version() const { return versionNumber; };
    GX_NODISCARD int table() const { return m_table; }
    void setTable(int table) { m_table = table; }

    GX_NODISCARD virtual int count() const = 0;
    GX_NODISCARD virtual Vector<String> keys() const = 0;
    GX_NODISCARD virtual bool contains(const String &string) const = 0;
    GX_NODISCARD virtual JsValue get(const String &string) const = 0;
    virtual bool set(const String &string, const JsValue &data) = 0;
    virtual bool erase(const String &string) = 0;

protected:
    int versionNumber;

private:
    int m_table;
};
} // namespace gx
