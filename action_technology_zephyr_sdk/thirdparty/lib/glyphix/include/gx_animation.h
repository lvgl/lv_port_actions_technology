/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_object.h"
#include "gx_signals.h"

namespace gx {
class AbstractEaseCurve;

class AbstractAnimation : public PrimitiveObject {
public:
    //! The deletion policy of animation object.
    enum DeletePolicy {
        //! Delete animation object manually.
        DeleteManually = 0,
        //! Automatically deletes the aniamtion object at the end of the animation.
        DeleteOnStop = 1
    };
    explicit AbstractAnimation(DeletePolicy policy = DeleteManually);
    virtual ~AbstractAnimation();

    GX_NODISCARD int duration() const { return int(m_duration); }
    GX_NODISCARD int repeat() const { return m_repeat; }
    GX_NODISCARD float progress() const;
    GX_NODISCARD AbstractEaseCurve *easeCurve() const { return m_easeCurve; }
    GX_NODISCARD bool isPlay() const { return m_play; }
    GX_NODISCARD DeletePolicy deletePolicy() const { return DeletePolicy(m_deletePolicy); }
    void setDuration(int xms);
    void setRepeat(int repeat) { m_repeat = repeat; }
    void setEaseCurve(AbstractEaseCurve *easeCurve);
    void setFrame(float progress);
    /**
     * Start the animation.
     * @param policy The deletion policy of animation object.
     * @return Return true if the animation was started.
     * @see DeletePolicy
     */
    bool start(DeletePolicy policy);
    bool start() { return start(deletePolicy()); }
    bool startFrame(float progress, DeletePolicy policy = DeleteManually);
    void stop();

    Signal<> finished;

protected:
    virtual void playFrame(float progress) = 0;
    virtual void stopped();

private:
    void tickSlot(uint32_t ms);

private:
    uint32_t m_play : 1;
    uint32_t m_deletePolicy : 1;
    uint32_t m_duration : 30;
    int m_repeat;
    uint32_t m_tickStart;
    AbstractEaseCurve *m_easeCurve;
};
} // namespace gx
