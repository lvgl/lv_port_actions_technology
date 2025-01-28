/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_gesture.h"
#include "gx_gesturerecognizer.h"
#include "gx_pointer.h"
#include "gx_registerlist.h"
#include "gx_vector.h"
#include "gx_widget.h"

namespace gx {
class TouchEvent;
class WidgetManager;
class WidgetZOrder;

class GestureManager {
    struct Extent;
    struct GestureNode;

public:
    explicit GestureManager(WidgetZOrder &collector);
    ~GestureManager();
    void touchEvent(Widget *root, TouchEvent *event);
    GX_NODISCARD Widget *target() const { return m_target; }
    void registered(GestureRecognizer *recognizer);

private:
    void recognizeGesture(TouchEvent *event);
    void dispatchGesture(Widget *root, TouchEvent *event);
    inline bool hitTest(Widget *widget);

    bool dispatchWidget(Widget *widget, Extent extent);
    bool dispatchDirect(Widget *target, GestureEvent *event);
    bool bubblingGesture(Widget *target, GestureEvent *event, bool transfer = true);
    void resetGestures();
    Gesture *disambiguation();

private:
    struct GestureNode {
        Gesture *gesture;
        GestureRecognizer *recognizer;
        GestureNode(Gesture *g, GestureRecognizer *r) : gesture(g), recognizer(r) {}
    };

    typedef RegisterList<GestureRecognizer, &GestureRecognizer::m_next> RegList;
    RegList m_list;
    Vector<GestureNode> m_gestures;
    Pointer<Widget> m_target;
    Gesture *m_active;
    GestureEvent *m_gestureEvent;
    WidgetZOrder &m_collector;
};
} // namespace gx
