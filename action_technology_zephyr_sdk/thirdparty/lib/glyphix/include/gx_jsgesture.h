/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_jsvm.h"

namespace gx {
class Gesture;
class TouchEvent;

namespace js {
JsValue parseGestureEvent(const Gesture *gesture);
} // namespace js
} // namespace gx
