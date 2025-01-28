/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_prelude.h"

namespace gx {
class FPSCounter {
public:
    explicit FPSCounter(uint32_t tick);
    ~FPSCounter();

    void increaseRenderFrame();

    static uint32_t framePreSecond();
    static uint32_t renderPerSecond();

private:
    uint32_t m_tick;
};
} // namespace gx
