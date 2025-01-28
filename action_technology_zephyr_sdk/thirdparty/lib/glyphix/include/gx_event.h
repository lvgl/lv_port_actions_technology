/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_string.h"

namespace gx {
class Object;
class Widget;

class Event {
public:
    enum EventType {
        Tick,
        Touch,
        Gesture,
        Layout,
        Paint,
        Resize,
        Move,
        Wheel,
        Focus,
        Launch,
        Key,
        Task,
        Quit,
        /* user command event. It should always be the last command type. */
        UserEvent = 0x100,
        UnknownEvent = 0xffff
    };

    virtual ~Event();
    GX_NODISCARD int type() const { return m_type; }
    GX_NODISCARD bool isAccepted() const { return m_accepted; }
    void setAccepted(bool status) { m_accepted = status; }
    void accept() { setAccepted(true); }
    void ignore() { setAccepted(false); }

protected:
    explicit Event(int type) : m_type(type), m_accepted(true) {}

private:
    const uint16_t m_type;
    bool m_accepted;
};

class TickEvent : public Event {
public:
    explicit TickEvent(uint32_t tick);
    GX_NODISCARD uint32_t tick() const { return m_tick; }

private:
    uint32_t m_tick;
};

class TaskEvent : public Event {
public:
    enum TaskPriority { HighPriority = 0, NormalPriority = 1, LowPriority = 2 };
    TaskEvent() : Event(Task) {}
    virtual void exec() = 0;
    virtual int priority() const { return NormalPriority; }
};

class PaintEvent : public Event {
public:
    PaintEvent() : Event(Paint) {}
};

class LaunchEvent : public Event {
public:
    explicit LaunchEvent(const String &name) : Event(Launch), m_name(name) {}
    GX_NODISCARD const String &name() const { return m_name; }

private:
    String m_name;
};

class QuitEvent : public Event {
public:
    QuitEvent() : Event(Quit) {}
};

class FocusEvent : public Event {
public:
    enum Operation { FocusIn, FocusOut };
    FocusEvent(Widget *focusIn, Widget *focusOut, Operation operation)
        : Event(Focus), m_focusIn(focusIn), m_focusOut(focusOut), m_operation(operation) {}
    GX_NODISCARD bool isFocusIn() const { return m_operation == FocusIn; }
    GX_NODISCARD bool isFocusOut() const { return m_operation == FocusOut; }
    GX_NODISCARD Widget *focusIn() const { return m_focusIn; }
    GX_NODISCARD Widget *focusOut() const { return m_focusOut; }

private:
    Widget *m_focusIn, *m_focusOut;
    Operation m_operation;
};

const Logger &operator<<(const Logger &log, const Event *e);
} // namespace gx
