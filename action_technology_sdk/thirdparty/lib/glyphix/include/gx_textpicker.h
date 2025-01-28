/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_abstractscorllarea.h"
#include "gx_widget.h"

namespace gx {
class TextPicker : public AbstractScrollArea {
    GX_OBJECT

public:
    explicit TextPicker(Widget *parent = nullptr);
    virtual ~TextPicker();

    GX_NODISCARD bool isLoop() const { return m_loop; }
    GX_NODISCARD int index() const { return m_index; }
    GX_NODISCARD const String &value() {
        return int(m_range.size()) > m_index ? m_range[m_index] : m_range[0];
    }
    void setRange(const Vector<String> &range);
    void setLoop(bool loop) { m_loop = loop; }
    void setIndex(int index);

    Signal<int, int> changed; // changed(newIndex, oldIndex)

    GX_PROPERTY(Vector<String> range, set setRange);
    GX_PROPERTY(bool loop, set setLoop, get isLoop);
    GX_PROPERTY(String value, get value, signal changed);
    GX_PROPERTY(int index, set setIndex, get index, signal changed)

protected:
    virtual bool gestureEvent(GestureEvent *event);
    virtual bool layoutEvent(LayoutEvent *event);
    virtual void paintEvent(PaintEvent *event);

    GX_NODISCARD virtual Rect contentRect() const;
    virtual void scrollContents(int x, int y);
    GX_NODISCARD virtual Point scrollAdjust(const Point &offset) const;

private:
    void setOffset(int position);

private:
    Vector<String> m_range;
    int m_index, m_position, m_showNum;
    bool m_loop;
};
} // namespace gx
