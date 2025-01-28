/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_timer.h"
#include "gx_widget.h"

namespace gx {
class Marquee : public Widget {
    GX_OBJECT

public:
    explicit Marquee(Widget *parent = nullptr);
    virtual ~Marquee();

    bool isPaused() const { return false; }
    void setPaused(bool paused);
    GX_NODISCARD const String &text() const { return m_text; }
    void setText(const String &text);
    GX_NODISCARD virtual Size sizeHint() const;
    GX_NODISCARD virtual int heightForWidth(int width) const;

    GX_PROPERTY(String text, get text, set setText)
    GX_PROPERTY(bool paused, get isPaused, set setPaused)

protected:
    virtual void updateStyle();
    virtual bool event(Event *event);
    virtual void resizeEvent(ResizeEvent *event);
    virtual void paintEvent(PaintEvent *event);

private:
    void recheck();
    void onTimeout();

private:
    String m_text;
    Timer m_timer;
    int m_progress, m_textOffset, m_textLength;
    bool m_paused;
};
} // namespace gx
