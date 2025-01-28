/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_animation.h"
#include "gx_widget.h"

namespace gx {
class Transitions {
public:
    virtual ~Transitions();
    virtual AbstractAnimation *openEnter(Widget *widget) = 0;
    virtual AbstractAnimation *closeEnter(Widget *widget) = 0;
    virtual AbstractAnimation *openExit(Widget *widget) = 0;
    virtual AbstractAnimation *closeExit(Widget *widget) = 0;
};
} // namespace gx
