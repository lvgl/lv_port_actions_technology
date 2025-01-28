/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_abstractbutton.h"
#include "gx_valueanimation.h"

namespace gx {
class CheckBox : public AbstractButton {
    GX_OBJECT

public:
    explicit CheckBox(Widget *parent = nullptr);
    virtual ~CheckBox();

    GX_NODISCARD bool isChecked() const { return m_checked; }
    virtual void setChecked(bool status);

    using Widget::baseline;
    GX_NODISCARD virtual Size sizeHint() const;

    Signal<bool> checked;

    GX_PROPERTY(bool value, get isChecked, set setChecked, signal checked)

protected:
    void setRadioStyle();
    GX_NODISCARD int scale() const { return m_scale; }
    GX_NODISCARD int transition() const { return m_transition; }

    virtual void setCheckState(CheckState state);
    virtual int baseline(int) const;
    virtual void paintEvent(PaintEvent *event);

private:
    void setTransition(int mode);
    void updateTransition(const std::pair<uint8_t, uint8_t> &x);

private:
    uint32_t m_checked : 1;
    uint32_t m_shapeStyle : 1;
    uint32_t m_scale : 8;
    uint32_t m_transition : 8;
    ValueAnimation<std::pair<uint8_t, uint8_t> > m_animation;
};
} // namespace gx
