/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_abstractslider.h"
#include "gx_signals.h"
#include "gx_widget.h"

namespace gx {
class Slider : public AbstractSlider {
    GX_OBJECT

public:
    explicit Slider(Widget *parent = nullptr);
    virtual ~Slider();

    GX_NODISCARD bool isVertical() const { return m_vertical; }
    void setVertical(bool status);
    GX_NODISCARD virtual Size sizeHint() const;
    GX_NODISCARD virtual bool ignoreStyleSize() const;

    GX_PROPERTY(bool vertical, get isVertical, set setVertical)

protected:
    virtual void paintEvent(PaintEvent *event);
    virtual bool gestureEvent(GestureEvent *event);

private:
    bool m_vertical;
};
} // namespace gx
