/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_signals.h"
#include "gx_widget.h"

namespace gx {
class AbstractButton : public Widget {
    GX_OBJECT

public:
    enum CheckState { Released, Pressed, Canceled };

    explicit AbstractButton(Widget *parent = nullptr);
    virtual ~AbstractButton();
    virtual void setCheckState(CheckState state);

    Signal<bool> checked;

protected:
    virtual bool gestureEvent(GestureEvent *event);
};
} // namespace gx
