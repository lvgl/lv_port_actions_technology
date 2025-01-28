/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_signals.h"

namespace gx {
class Timer : public NonCopyable<Timer> {
public:
    explicit Timer(int millisecond = 1000);
    virtual ~Timer();

    GX_NODISCARD bool isActive() const { return m_slot != 0x7fffffffu; }
    GX_NODISCARD int interval() const { return m_interval; }
    GX_NODISCARD bool isSingleShot() const { return m_singleShot; }
    void setInterval(int millisecond);
    void setSingleShot(bool singleShot);
    void start(int millisecond);
    void start();
    void stop();

    Signal<> timeout;

protected:
    virtual void timeoutEvent();

private:
    int m_interval;
    uint32_t m_singleShot : 1;
    uint32_t m_slot : 31;
    friend class TimerManager;
};
} // namespace gx
