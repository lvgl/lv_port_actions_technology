/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_wheelevent.h"

namespace gx {
class WheelDevice {
public:
    WheelDevice();
    void wheelEvent(Widget *target, WheelEvent *event);

private:
    uint32_t m_lastTick;
};
} // namespace gx
