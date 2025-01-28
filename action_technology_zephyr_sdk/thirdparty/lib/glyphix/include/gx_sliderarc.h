/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_abstractslider.h"
#include "gx_signals.h"
#include "gx_styleengine.h"
#include "gx_widget.h"

namespace gx {
class SliderArc : public AbstractSlider {
    GX_OBJECT

public:
    explicit SliderArc(Widget *parent = nullptr);
    virtual ~SliderArc();

    GX_NODISCARD virtual Size sizeHint() const;

    void setStartAngle(float value);
    void setProgressAngle(float value);
    void setArcCenter(Point arcCenter);
    void setArcWidth(int width);
    void setArcRadius(int radius);

    GX_NODISCARD float startAngle() const { return m_arc.startAngle; }
    GX_NODISCARD float progressAngle() const { return m_progressAngle; }
    GX_NODISCARD Point arcCenter() const { return m_arc.center; }
    GX_NODISCARD int arcWidth() const { return m_arc.width; }
    GX_NODISCARD int arcRadius() const { return m_radius; }

    GX_PROPERTY(float startAngle, get startAngle, set setStartAngle)
    GX_PROPERTY(float progressAngle, get progressAngle, set setProgressAngle)
    GX_PROPERTY(Point arcCenter, get arcCenter, set setArcCenter)
    GX_PROPERTY(int arcWidth, get arcWidth, set setArcWidth)
    GX_PROPERTY(int arcRadius, get arcRadius, set setArcRadius)

protected:
    virtual void paintEvent(PaintEvent *event);
    virtual bool gestureEvent(GestureEvent *event);

private:
    struct Arc {
        PointF center;
        int radiusX;
        int radiusY;
        float startAngle;
        float middleAngle;
        float stopAngle;
        int width;
    };
    Arc m_arc;
    float m_progressAngle;
    int m_radius;
};
} // namespace gx
