/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_pointer.h"
#include "gx_signals.h"
#include "gx_widget.h"

namespace gx {
class RadioButton;

class RadioGroup : public Widget {
    GX_OBJECT

public:
    explicit RadioGroup(Widget *parent = nullptr);

    GX_NODISCARD String value() const { return m_value; }
    void setValue(const String &value);

    Signal<String> checked;

    GX_PROPERTY(String value, get value, set setValue, signal checked)

private:
    String m_value;
    Vector<Pointer<RadioButton> > m_radioButtons;
    friend class RadioButton;
};
} // namespace gx
