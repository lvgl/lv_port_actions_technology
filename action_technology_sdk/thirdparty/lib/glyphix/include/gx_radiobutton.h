/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_checkbox.h"

namespace gx {
class RadioGroup;

class RadioButton : public CheckBox {
    GX_OBJECT

public:
    explicit RadioButton(Widget *parent = nullptr);
    virtual ~RadioButton();

    GX_NODISCARD String value() const { return m_id; }
    void setValue(const String &id) { m_id = id; }
    virtual void setChecked(bool status);

    GX_PROPERTY(String value, get value, set setValue)

protected:
    virtual void setCheckState(CheckState state);
    virtual bool event(Event *event);

private:
    String m_id;
    Pointer<RadioGroup> m_radioGroup;
};
} // namespace gx
