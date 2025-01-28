/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_event.h"
#include "gx_geometry.h"

namespace gx {
class TouchEvent : public Event {
public:
    enum State { Released, Pressed, Updated };
    TouchEvent(State state, const Point &point, unsigned int timestamp);
    GX_NODISCARD Point point() const { return m_point; }
    GX_NODISCARD State state() const { return m_state; }
    GX_NODISCARD uint32_t timestamp() const { return m_timestamp; }
    GX_NODISCARD bool isPressed() const { return m_state != Released; }
    void setState(State state) { m_state = state; }

private:
    State m_state;
    Point m_point;
    uint32_t m_timestamp;
};

const Logger &operator<<(const Logger &, const TouchEvent *);
} // namespace gx
