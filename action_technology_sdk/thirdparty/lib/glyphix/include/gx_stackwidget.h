/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_layout.h"
#include "gx_widget.h"

namespace gx {
class StackWidget : public Widget {
    GX_OBJECT

public:
    explicit StackWidget(Widget *parent = nullptr);
};
} // namespace gx
