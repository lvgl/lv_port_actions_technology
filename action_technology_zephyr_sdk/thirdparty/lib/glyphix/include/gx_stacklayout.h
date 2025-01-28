/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_layout.h"

namespace gx {
class StackLayout : public Layout {
public:
    explicit StackLayout(Widget *container = nullptr) : Layout(Vertical, container) {}
    GX_NODISCARD virtual Size sizeHint() const;
    GX_NODISCARD virtual int heightForWidth(int) const;

protected:
    virtual void layout(const Size &size);
};
} // namespace gx
