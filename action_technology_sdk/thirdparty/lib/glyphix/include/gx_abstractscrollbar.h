/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_widget.h"

namespace gx {
class AbstractScrollBar : public Widget {
    GX_OBJECT

public:
    explicit AbstractScrollBar(Widget *parent = nullptr);
    virtual ~AbstractScrollBar();

    void setPageStep(int pageStep);
    void setMinimum(int value);
    void setMaximum(int value);
    void setValue(int value);
    void setRange(int minimum, int maximum);

    GX_NODISCARD int pageStep() const { return m_pageStep; }
    GX_NODISCARD int minimum() const { return m_minimum; }
    GX_NODISCARD int maximum() const { return m_maximum; }
    GX_NODISCARD int value() const { return m_value; }

    GX_PROPERTY(int pageStep, get pageStep, set setPageStep)
    GX_PROPERTY(int maximum, get maximum, set setMaximum)
    GX_PROPERTY(int minimum, get minimum, set setMinimum)
    GX_PROPERTY(int value, get value, set setValue)

private:
    int m_minimum, m_maximum;
    int m_value, m_pageStep;
};
} // namespace gx
