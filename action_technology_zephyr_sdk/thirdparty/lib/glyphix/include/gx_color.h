/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_sharedref.h"
#include "gx_string.h"
#include "gx_vector.h"

namespace gx {
class Image;
class Pixmap;
class Transform;

/**
 * The Color class supports defining color values using the RGB color space.
 */
class Color {
public:
    /// Built-in standard color definitions.
    enum Preset { Black, Blue, Cyan, Gray, Green, Magenta, Red, Transparent, White, Yellow };

    Color() : m_argb(0xff000000) {}
    Color(Preset preset);
    explicit Color(const char *name);
    explicit Color(const String &name);
    explicit Color(uint32_t value) : m_argb(value) {}
    Color(int r, int g, int b, int a = 255);
    GX_NODISCARD bool isTransparent() const { return m_argb < 0x01000000; }
    GX_NODISCARD bool isOpaque() const { return m_argb >= 0xff000000; }
    GX_NODISCARD uint32_t argb() const { return m_argb; }
    GX_NODISCARD int alpha() const;
    GX_NODISCARD int red() const;
    GX_NODISCARD int green() const;
    GX_NODISCARD int blue() const;
    /**
     * Gets the hexadecimal encoded string of the colour.
     * @return The encoding format is ``#RRGGBB[AA]``. The Alpha channel is only included if the
     * value is not 255.
     */
    GX_NODISCARD String name() const;
    GX_NODISCARD Color blend(int alpha) const;
    GX_NODISCARD Color blend(const Color &color) const;
    GX_NODISCARD Color blend(const Color &target, float progress) const;

    Color operator*(int alpha) const { return blend(alpha); }
    Color operator*(const Color &color) const { return blend(color); }
    bool operator==(const Color &color) const { return m_argb == color.m_argb; }
    bool operator!=(const Color &color) const { return !(*this == color); }

private:
    uint32_t m_argb;
};

class Gradient {
public:
    enum Type { None, Linear, Radial, Conical };
    struct Stop {
        int position; // Q16.16
        Color color;
        Stop() : position() {}
        Stop(int position, const Color &color) : position(position), color(color) {}
    };

    Gradient();
    Gradient(const Gradient &other);
    ~Gradient();
    GX_NODISCARD Type type() const;
    void setStop(float position, const Color &color);
    GX_NODISCARD const Vector<Stop> &stops() const;

    Gradient &operator=(const Gradient &rhs);

protected:
    class Data;
    inline explicit Gradient(Data *d);
    inline bool detach();
    template<class T> GX_NODISCARD T *data() { return static_cast<T *>(m_d.get()); }
    template<class T> GX_NODISCARD const T *data() const {
        return static_cast<const T *>(m_d.get());
    }

private:
    SharedRef<Data> m_d;
};

class Brush {
public:
    enum { Color, Image, Pixmap, Gradient };
    Brush(gx::Color color = gx::Color());
    Brush(gx::Color::Preset color);
    explicit Brush(const gx::Image &image);
    explicit Brush(const gx::Gradient &gradient);
    explicit Brush(const gx::Pixmap &pixmap);
    Brush(const Brush &brush);
    ~Brush();
    GX_NODISCARD int type() const { return m_type & 0x7f; }
    GX_NODISCARD bool isTransformed() const { return m_type & 0x80; }
    GX_NODISCARD gx::Color color() const;
    GX_NODISCARD gx::Image image() const;
    GX_NODISCARD gx::Pixmap pixmap() const;
    GX_NODISCARD gx::Gradient gradient() const;
    GX_NODISCARD const Transform *transform() const;
    void setColor(gx::Color color);
    void setImage(const gx::Image &image);
    void setPixmap(const gx::Pixmap &pixmap);
    void setGradient(const gx::Gradient &gradient);
    void setTransform(const Transform &transform);

    Brush &operator=(const Brush &brush);

private:
    class ExtraData;
    typedef SharedRef<ExtraData> ExtraRef;
    template<class T> GX_NODISCARD const T &data() const {
        return *reinterpret_cast<const T *>(m_buf.p);
    }
    template<class T> GX_NODISCARD T &data() { return *reinterpret_cast<T *>(m_buf.p); }
    GX_NODISCARD ExtraRef &extraData() { return data<ExtraRef>(); }
    GX_NODISCARD const ExtraRef &extraData() const { return data<ExtraRef>(); }
    void release();

    uint8_t m_type;
    union {
        uint8_t p[1];
        intptr_t i;
    } m_buf;
};

class Pen {
public:
    enum JoinMode { BevelJoin = 0, MiterJoin = 1, RoundJoin = 3 };
    enum CapMode { FlatCap = 0, SquareCap = 2, RoundCap = 4 };

    Pen(const Brush &brush = Brush(), float size = 1, CapMode cap = FlatCap,
        JoinMode join = BevelJoin);
    Pen(Color color, float size = 1, CapMode cap = FlatCap, JoinMode join = BevelJoin);
    Pen(Color::Preset color, float size = 1, CapMode cap = FlatCap, JoinMode join = BevelJoin);
    GX_NODISCARD Color color() const { return m_brush.color(); }
    GX_NODISCARD float size() const { return m_size; }
    GX_NODISCARD JoinMode joinMode() const { return JoinMode(m_joinMode); }
    GX_NODISCARD CapMode capMode() const { return CapMode(m_capMode); }
    GX_NODISCARD const Brush &brush() const { return m_brush; }
    void setColor(Color color) { m_brush.setColor(color); }
    void setSize(float size) { m_size = size; }
    void setLineJoin(JoinMode mode) { m_joinMode = mode; }
    void setLineCap(CapMode mode) { m_capMode = mode; }
    void setBrush(const Brush &brush) { m_brush = brush; }

private:
    Brush m_brush;
    float m_size;
    uint32_t m_joinMode : 3;
    uint32_t m_capMode : 3;
};

const Logger &operator<<(const Logger &, const Color &);
} // namespace gx
