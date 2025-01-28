/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_event.h"

namespace gx {
class EventDispatcher {
public:
    EventDispatcher();
    virtual ~EventDispatcher();

    void link(EventDispatcher *base);
    virtual bool dispatch(Event *event, int filter) = 0;

    static bool defaultDispatch(Event *event, int filter);

protected:
    bool raise(Event *event, int filter);
    bool then(Event *event, int filter) { return raise(event, filter), true; }

private:
    EventDispatcher *m_base;
};
} // namespace gx
