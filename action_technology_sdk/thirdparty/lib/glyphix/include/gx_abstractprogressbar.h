/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_widget.h"

namespace gx {
template<typename T> class ValueAnimation;

class AbstractProgressBar : public Widget {
    GX_OBJECT

public:
    explicit AbstractProgressBar(Widget *parent = nullptr);
    virtual ~AbstractProgressBar();

    GX_NODISCARD int minimum() const { return m_minimum; }
    GX_NODISCARD int maximum() const { return m_maximum; }
    GX_NODISCARD int range() const { return maximum() - minimum(); }
    GX_NODISCARD int value() const { return m_value; }
    GX_NODISCARD bool isBusy() const { return m_busy; }

    void setMinimum(int value);
    void setMaximum(int value);
    void setValue(int value);
    void setBusy(bool enabled);

    GX_PROPERTY(int max, get maximum, set setMaximum)
    GX_PROPERTY(int min, get minimum, set setMinimum)
    GX_PROPERTY(int value, get value, set setValue)
    GX_PROPERTY(bool busy, get isBusy, set setBusy)

protected:
    GX_NODISCARD float busyPogress() const { return float(m_busyPogress) * (1.f / 65535); }

private:
    void onBusyProgress(int value);

private:
    int m_minimum, m_maximum, m_value;
    uint8_t m_busy;
    uint16_t m_busyPogress;
    ValueAnimation<int> *m_animation;
};
} // namespace gx
