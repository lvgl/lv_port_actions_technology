/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_checkbox.h"

namespace gx {
class Switch : public CheckBox {
    GX_OBJECT

public:
    explicit Switch(Widget *parent = nullptr);
    virtual ~Switch();

    GX_NODISCARD virtual Size sizeHint() const;

protected:
    virtual void paintEvent(PaintEvent *event);
};
} // namespace gx
