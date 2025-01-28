/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_event.h"

namespace gx {
enum KeyCode {
    KeyPower,
    KeyFunction,
};

class KeyEvent : public Event {
public:
    enum State { Up = false, Down = true };
    KeyEvent(int keyCode, State state, uint32_t timestamp)
        : Event(Key), m_keyCode(keyCode), m_state(state), m_timestamp(timestamp) {}
    GX_NODISCARD int keyCode() const { return m_keyCode; }
    GX_NODISCARD bool isDown() const { return m_state == Down; }
    GX_NODISCARD bool isUp() const { return m_state == Up; }
    GX_NODISCARD uint32_t timestamp() const { return m_timestamp; }

private:
    uint16_t m_keyCode;
    uint8_t m_state;
    uint32_t m_timestamp;
};
} // namespace gx
