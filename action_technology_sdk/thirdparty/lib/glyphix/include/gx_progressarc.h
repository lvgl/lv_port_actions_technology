/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_abstractprogressbar.h"
#include "gx_label.h"
#include "gx_paintdevice.h"
#include "gx_slider.h"

namespace gx {
class ProgressArc : public AbstractProgressBar {
    GX_OBJECT

public:
    explicit ProgressArc(Widget *parent = nullptr);
    virtual ~ProgressArc();
    GX_NODISCARD float startAngle() const { return m_startAngle; }
    GX_NODISCARD float stopAngle() const { return m_stopAngle; }
    void setStartAngle(float angle);
    void setStopAngle(float angle);

    GX_NODISCARD virtual Size sizeHint() const;

    GX_PROPERTY(float startAngle, get startAngle, set setStartAngle)
    GX_PROPERTY(float stopAngle, get stopAngle, set setStopAngle)

protected:
    virtual void paintEvent(PaintEvent *event);

private:
    float m_startAngle, m_stopAngle;
};
} // namespace gx
