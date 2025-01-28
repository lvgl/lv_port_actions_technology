/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_event.h"

namespace gx {
class WheelEvent : public Event {
public:
    enum Direction {
        Forward,
        Back,
    };
    explicit WheelEvent(float delta);
    WheelEvent(float delta, uint32_t timestamp);
    GX_NODISCARD float delta() const { return m_delta; }
    GX_NODISCARD float velocity() const { return m_velocity; }
    GX_NODISCARD Direction direction() const;

private:
    float m_delta;
    float m_velocity;
    uint32_t m_tick;
    friend class WheelDevice;
};
} // namespace gx
