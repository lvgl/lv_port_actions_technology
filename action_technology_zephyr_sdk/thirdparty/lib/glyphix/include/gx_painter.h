/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_color.h"
#include "gx_font.h"
#include "gx_geometry.h"
#include "gx_transform.h"

namespace gx {
class Image;
class Texture;
class PaintDevice;
class VectorPath;
class Widget;
class TextLayout;

class Painter {
public:
    enum TransferMode {
        Clear = 0,
        Src,
        Dst,
        SrcOver,
        DstOver,
        SrcIn,
        DstIn,
        SrcOut,
        DstOut,
        UnknownTransferMode
    };
    class Helper;

    explicit Painter(Widget *widget);
    explicit Painter(PaintDevice *device);
    Painter(PaintDevice *device, const Rect &clip);
    ~Painter();
    GX_NODISCARD int opacity() const;
    GX_NODISCARD Rect clip() const;
    GX_NODISCARD const Pen &pen() const { return m_pen; }
    GX_NODISCARD const Brush &brush() const { return m_brush; }
    GX_NODISCARD const Font &font() const { return m_font; }
    GX_NODISCARD const Transform &transform() { return m_transform; }
    GX_NODISCARD float lineHeight() const;
    GX_NODISCARD TransferMode transferMode() const;
    GX_NODISCARD LayoutDirection layoutDirection() const;
    void setOpacity(int opacity);
    void setClip(const Rect &rect);
    void setPen(const Pen &pen) { m_pen = pen; }
    void setBrush(const Brush &brush) { m_brush = brush; }
    void setFont(const Font &font) { m_font = font; }
    void setTransform(const Transform &transform);
    void setLineHeight(float lineHeight);
    void setTransferMode(TransferMode mode);
    void setLayoutDirection(LayoutDirection direction);
    void drawArc(const PointF &position, float radius, float startAngle, float stopAngle);
    void drawArc(const PointF &position, float radius, float startAngle, float stopAngle,
                 const Pen &pen);
    void drawArc(float x, float y, float radius, float startAngle, float stopAngle);
    void drawArc(float x, float y, float radius, float startAngle, float stopAngle, const Pen &pen);
    void drawPoint(const Point &point);
    void drawPoint(const Point &point, const Pen &pen);
    void drawLine(const Point &p1, const Point &p2);
    void drawLine(const Point &p1, const Point &p2, const Pen &pen);
    void drawLine(const Line &line);
    void drawLine(const Line &line, const Pen &pen);
    void fillRect(const Rect &rect);
    void fillRect(const Rect &rect, const Brush &brush);
    void fillRoundedRect(const RectF &rect, float radius);
    void fillRoundedRect(const RectF &rect, float radiusTopLeft, float radiusTopRight,
                         float radiusBottomLeft, float radiusBottomRight);
    void drawRect(const Rect &rect);
    void drawRect(const Rect &rect, const Pen &pen);
    void drawRoundedRect(const RectF &rect, float radius);
    void drawRoundedRect(const RectF &rect, float radiusTopLeft, float radiusTopRight,
                         float radiusBottomLeft, float radiusBottomRight);
    void drawTexture(const Rect &dstRect, const Texture &texture);
    void drawTexture(const Rect &dstRect, const Texture &texture, const Rect &srcRect);
    void drawTexture(const Point &point, const Texture &texture, const Rect &srcRect);
    void drawTexture(const Point &point, const Texture &texture);
    void drawImage(const Rect &dstRect, const Image &image);
    void drawImage(const Rect &dstRect, const Image &image, const Rect &srcRect);
    void drawImage(const Point &point, const Image &image, const Rect &srcRect);
    void drawImage(const Point &point, const Image &image);
    void drawPath(const VectorPath &path);
    void drawPath(const VectorPath &path, const Pen &pen);
    void fillPath(const VectorPath &path);
    void fillPath(const VectorPath &path, const Brush &brush);
    void drawText(const Point &position, const String &text);
    void drawText(const Rect &rect, const String &text, int flags = 0, int maxLines = 1);
    void drawText(const Rect &rect, const TextLayout &layout);

private:
    void initialization();

private:
    class PaintEngine *m_engine;
    class PaintDevice *m_device;
    Pen m_pen;
    Brush m_brush;
    Font m_font;
    Transform m_transform;
    Rect m_clip;
    uint32_t m_lineHeight : 30;
    uint32_t m_layoutDir : 2;
};
} // namespace gx
