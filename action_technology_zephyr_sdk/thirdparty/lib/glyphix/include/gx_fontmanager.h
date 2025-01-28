/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_fontfacemap.h"
#include "gx_fontloader.h"
#include "intrusive/gx_cachemap.h"

namespace gx {
class FontLoader;

class FontManager {
    struct CacheHelper {
        typedef std::pair<String, FontAttribute> key_type;
        typedef std::size_t size_type;
        static const key_type &key(const FontDriver &value);
        static size_type size(const FontDriver &) { return 1; }
        static size_type release(const FontDriver &, size_type) { return 0; }
    };

public:
    typedef intrusive::CacheMap<FontDriver, SharedRef<FontDriver>, CacheHelper> FontDriverMap;

    FontManager();
    ~FontManager();
    void install(FontLoader *loader);

    GX_NODISCARD int driversCount() const { return static_cast<int>(m_drivers.count()); }
    GX_NODISCARD FontFaceMap &faces() { return m_faces; }
    GX_NODISCARD const FontFaceMap &faces() const { return m_faces; }
    GX_NODISCARD const FontDriverMap &drivers() const { return m_drivers; }
    GX_NODISCARD FontDriver *lookup(const String &family, const FontAttribute &attr);
    GX_NODISCARD FontDriver *load(const String &family, const FontAttribute &attr);
    GX_NODISCARD std::size_t driverCount() const { return m_drivers.count(); }
    void insert(FontDriver *driver);
    void remove(FontDriver *driver);
    void flush();

private:
    FontFaceMap m_faces;
    class FontLoaderFamily *m_loader;
    FontDriverMap m_drivers;
    friend class FontLoaderFamily;
};
} // namespace gx
