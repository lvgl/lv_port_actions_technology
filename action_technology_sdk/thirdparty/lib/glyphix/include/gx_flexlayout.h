/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_layout.h"

namespace gx {
class FlexLayout : public Layout {
public:
    explicit FlexLayout(Direction direction);
    void setJustifyContent(int mode);
    void setAlignItems(int mode);
    int justifyContent() const { return int(m_justifyContent); }
    int alignItems() const { return int(m_alignContent); }
    bool horizontal() const { return int(direction()) == Horizontal; }

    using Layout::layout;
    virtual Size sizeHint() const;
    virtual int heightForWidth(int) const;

protected:
    virtual void layout(const Size &size);

private:
    struct Item;
    struct LayoutData;

    void alignMainAxis(LayoutData &data, int mainSize) const;
    void layoutMainAxis(LayoutData &data, const Size &size) const;
    void arrangeMainAxis(LayoutData &data, int space) const;
    static void arrangeGrow(LayoutData &data, int space);
    static void arrangeShrink(LayoutData &data, int space);

private:
    mutable uint32_t m_heightForWidth : 1;
    uint32_t m_justifyContent : 4;
    uint32_t m_alignContent : 4;
};
} // namespace gx
