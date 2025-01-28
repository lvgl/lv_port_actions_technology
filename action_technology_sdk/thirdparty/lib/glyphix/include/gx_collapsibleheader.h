/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_pointer.h"
#include "gx_widget.h"

namespace gx {
class CollapsibleHeader : public Widget {
    GX_OBJECT

public:
    explicit CollapsibleHeader(Widget *parent);
    virtual ~CollapsibleHeader();

    Widget *headerWidget() const;
    void setHeaderWidget(Widget *header);
    void setContentWidget(Widget *content);

    virtual void addItem(Widget *widget, int index);

private:
    void scrollHeader(int offset);
    void onScrolled(const Point &offset) { scrollHeader(offset.y()); }

private:
    class CollapsibleHeaderLayout;
    int m_headerOffset;
    Pointer<Widget> m_headerWidget;
};
} // namespace gx
