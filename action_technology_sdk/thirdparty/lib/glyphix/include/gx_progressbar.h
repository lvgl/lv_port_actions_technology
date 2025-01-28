/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_abstractprogressbar.h"

namespace gx {
class ProgressBar : public AbstractProgressBar {
    GX_OBJECT

public:
    explicit ProgressBar(Widget *parent = nullptr);
    virtual ~ProgressBar();

    GX_NODISCARD bool isVertical() const { return m_vertical; }
    void setVertical(bool status);
    GX_NODISCARD virtual Size sizeHint() const;

    GX_PROPERTY(bool vertical, get isVertical, set setVertical)

protected:
    virtual void paintEvent(PaintEvent *event);

private:
    bool m_vertical;
};
} // namespace gx
