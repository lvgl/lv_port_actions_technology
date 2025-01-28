/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_gesturemanager.h"
#include "gx_signals.h"
#include "gx_snapshotcache.h"
#include "gx_textlayoutcache.h"
#include "gx_wheeldevice.h"
#include "gx_widget.h"

namespace gx {
class TickEvent;
class TouchEvent;
class KeyEvent;
class GestureManager;

/**
 * Calculate the extent area of the Widget.
 */
class WidgetExtent {
public:
    WidgetExtent() GX_DEFAULT;
    explicit WidgetExtent(const Rect &rect) : m_rect(rect), m_offset(rect.topLeft()) {}
    WidgetExtent(const Rect &rect, const Point &offset) : m_rect(rect), m_offset(offset) {}
    GX_NODISCARD bool isEmpty() const { return m_rect.isEmpty(); }
    GX_NODISCARD bool intersects(const Rect &rect) const;
    void join(const Rect &rect);
    void join(const Rect &rect, const Point &offset);
    void join(Widget *widget);
    GX_NODISCARD const Rect &rect() const { return m_rect; }
    GX_NODISCARD const Point &offset() const { return m_offset; }
    GX_NODISCARD bool contains(const Point &point) const;

private:
    Rect m_rect;
    Point m_offset;
};

/**
 * Maintains the Z-index order of the Widget.
 */
class WidgetZOrder {
public:
    typedef Vector<Widget *>::const_iterator iterator;
    typedef Vector<Widget *>::const_reverse_iterator reverse_iterator;

    class Sort {
    public:
        Sort(WidgetZOrder &owner, Widget *widget, const WidgetExtent &extent);
        Sort(WidgetZOrder &owner, Widget *widget, const Point &point);
        ~Sort() { m_owner.m_depth--; }
        GX_NODISCARD iterator begin() const { return m_collected->begin(); }
        GX_NODISCARD iterator end() const { return m_collected->end(); }
        GX_NODISCARD reverse_iterator rbegin() const { return m_collected->rbegin(); }
        GX_NODISCARD reverse_iterator rend() const { return m_collected->rend(); }

    private:
        Sort(const Sort &other) GX_DELETE;
        WidgetZOrder &m_owner;
        const Vector<Widget *> *m_collected;
        friend class WidgetZOrder;
    };
    WidgetZOrder();
    ~WidgetZOrder();
    GX_NODISCARD const Margin &relaxation() const { return m_relaxation; }
    void setRelaxation(const Size &extent);

private:
    template<typename T> struct Detector {};
    template<typename T> void collect(Widget *widget, const T &probe);

private:
    Vector<Vector<Widget *> *> m_vector;
    int m_depth;
    Margin m_relaxation;
};

class WidgetManager {
public:
    WidgetManager();
    ~WidgetManager();

    GX_NODISCARD Widget *window() const { return m_window; }
    GX_NODISCARD Widget *focusWidget() const { return m_focus; }
    GX_NODISCARD Painter *contextPainter() const { return m_contextPainter; }
    void setContextPainter(Painter *painter) { m_contextPainter = painter; }
    void setWindow(Widget *widget) { m_window = widget; }
    void setFocusWidget(Widget *widget);
    SnapshotCache *snapshotCache() { return &m_snapshotCache; }
    GestureManager *gestureManager() { return &m_gestureManager; }
    /**
     * Gets whether the layout is being executed. The Widget::updateLayout() call will not take
     * effect while the layout is being executed.
     */
    GX_NODISCARD bool isLayoutExecuting() const { return m_layoutExecuting; }
    /**
     * Send an event to the WidgetManager and process it immediately.
     * @param event The event to be processed.
     * @param filter Event filter taken from EventLoop::EventFilter. Only events that match the
     * filter will be processed.
     * @return Returns true if the event was successfully processed.
     */
    bool sendEvent(Event *event, int filter = 0);
    /**
     * Solve united dirty rectangles from the window (root) and clear the dirty markers of the
     * widgets. Since widget paint is also driven by dirty markers, calling unitedDirtyRect() will
     * prevent that.
     * @return The dirty rectangle of the entire widget tree.
     */
    Rect unitedDirtyRect() const;
    /**
     * Solve united dirty rectangles from the window (root) and clear the dirty markers of the
     * widgets. Since widget paint is also driven by dirty markers, calling unitedDirtyRect() will
     * prevent that.
     * @param clip The clipping rectangle, it is usually set to a screen or window rectangle.
     * @return The dirty rectangle of the entire widget tree.
     */
    Rect unitedDirtyRect(const Rect &clip) const;
    TextLayoutCache *textLayoutCache() { return &m_textLayoutCache; }

    Signal<> beforePaint;

private:
    void tickEvent(TickEvent *event, int filter);
    void touchEvent(TouchEvent *event, int filter);
    void wheelEvent(WheelEvent *event, int filter);
    void keyEvent(KeyEvent *event, int filter);
    void paintEvent(uint32_t tick);
    bool paintAll();
    void paintTree(Widget *widget, WidgetExtent extent);
    bool paintChildWidget(Widget *widget, WidgetExtent extent);
    bool paintWidget(Widget *widget, const WidgetExtent &extent);
    bool paintSnapshot(Widget *widget, WidgetExtent extent);
    void processLayout(Widget *widget);
    void processMarks(Widget *widget, uint32_t tick);

    static void clearDirty(Widget *widget);
    static void unitedDirtyRect(Widget *widget, WidgetExtent extent, Rect &rect);

private:
    uint32_t m_touchPressed : 1;
    uint32_t m_layoutExecuting : 1;
    uint32_t m_painting : 1;
    Widget *m_window;
    Pointer<Widget> m_focus;
    Painter *m_contextPainter;
    WidgetZOrder m_collector;
    GestureManager m_gestureManager;
    WheelDevice m_wheelManager;
    SnapshotCache m_snapshotCache;
    TextLayoutCache m_textLayoutCache;
    friend class Widget;
};

inline bool WidgetExtent::intersects(const Rect &rect) const {
    return m_rect.intersects(rect + m_offset);
}

inline void WidgetExtent::join(const Rect &rect) {
    m_rect &= rect + m_offset;
    m_offset += rect.topLeft();
}

inline void WidgetExtent::join(const Rect &rect, const Point &offset) {
    m_rect &= rect + m_offset;
    m_offset += offset;
}

inline bool WidgetExtent::contains(const Point &point) const { return m_rect.contains(point); }

inline Rect WidgetManager::unitedDirtyRect() const {
    return window() ? unitedDirtyRect(window()->geometry()) : Rect();
}

const Logger &operator<<(const Logger &, const WidgetExtent &);
} // namespace gx
