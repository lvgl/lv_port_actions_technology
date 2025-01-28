/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_abstractbutton.h"
#include "gx_signals.h"

namespace gx {
class Button : public AbstractButton {
    GX_OBJECT

public:
    explicit Button(Widget *parent = nullptr);
    virtual ~Button();

    GX_NODISCARD bool isDown() const { return m_down; }
    GX_NODISCARD bool isCheckable() const { return m_checkable; }
    GX_NODISCARD bool isToggleable() const { return m_toggleable; }
    GX_NODISCARD const String &text() const { return m_text; }
    void setDown(bool state);
    void setCheckable(bool status);
    void setToggleable(bool status);
    void setText(const String &text);
    GX_NODISCARD virtual Size sizeHint() const;

    Signal<bool> pressed;

    GX_PROPERTY(String text, get text, set setText)
    GX_PROPERTY(bool press, get isDown, set setDown, signal pressed)
    GX_PROPERTY(bool value, get isDown, set setDown, signal pressed)
    GX_PROPERTY(bool checkable, get isCheckable, set setCheckable)
    GX_PROPERTY(bool toggleable, get isToggleable, set setToggleable)

protected:
    virtual void updateStyle();
    virtual void resizeEvent(ResizeEvent *event);
    virtual bool gestureEvent(GestureEvent *event);
    virtual void paintEvent(PaintEvent *event);

private:
    enum ButtonState { Released, Hovered, Pressed };
    void setButtonState(ButtonState state, bool hasSignal = true);

private:
    uint32_t m_down : 1;
    uint32_t m_checkable : 1;
    uint32_t m_toggleable : 1;
    String m_text;
};
} // namespace gx
