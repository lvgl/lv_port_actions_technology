/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_gesture.h"

namespace gx {
class Event;

class GestureRecognizer {
public:
    enum State {
        // Here we use the enumeration value of Gesture::State to help the compiler optimize some
        // code
        Ignore = Gesture::Reset,
        Trigger = Gesture::Started,
        Single = Gesture::Updated,
        Finish = Gesture::Finished,
        Cancel = Gesture::Canceled
    };
    GestureRecognizer();
    virtual ~GestureRecognizer();
    virtual Gesture *create() = 0;
    virtual State recognize(Gesture *gesture, Event *event) = 0;
    virtual void reset(Gesture *gesture);

    static void registered(GestureRecognizer *reco);

private:
    GestureRecognizer *m_next;
    friend class GestureManager;
};

class PressGestureRecognizer : public GestureRecognizer {
public:
    virtual Gesture *create();
    virtual State recognize(Gesture *gesture, Event *event);
    virtual void reset(Gesture *gesture);
};

class PanGestureRecognizer : public GestureRecognizer {
public:
    virtual Gesture *create();
    virtual State recognize(Gesture *gesture, Event *event);
    virtual void reset(Gesture *gesture);

private:
    static void updateVelocity(PanGesture *g, uint32_t timestamp);
};
} // namespace gx
