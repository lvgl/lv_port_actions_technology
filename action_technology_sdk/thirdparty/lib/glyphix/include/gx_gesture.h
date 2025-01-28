/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_event.h"
#include "gx_geometry.h"

namespace gx {
class Widget;
class TouchEvent;

class Gesture {
public:
    enum State { Reset, Started, Updated, Finished, Canceled };
    enum Type { Press = 0, Pan, Unknown };
    virtual ~Gesture();
    // Gets the gesture type.
    GX_NODISCARD Type type() const { return Type(m_type); }
    // Gets the gesture state.
    GX_NODISCARD State state() const { return State(m_state); }
    //! Gets if the gesture starts.
    GX_NODISCARD bool isStarted() const { return state() == Started; }
    //! Gets if the gesture is updated.
    GX_NODISCARD bool isUpdated() const { return state() == Updated; }
    //! Gets if the gesture has been finished.
    GX_NODISCARD bool isFinished() const { return state() == Finished; }
    //! Gets if the gesture has been canceled.
    GX_NODISCARD bool isCanceled() const { return state() == Canceled; }
    //! The starting point of the gesture relative to the device screen.
    GX_NODISCARD const Point &point() const { return m_point; }
    //! The starting point of the gesture relative to the target widget.
    GX_NODISCARD const Point &clientPoint() const { return m_clientPoint; }
    //! The timestamp at the beginning of the gesture.
    GX_NODISCARD uint32_t timestamp() const { return m_timestamp; }
    void setPoint(const Point &point, uint32_t timestamp);
    void setClientPoint(const Point &point) { m_clientPoint = point; }

protected:
    explicit Gesture(Type type);

private:
    uint32_t m_type : 8;
    uint32_t m_state : 3;
    uint32_t m_ignored : 1;
    uint32_t m_timestamp;
    Point m_point, m_clientPoint;
    friend class GestureEvent;
    friend class GestureManager;
    friend class GestureRecognizer;
};

class PressGesture : public Gesture {
public:
    PressGesture();
    GX_NODISCARD uint32_t duration() const { return m_lastTimestamp - timestamp(); }

private:
    uint32_t m_lastTimestamp;
    friend class PressGestureRecognizer;
};

class PanGesture : public Gesture {
public:
    enum SwipeDirection { SwipeNone = NoDirection, SwipeLeft, SwipeRight, SwipeUp, SwipeDown };

    PanGesture();
    GX_NODISCARD Point offset() const { return m_offset; }
    GX_NODISCARD Point delta() const { return m_delta; }
    GX_NODISCARD Point velocity() const { return m_velocity; }
    GX_NODISCARD int speed() const { return m_velocity.modules(); }
    GX_NODISCARD Direction direction() const { return Direction(m_direction); }
    //! Returns true when first swipe.
    GX_NODISCARD bool isSwiped() const { return m_swiped; }
    GX_NODISCARD SwipeDirection swipeDirection() const { return SwipeDirection(m_swipe); }

private:
    Point m_offset, m_strictOffset, m_delta, m_velocity;
    uint32_t m_lastTimestamp;
    uint32_t m_direction : 2;
    uint32_t m_swiped : 1;
    uint32_t m_swipe : 3;
    friend class PanGestureRecognizer;
};

class GestureEvent : public Event {
public:
    GestureEvent(class Gesture *gesture, TouchEvent *touchEvent, Widget *target = nullptr);
    GX_NODISCARD class Gesture *gesture() const { return m_gesture; }
    GX_NODISCARD TouchEvent *touchEvent() const { return m_touchEvent; }
    GX_NODISCARD Widget *target() const { return m_target; }
    GX_NODISCARD bool isHitTest() const { return !m_target; }
    GX_NODISCARD bool isWeakResponse() const { return m_weakResponse; }
    void setWeakResponse(bool enable) { m_weakResponse = enable; }

protected:
    void setTarget(Widget *target) { m_target = target; }

private:
    class Gesture *m_gesture;
    TouchEvent *m_touchEvent;
    Widget *m_target;
    bool m_weakResponse;
    friend class GestureManager;
};

inline void Gesture::setPoint(const Point &point, uint32_t timestamp) {
    m_point = point;
    m_timestamp = timestamp;
}

const Logger &operator<<(const Logger &, const Gesture::State &t);
const Logger &operator<<(const Logger &, const Gesture::Type &t);
const Logger &operator<<(const Logger &, const GestureEvent *);
const Logger &operator<<(const Logger &, const Gesture *);
} // namespace gx
