/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_object.h"
#include "gx_sharedref.h"
#include "gx_string.h"
#include "gx_texture.h"

namespace gx {
class VectorPath;

// designed for horizontal layout only!!!
struct GlyphMetrics {
    // Q26.6 Fixed Format
    int16_t left, top;
    int16_t height, xadvance;
};

struct FontAttribute {
    uint32_t style : 2;
    uint32_t underline : 1;
    uint32_t overline : 1;
    uint32_t weight : 8;
    uint32_t size : 16;
    uint16_t remSize;
    FontAttribute();
};

class GlyphBitmap : public Texture {
public:
    GlyphBitmap();
    ~GlyphBitmap();
    bool isValid() const { return data.flag1; }
    void setValid() { data.flag1 = 1; }
    /**
     * Release the glyph texture and don't reset metrics.
     * @return Number of texture bytes already released.
     **/
    std::size_t release();
    void resize(int width, int height, int pixfmt = PixelFormat::A8);
    uint32_t *allocPalette(int size);
    static void move(GlyphBitmap &dst, GlyphBitmap &src);
    GlyphMetrics metrics;

private:
    GlyphBitmap(const GlyphBitmap &);    // non-copyable
    void operator=(const GlyphBitmap &); // non-copyable
    friend class FontDriver;
};

class FontDriver : public SharedValue, public PrimitiveObject {
    GX_OBJECT

public:
    enum FontType { Vector = 1, Bitmap = 2 };
    enum AttributeRequest {
        SizeRequest = 0x01,
        WeightRequest = 0x02,
        StyleRequest = 0x04,
        UnderlineRequest = 0x08,
        OverlineRequest = 0x10,
        AllAttrsRequest = 0xffff
    };

    virtual ~FontDriver();
    GX_NODISCARD int type() const { return m_type; }
    GX_NODISCARD const String &faceName() const { return m_key.first; }
    GX_NODISCARD uint32_t faceHash() const { return m_hashcode; }
    GX_NODISCARD uint32_t hash() const;
    GX_NODISCARD const std::pair<String, FontAttribute> &key() const { return m_key; }
    GX_NODISCARD const FontAttribute &attributes() const { return m_key.second; }

    GX_NODISCARD virtual int baseline() const;
    virtual bool bitmapOf(uint32_t code, GlyphBitmap *bitmap) = 0;
    virtual bool metricsOf(uint32_t code, GlyphMetrics *metrics) = 0;
    virtual bool outlineOf(uint32_t code, VectorPath *outline);

    void request(const FontAttribute &attr, int req);
    bool operator==(const FontDriver &other) const;

    GX_NODISCARD static uint32_t hash(const String &face, const FontAttribute &attr);

    GX_NODISCARD virtual FontDriver *duplicate() const = 0;

protected:
    FontDriver(const String &family, int type);
    virtual void requestHandler(int req) = 0;
    static void copy(FontDriver &dst, const FontDriver &src);

private:
    FontDriver(const FontDriver &);
    FontDriver &operator=(const FontDriver &);

private:
    uint8_t m_type;
    uint32_t m_hashcode;
    std::pair<String, FontAttribute> m_key;
    friend class FontDriverCache;
};

inline bool operator==(const FontAttribute &a, const FontAttribute &b) {
    return a.style == b.style && a.overline == b.overline && a.underline == b.underline &&
           a.weight == b.weight && a.size == b.size && a.remSize == b.remSize;
}

inline bool operator!=(const FontAttribute &a, const FontAttribute &b) { return !(a == b); }
} // namespace gx
