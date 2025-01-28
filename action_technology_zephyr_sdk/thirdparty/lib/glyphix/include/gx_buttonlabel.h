/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_pointer.h"
#include "gx_spanlabel.h"

namespace gx {
class AbstractButton;

class ButtonLabel : public SpanLabel {
    GX_OBJECT
public:
    explicit ButtonLabel(Widget *parent = nullptr);
    GX_NODISCARD AbstractButton *target() const;
    void setTarget(AbstractButton *target);

    GX_PROPERTY(String target, set setTarget, get targetName)

protected:
    virtual bool gestureEvent(GestureEvent *event);

private:
    GX_NODISCARD String targetName() const;
    void setTarget(const String &target);

private:
    String m_targetName;
    Pointer<AbstractButton> m_target;
};
} // namespace gx
