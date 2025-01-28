/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_widget.h"

namespace gx {
class AbstractAnimation;

/**
 * The widget that can apply transition animations, e.g. Applet and AppletPage.
 */
class TransitionWidget : public Widget {
    GX_OBJECT

public:
    explicit TransitionWidget(Widget *parent = nullptr) : Widget(parent) {}

protected:
    /**
     * Gets a transition animation that can be used for this widget.
     * @param name The name of the animation.
     * @param mode Transition mode, the type is actually AppletKit::TransitionMode.
     * @return The transition animation object, otherwise returns nullptr if not configured.
     */
    virtual AbstractAnimation *transition(const String &name, int mode) = 0;
    //! The finished event handler of the OpenEnter transition animation.
    virtual void openEnter();
    //! The finished event handler of the OpenExit transition animation.
    virtual void openExit();
    //! The finished event handler of the CloseEnter transition animation.
    virtual void closeEnter();
    //! The finished event handler of the CloseExit transition animation.
    virtual void closeExit();

private:
    void openEnterFinished() { openEnter(); }
    void openExitFinished() { openExit(); }
    void closeEnterFinished() { closeEnter(); }
    void closeExitFinished() { closeExit(); }

    friend class TransitionManager;
};
} // namespace gx
