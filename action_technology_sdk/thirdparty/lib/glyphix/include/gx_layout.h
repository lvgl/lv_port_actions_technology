/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_object.h"
#include "gx_widget.h"

namespace gx {
class Layout {
public:
    class iterator;
    enum AlignContent {
        AlignAuto = 0,
        AlignStretch = 1,
        AlignStart = 2,
        AlignEnd = 3,
        AlignCenter = 4,
        AlignBaseline = 5,
        SpaceBetween = 6,
        SpaceAround = 7,
        UnknownAlignMode
    };
    /**
     * Stretch (grow or shrink) policy in case of layout space remainder or overflow.
     */
    enum StretchPolicy {
        // ！No stretch, whatever the remainder or overflow in the x/y dimension in layout.
        NoStretch = NoDirection,
        //! Stretch when the x dimension remainder or overflow in layout.
        HorizontalStretch = Horizontal,
        //! Stretch when the y dimension remainder or overflow in layout.
        VerticalStretch = Vertical,
        //! Stretch when the x/y dimension remainder or overflow in layout.
        AllStretch = AllDirections
    };
    enum Display {
        BlockDisplay = 0,
        InlineDisplay,
        FlexDisplay,
        InlineFlexDisplay,
        NoneDisplay,
        UnknownDisplay
    };
    enum FlexWarpPolicy { FlexNoWrap = 0, FlexWrap, FlexWrapReverse };

    Layout();
    virtual ~Layout();
    GX_NODISCARD Size viewSize() const { return m_container->size(); }
    GX_NODISCARD Size contentSize() const { return Size(m_contentWidth, m_contentHeight); }
    GX_NODISCARD Direction direction() const { return Direction(m_direction); }
    /**
     * Get the shrinkage policy in case of layout overflow. The default policy is AllStretch.
     * @see setStretchPolicy()
     */
    GX_NODISCARD StretchPolicy stretchPolicy() const { return StretchPolicy(m_shrink); }
    GX_NODISCARD virtual Size sizeHint() const;
    GX_NODISCARD virtual int heightForWidth(int) const;
    GX_NODISCARD int baseline() const;
    void layout();
    GX_NODISCARD iterator begin() const;
    GX_NODISCARD iterator end() const;

    /**
     * Get the stretch policy in case of layout overflow.
     * @see stretchPolicy()
     */
    void setStretchPolicy(StretchPolicy policy) { m_shrink = policy; }

    static AlignContent parseAlignContent(const char *str);
    static const char *stringifyAlignContent(int mode);
    static Layout *factory(const Style &style);

protected:
    explicit Layout(Direction direction, Widget *container = nullptr);
    virtual void layout(const Size &size) = 0;
    GX_NODISCARD Widget *container() const { return m_container; }
    void setBaseline(int baseline) { m_baseline = min(65535, baseline); }

private:
    void setContainer(Widget *widget);
    void layoutNonStatic();

private:
    Widget *m_container;
    int m_contentWidth, m_contentHeight;
    uint32_t m_direction : 2;
    uint32_t m_shrink : 2;
    mutable uint32_t m_baseline : 16;
    friend class Widget;
};

class Layout::iterator {
public:
    iterator() : m_iter() {}
    explicit iterator(ObjectList::const_iterator iter) : m_iter(iter) {}
    GX_NODISCARD Widget *widget() const;
    GX_NODISCARD bool isLayoutNode() const;
    GX_NODISCARD bool isInlineNode() const { return widget()->isInlineWidget(); }
    GX_NODISCARD Size size() const;
    GX_NODISCARD int baseline() const;
    GX_NODISCARD std::pair<int, int> minimumSize(const Size &size) const;
    GX_NODISCARD std::pair<int, int> maximumSize(const Size &size) const;
    GX_NODISCARD Style::FlexLength flex() const;
    GX_NODISCARD bool flexBasis() const;
    GX_NODISCARD AlignContent alignSelf() const;
    void setGeometry(Layout *layout, const Rect &rect);
    void setGeometry(Layout *layout, int x, int y, int width, int height);
    /**
     * Measure the size of a layout node.
     */
    GX_NODISCARD const Size &sizeHint() const;
    /**
     * Measures the size of the layout node, otherwise uses the percentage size if the specifies it
     * in style.
     * @param size The layout size.
     * @return The result size.
     */
    GX_NODISCARD Size sizeHint(const Size &size) const;
    GX_NODISCARD int heightForWidth(int width) const;
    GX_NODISCARD Display display() const;

    iterator &operator++();
    iterator operator++(int);
    bool operator==(const iterator &rhs) const { return m_iter == rhs.m_iter; }
    bool operator!=(const iterator &rhs) const { return !(*this == rhs); }

private:
    ObjectList::const_iterator m_iter;
    friend class Layout;
};

inline Layout::iterator &Layout::iterator::operator++() {
    m_iter++;
    return *this;
}

inline Layout::iterator Layout::iterator::operator++(int) {
    iterator temp(*this);
    m_iter++;
    return temp;
}

inline Widget *Layout::iterator::widget() const {
    GX_CRT_ASSERT((*m_iter)->isWidget());
    return static_down_cast<Widget *>(*m_iter);
}

inline void Layout::iterator::setGeometry(Layout *layout, int x, int y, int width, int height) {
    setGeometry(layout, Rect(x, y, width, height));
}
} // namespace gx
