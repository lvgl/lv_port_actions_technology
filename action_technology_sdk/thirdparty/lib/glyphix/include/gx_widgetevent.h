/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_event.h"
#include "gx_geometry.h"

namespace gx {
class Widget;

class ResizeEvent : public Event {
public:
    ResizeEvent(const Size &size, const Size &oldSize)
        : Event(Resize), m_size(size), m_oldSize(oldSize) {}
    GX_NODISCARD const Size &size() const { return m_size; }
    GX_NODISCARD const Size &oldSize() const { return m_oldSize; }

private:
    Size m_size, m_oldSize;
};

class MoveEvent : public Event {
public:
    MoveEvent(const Point &pos, const Point &oldPos) : Event(Move), m_pos(pos), m_oldPos(oldPos) {}
    GX_NODISCARD const Point &position() const { return m_pos; }
    GX_NODISCARD const Point &oldPosition() const { return m_oldPos; }

private:
    Point m_pos, m_oldPos;
};
} // namespace gx
