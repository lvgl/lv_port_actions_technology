/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_widget.h"

namespace gx {
class Label : public Widget {
    GX_OBJECT

public:
    explicit Label(Widget *parent = nullptr);
    virtual ~Label();

    GX_NODISCARD const String &text() const { return m_text; }
    void setText(const String &text);
    GX_NODISCARD virtual Size sizeHint() const;
    GX_NODISCARD virtual int heightForWidth(int width) const;

    GX_PROPERTY(String text, get text, set setText)

protected:
    virtual void updateStyle();
    virtual void resizeEvent(ResizeEvent *event);
    virtual void paintEvent(PaintEvent *event);

private:
    String m_text;
};
} // namespace gx
