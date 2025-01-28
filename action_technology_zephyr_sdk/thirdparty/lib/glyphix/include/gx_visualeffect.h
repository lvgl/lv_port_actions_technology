/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_widget.h"

namespace gx {
class AbstractScrollArea;

class AbstractVisualEffect {
public:
    AbstractVisualEffect() GX_DEFAULT;
    virtual ~AbstractVisualEffect() GX_DEFAULT;
    virtual void reset(Widget *target) = 0;
    virtual void apply(Widget *target, const Rect &window, const Point &position, bool focus) = 0;
};

class FisheyeVisualEffect : public AbstractVisualEffect {
public:
    FisheyeVisualEffect() GX_DEFAULT;
    virtual ~FisheyeVisualEffect() GX_DEFAULT;
    virtual void reset(Widget *target);
    virtual void apply(Widget *target, const Rect &window, const Point &position, bool);
};
} // namespace gx
