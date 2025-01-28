/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_paintengine.h"

namespace gx {
class PixmapPaintDevice;

class PixmapPaintEngine : public PaintEngine {
public:
    explicit PixmapPaintEngine(PixmapPaintDevice *device);
    virtual ~PixmapPaintEngine();
    virtual void drawPoints(const Point *points, int count, const Pen &pen);
    virtual void drawLines(const Line *lines, int count, const Pen &pen);
    virtual void drawRects(const Rect *rects, int count, const Brush &brush);
    virtual void drawPath(const VectorPath &path, const Pen &pen);
    virtual void fillPath(const VectorPath &path, const Brush &brush);
    virtual void drawTexture(const Rect &dr, const Texture &, const Rect &sr, const Brush &br);

    template<typename T> static void setFactory();
    static PaintEngine *factory(PixmapPaintDevice *device) { return (*m_instance)(device); }

protected:
    PixmapPaintDevice *device;

private:
    struct Factory {
        virtual PaintEngine *operator()(PixmapPaintDevice *device) const = 0;
    };
    template<typename T> struct FactoryT : public Factory {
        virtual PaintEngine *operator()(PixmapPaintDevice *d) const { return new T(d); }
    };

    static Factory *m_instance;
};

template<typename T> void PixmapPaintEngine::setFactory() {
    static FactoryT<T> factory;
    m_instance = &factory;
}
} // namespace gx
