/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_fontloader.h"

namespace gx {
struct TrueTypeGlyphMetrics {
    // Q26.6 Fixed Format
    int left, top;
    int width, height;
    int xadvance, yadvance;
};

class TrueTypeStroker {
public:
    virtual ~TrueTypeStroker() {}
    virtual void moveTo(int x, int y) = 0;
    virtual void lineTo(int x, int y) = 0;
    virtual void conicTo(int x1, int y1, int x2, int y2) = 0;
    virtual void cubicTo(int x1, int y1, int x2, int y2, int x3, int y3) = 0;
};

class TrueTypeFace {
public:
    enum LoadFlags { NoFlags = 0, OnlyMetrics = 1 << 0 };
    GX_FLAGS(LoadFlags);

    virtual ~TrueTypeFace() {}
    virtual bool loadChar(uint32_t code, LoadFlags flags) = 0;
    virtual bool glyphOutline(TrueTypeStroker *stroker) = 0;
    virtual void glyphMetrics(TrueTypeGlyphMetrics *metrics) = 0;
    virtual int unitsPerEM() const = 0;
    virtual int ascender() const = 0;
    virtual int descender() const = 0;
};

class TrueTypeLibrary {
public:
    virtual ~TrueTypeLibrary() {}
    virtual TrueTypeFace *openFace(const String &faceName) = 0;
};

class FontLoaderTTF : public FontLoader {
public:
    explicit FontLoaderTTF(TrueTypeLibrary *lib);
    virtual ~FontLoaderTTF();
    virtual FontDriver *load(const String &face, const FontAttribute &attr);
};
} // namespace gx
