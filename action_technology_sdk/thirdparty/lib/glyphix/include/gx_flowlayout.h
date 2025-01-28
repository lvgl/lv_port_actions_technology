/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_layout.h"

namespace gx {
class FlowLayout : public Layout {
public:
    explicit FlowLayout(Widget *container = nullptr) : Layout(Vertical, container) {}

    using Layout::layout;
    GX_NODISCARD virtual Size sizeHint() const;
    GX_NODISCARD virtual int heightForWidth(int) const;

protected:
    virtual void layout(const Size &size);

private:
    int process(const Size &size, bool layout);
};
} // namespace gx
