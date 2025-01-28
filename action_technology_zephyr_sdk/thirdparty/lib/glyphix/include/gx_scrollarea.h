/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_abstractscorllarea.h"

namespace gx {
class AbstractVisualEffect;

class ScrollArea : public AbstractScrollArea {
    GX_OBJECT

public:
    enum GX_ENUM ScrollBarStyle {
        RemoveScrollBar GX_ALIAS("hidden"),
        LinearScrollBar GX_ALIAS("line"),
        DotsScrollBar GX_ALIAS("dots")
    };

    explicit ScrollArea(Widget *parent = nullptr);
    explicit ScrollArea(SnapType snapType, Widget *parent = nullptr);
    virtual ~ScrollArea();

    GX_NODISCARD SnapType scrollSnap() const;
    void setScrollSnap(SnapType mode);

    /**
     * Gets the index of the element at the current view position, depending on the value of the
     * \ref isAlignment().
     */
    GX_NODISCARD int index() const;
    /**
     * Moves the viewport directly to the specified child without animation transitions. The
     * viewport position stops at the first or last widget if the content crosses the boundary.
     * @param index The target child to be moved.
     * @param behavior Set to index instantly or scroll smoothly to index.
     */
    GX_METHOD void setIndex(int index, ScrollBehavior behavior = Smooth);
    GX_NODISCARD int scrollPosition() const;
    /**
     * scroll to position
     * @param position The location where the scrolling is required.
     */
    GX_METHOD void scrollTo(int position, ScrollBehavior behavior = Instant);

    void setScrollBar(ScrollBarStyle mode);
    void setVisualEffect(AbstractVisualEffect *effect);

    GX_NODISCARD virtual Rect contentRect() const;
    GX_NODISCARD virtual Point leadingSpace() const;
    virtual bool event(Event *event);
    virtual void setLayout(Layout *layout);

    /**
     * The signal is emitted when the child widget in viewport changes, this can be caused by user
     * scrolling or calls such as setIndex(), scrollTo(), etc.
     * @tparam int[0] The new widget index.
     * @tparam int[1] The index of the widget before scrolling.
     */
    Signal<int, int> changed;

    GX_PROPERTY(SnapType scrollSnap, set setScrollSnap, get scrollSnap)
    GX_PROPERTY(int index, set setIndex, get index, signal changed)
    GX_PROPERTY(AbstractVisualEffect *deformation, set setVisualEffect)
    GX_PROPERTY(ScrollBarStyle indicator, set setScrollBar)

protected:
    ScrollArea(AbstractScrollEffect *effect, Widget *parent);

    virtual void scrollContents(int x, int y);
    virtual bool eventFilter(Object *receiver, Event *event);
    virtual void paintEvent(PaintEvent *event);

private:
    void initialization();
    void processIndex();

    int m_index, m_indexShadow;
    AbstractVisualEffect *m_effect;
};
} // namespace gx
