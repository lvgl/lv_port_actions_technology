/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_abstractscrollbar.h"

namespace gx {
class ScrollBar : public AbstractScrollBar {
    GX_OBJECT

public:
    explicit ScrollBar(Widget *parent = nullptr);
    virtual ~ScrollBar();

protected:
    virtual void paintEvent(PaintEvent *event);
};
} // namespace gx
