/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_color.h"
#include "gx_geometry.h"

namespace gx {
class Pen;
class Brush;
class Texture;
class Transform;
class VectorPath;
class PaintDevice;
class Font;

class PaintEngine {
public:
    virtual ~PaintEngine();
    virtual void drawPoints(const Point *points, int count, const Pen &pen) = 0;
    virtual void drawLines(const Line *lines, int count, const Pen &pen) = 0;
    virtual void drawRects(const Rect *rects, int count, const Brush &brush) = 0;
    virtual void drawPath(const VectorPath &path, const Pen &pen) = 0;
    virtual void fillPath(const VectorPath &path, const Brush &brush) = 0;
    virtual void drawTexture(const Rect &dr, const Texture &, const Rect &sr, const Brush &) = 0;

    GX_NODISCARD const Rect &clip() const { return m_clip; }
    GX_NODISCARD const Point &origin() const { return m_origin; }
    GX_NODISCARD const Transform *transform() const { return m_transform; }
    GX_NODISCARD int opacity() const { return m_opacity; }
    GX_NODISCARD int transferMode() const { return m_transferMode; }

protected:
    PaintEngine();

private:
    Rect m_clip;                  // clip area
    Point m_origin;               // paint origin
    const Transform *m_transform; // transform matrix
    uint16_t m_opacity;           // constant alpha
    uint16_t m_transferMode;      // transfer mode
    friend class Painter;
};
} // namespace gx
