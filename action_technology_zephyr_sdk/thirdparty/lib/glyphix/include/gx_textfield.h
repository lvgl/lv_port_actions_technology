/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_timer.h"
#include "gx_widget.h"

namespace gx {
class TextField : public Widget {
    GX_OBJECT
public:
    explicit TextField(Widget *parent = nullptr);
    virtual ~TextField();

    GX_NODISCARD const String &text() const { return m_text; }
    void setText(const String &text);
    GX_METHOD void insert(const String &text);
    GX_METHOD void backspace();
    virtual Size sizeHint() const;
    virtual int heightForWidth(int width) const;

    Signal<String> edited;

    GX_PROPERTY(String value, get text, set setText, signal edited)

protected:
    virtual void resizeEvent(ResizeEvent *event);
    virtual void paintEvent(PaintEvent *event);
    virtual bool gestureEvent(GestureEvent *event);

private:
    void onTimeout();
    void resetCursor();
    void locationCursor(int position);
    void updateText();

private:
    String m_text;
    int m_textOffset, m_textLength;
    int m_cursorOffset, m_cursorIndex;
    bool m_showCursor;
    Timer m_timer;
};
} // namespace gx
