/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_scrollarea.h"
#include "gx_scrolleffect.h"
#include "gx_widget.h"

namespace gx {
class Swiper : public ScrollArea {
    GX_OBJECT

public:
    explicit Swiper(Widget *parent = nullptr);
    virtual ~Swiper();

    GX_NODISCARD bool isLoop() const { return m_loop; }
    GX_NODISCARD const Length &pageLength() const { return m_pageLength; }

    void setLoop(bool loop) { m_loop = loop; }
    void setVertical(bool vertical);
    void setPageLength(const Length &length);

    GX_NODISCARD virtual Rect contentRect() const;

    GX_PROPERTY(bool loop, set setLoop, get isLoop)
    GX_PROPERTY(bool vertical, set setVertical, get isVertical)
    GX_PROPERTY(Length pageLength, set setPageLength, get pageLength)

protected:
    virtual void scrollContents(int dx, int dy);
    virtual bool layoutEvent(LayoutEvent *event);

private:
    bool m_loop;
    Length m_pageLength;
};
} // namespace gx
