/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_widget.h"

namespace gx {
class Window : public Widget {
    GX_OBJECT

public:
    explicit Window(Widget *parent = nullptr);

protected:
    virtual void paintEvent(PaintEvent *event);
};
} // namespace gx
