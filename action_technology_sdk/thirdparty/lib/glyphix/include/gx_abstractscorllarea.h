/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_signals.h"
#include "gx_widget.h"

namespace gx {
class ScrollBar;
class AbstractScrollEffect;
template<typename T> class ValueAnimation;

class AbstractScrollArea : public Widget {
    GX_OBJECT

public:
    enum ExtraFlags { ExtraSnapshot = 1 << 0, ExtraScrolling = 1 << 1, ExtraUser = 1 << 8 };
    enum GX_ENUM SnapType {
        SnapNone GX_ALIAS("none") = 0,
        SnapStart GX_ALIAS("start"),
        SnapCenter GX_ALIAS("center"),
        SnapEdge GX_ALIAS("edge"),
        SnapEnd GX_ALIAS("end")
    };
    enum GX_ENUM ScrollBehavior { Instant GX_ALIAS("instant"), Smooth GX_ALIAS("smooth") };
    enum ScrollState { ScrollStop = 0, ScrollGesture = 1, ScrollInertial = 2 };
    struct GX_STRUCT ScrollEvent {
        int scrollX, scrollY;
        int scrollState; // The actual type is ScrollState.
    };

    explicit AbstractScrollArea(Widget *parent = nullptr);
    virtual ~AbstractScrollArea();

    GX_NODISCARD bool isHorizontal() const { return direction() & Horizontal; }
    GX_NODISCARD bool isVertical() const { return direction() & Vertical; }
    GX_NODISCARD Direction direction() const { return Direction((m_mask >> 8) & 3); }
    GX_NODISCARD ScrollBar *horizontalScrollBar() const;
    GX_NODISCARD ScrollBar *verticalScrollBar() const;
    GX_NODISCARD SnapType weakGesture() const;
    GX_NODISCARD Widget *contentWidget() const;
    GX_NODISCARD bool isSnapshot() const { return extraFlags() & ExtraSnapshot; }
    GX_NODISCARD bool isScrolling() const { return extraFlags() & ExtraScrolling; }
    GX_NODISCARD bool isAnimationScrolling() const;
    GX_NODISCARD ScrollEvent scrollEventValue() const;
    void setWeakGesture(SnapType mode);
    void setHorizontalScrollBar(ScrollBar *widget);
    void setVerticalScrollBar(ScrollBar *widget);
    void setContentWidget(Widget *widget);
    void scrollTo(int x, int y, ScrollBehavior behavior = Instant);
    void scrollTo(const Point &offset, ScrollBehavior behavior = Instant);
    void setSnapshot(bool enable) { setExtraFlags(ExtraSnapshot, enable); }
    void setDamping(float value);

    GX_NODISCARD virtual Rect contentRect() const;
    GX_NODISCARD virtual Layout *layout() const;
    virtual void setLayout(Layout *layout);
    virtual void addItem(Widget *widget, int index);
    using Widget::addItem;

    Signal<Point, bool> scrolled;
    Signal<Size> contentResized;

    GX_PROPERTY(int scrollTop, set scrollToTop, get scrollTop, signal scrolled)
    GX_PROPERTY(int scrollLeft, set scrollToLeft, get scrollLeft, signal scrolled)
    GX_PROPERTY(ScrollEvent scroll, get scrollEventValue, signal scrolled)
    GX_PROPERTY(bool scrolled, get isScrolling, signal scrolled)
    GX_PROPERTY(int scrollWidth, get scrollWidth, signal contentResized)
    GX_PROPERTY(int scrollHeight, get scrollHeight, signal contentResized)
    GX_PROPERTY(float damping, set setDamping)
    GX_PROPERTY(bool snapshot, get isSnapshot, set setSnapshot)
    GX_PROPERTY(SnapType weakGesture, get weakGesture, set setWeakGesture)

protected:
    AbstractScrollArea(AbstractScrollEffect *effect, Widget *parent);

    void setupView();
    GX_NODISCARD int extraFlags() const { return m_flags; }
    void setExtraFlags(int flags, bool enable);
    void setDirection(Direction direction);
    void setScrollEffect(AbstractScrollEffect *effect);

    virtual void scrollContents(int x, int y);
    virtual bool gestureEvent(GestureEvent *event);
    virtual bool wheelEvent(WheelEvent *event);
    virtual void resizeEvent(ResizeEvent *event);

private:
    void initialization(AbstractScrollEffect *effect);
    GX_NODISCARD int scrollTop() const { return -contentRect().top(); }
    GX_NODISCARD int scrollLeft() const { return -contentRect().left(); }
    GX_NODISCARD int scrollWidth() const { return contentRect().width(); }
    GX_NODISCARD int scrollHeight() const { return contentRect().height(); }
    void scrollToTop(int y) { scrollTo(-contentRect().x(), y, Instant); }
    void scrollToLeft(int x) { scrollTo(x, -contentRect().y(), Instant); }
    void scrollContentsAndScrollBar(int x, int y);
    void setChildWidget(Widget *widget, int index);
    GX_NODISCARD Widget *childWidget(int index) const;
    void startAnimation(const Point &velocity);
    void gestureScroll(const Point &offset);
    void animationScroll(const PointF &point);
    void enableSnapshot();
    void disableSnapshot();
    void animationFinished();
    void rebound() { startAnimation(Point()); }

    AbstractScrollEffect *m_effect;
    uint16_t m_mask;
    uint16_t m_flags;
};

inline void AbstractScrollArea::scrollTo(const Point &offset, ScrollBehavior behavior) {
    scrollTo(offset.x(), offset.y(), behavior);
}
} // namespace gx
