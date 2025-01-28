/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_string.h"

namespace gx {
//! @brief The brief applet manifest information.
class BriefManifest {
public:
    BriefManifest() : m_versionCode(), m_dialPackage() {}
    GX_NODISCARD const String &name() const { return m_name; }
    GX_NODISCARD const String &package() const { return m_package; }
    GX_NODISCARD const String &versionName() const { return m_versionName; }
    GX_NODISCARD int versionCode() const { return m_versionCode; }
    GX_NODISCARD bool isDialPackage() const { return m_dialPackage; }
    GX_NODISCARD const String &icon() const { return m_icon; }
    GX_NODISCARD String iconUri() const;

private:
    String m_package;
    String m_name;
    String m_icon;
    String m_versionName;
    int m_versionCode;
    bool m_dialPackage;

    friend class Applet;
};
} // namespace gx
