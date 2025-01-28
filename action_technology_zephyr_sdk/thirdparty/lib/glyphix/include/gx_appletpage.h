/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_jsvm.h"
#include "gx_pointer.h"
#include "gx_transitionwidget.h"

namespace gx {
class Applet;
class Component;
class AbstractAnimation;

class AppletPage : public TransitionWidget {
    GX_OBJECT

public:
    //! The flags used to setFlags() and flags() in AppletPage.
    enum Flags {
        //! The visible status of the page has changed.
        VisibleUpdated = UserWidgetFlags,
        //! Deletes itself after an open-exit or close-exit transition.
        DeleteAfterExit = UserWidgetFlags << 1
    };

    explicit AppletPage(Applet *applet, Widget *parent = nullptr);
    virtual ~AppletPage();
    GX_NODISCARD Applet *applet() const;
    GX_NODISCARD Component *component() const { return m_component; }
    GX_NODISCARD const String &pageName() const { return objectName(); }
    bool construct(const String &name, const String &path, const JsValue &params = JsValue());
    /**
     * Toggles the visible state of the page. This function causes the `onShow()` or `onHide()`
     * lifecycle function of the page to be called.
     * @param state Specifies the visible state of ppthe page.
     */
    virtual void setVisible(bool state);
    void render();
    bool postRender();
    //! Close the page and delete itself.
    void close();
    virtual void addItem(Widget *widget, int index);

protected:
    explicit AppletPage(Widget *parent = nullptr);
    virtual bool layoutEvent(LayoutEvent *event);
    virtual bool gestureEvent(GestureEvent *event);
    virtual bool focusEvent(FocusEvent *event);
    virtual void paintEvent(PaintEvent *event);

    GX_NODISCARD virtual AbstractAnimation *transition(const String &name, int mode);
    virtual void openEnter();
    virtual void openExit();
    virtual void closeEnter();
    virtual void closeExit();

private:
    void renderComponents();

private:
    Component *m_component;
    Pointer<Widget> m_focusWidget;
};
} // namespace gx
