/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_geometry.h"
#include "gx_sharedref.h"
#include "gx_string.h"

namespace gx {
class FontDriver;

template<> struct utils::SharedHelper<FontDriver> {
    static FontDriver *duplicate(const SharedValue &x);
    static void destroy(SharedValue *x);
};

class Font {
public:
    enum Style { NormalStyle, ItalicStyle, ObliqueStyle };
    enum Weight {
        Thin = 1,       // 100
        ExtraLight = 2, // 200
        Light = 3,      // 300
        Regular = 4,    // 400
        Medium = 5,     // 500
        DemiBold = 6,   // 600
        Bold = 7,       // 700
        ExtraBold = 8,  // 800
        Black = 9       // 900
    };

    Font();
    explicit Font(const String &family, const Length &size = Length(), int weight = 400,
                  bool italic = false);
    GX_NODISCARD bool isNull() const { return !m_d; }
    ~Font();

    GX_NODISCARD String family() const;
    GX_NODISCARD float pixelSize() const;
    GX_NODISCARD float pointSize() const;
    GX_NODISCARD float baseline() const;
    GX_NODISCARD int pixelSize26Q6() const;
    GX_NODISCARD int baseline26Q6() const;

    void setFamily(const String &family);
    void setPixelSize(float size);
    void setPointSize(float size);
    void setSize(const Length &size);

    static Font fromDefault();

private:
    void updateAttrs(const struct FontAttribute &attr, int req);

private:
    SharedRef<FontDriver> m_d;
    friend class FontGlyphCache;
    friend class FontMetrics;
    friend class TextLayout;
    friend class VectorPath;
};
} // namespace gx
