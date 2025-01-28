/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_label.h"

namespace gx {
class SpanLabel : public Label {
    GX_OBJECT

public:
    explicit SpanLabel(Widget *parent = nullptr);

    GX_NODISCARD bool testBlank(const Point &point) const;
    virtual Line inlineSpan(int width, int indent);
    GX_NODISCARD virtual int layoutHints() const;

protected:
    virtual void paintEvent(PaintEvent *event);

private:
    Line m_inlineSpan;
};
} // namespace gx
