/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_signals.h"
#include "gx_widget.h"

namespace gx {
class AbstractSlider : public Widget {
    GX_OBJECT

public:
    explicit AbstractSlider(Widget *parent = nullptr);
    virtual ~AbstractSlider();

    void setMinimum(int value);
    void setMaximum(int value);
    void setValue(int value);

    GX_NODISCARD int minimum() const { return m_minimum; }
    GX_NODISCARD int maximum() const { return m_maximum; }
    GX_NODISCARD int range() const { return maximum() - minimum(); }
    GX_NODISCARD int value() const { return m_value; }

    //! The changed signal is emitted when the value is changed by gestures, but not by setValue().
    Signal<int> changed;

    GX_PROPERTY(int max, get maximum, set setMaximum)
    GX_PROPERTY(int min, get minimum, set setMinimum)
    GX_PROPERTY(int value, get value, set setValue, signal changed)

private:
    int m_minimum, m_maximum, m_value;
};
} // namespace gx
