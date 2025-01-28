/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_atomic.h"
#include "gx_mutex.h"
#include "gx_semaphore.h"
#include <deque>
#ifndef GX_COPMATIBLE_CXX_98
#include <functional>
#endif

namespace gx {
class Event;
class EventDispatcher;

class EventLoop : NonCopyable<EventLoop> {
public:
    //! The event filter specifies which events will be processed or ignored.
    enum EventFilter {
        AllEvents = 0,             //!< All events will be processed (default).
        ExcludeInputEvents = 0x01, //!< Do not process input events.
        ExcludePaintEvents = 0x02, //!< Do not process paint events.
    };
    GX_FLAGS(EventFilter)
    enum QueuePriority { LowPriority = 0, HighPriority = 1 };
    EventLoop();
    virtual ~EventLoop();

    void installDispatcher(EventDispatcher *dispatcher);
    void postEvent(Event *event, int priority = LowPriority);
    void sendEvent(Event *e, EventFilter filter = AllEvents);
#ifndef GX_COPMATIBLE_CXX_98
    void postTask(std::function<void()> task, int priority = LowPriority);
#endif
    void quit();
    bool waitEvents(int timeout = -1);
    bool processEvents(EventFilter filter = AllEvents);
    virtual bool event(Event *event, EventFilter filter);
    GX_NODISCARD Mutex &mutex() { return m_mutex; }
    void notify() { m_notify.notify(); }
    GX_NODISCARD bool isQuited() const { return m_quit; }

private:
    inline bool isEmptyQueue();
    inline Event *popEventQueue();

private:
    AtomicInt m_quit;
    Mutex m_mutex;
    Semaphore m_notify;
    std::deque<Event *> m_eventQueue;
    EventDispatcher *m_dispatcher;
};

inline void EventLoop::sendEvent(Event *e, EventFilter filter) { event(e, filter); }
} // namespace gx
