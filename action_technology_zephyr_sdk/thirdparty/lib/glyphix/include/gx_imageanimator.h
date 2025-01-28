/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_imagebox.h"
#include "gx_timer.h"
#include "gx_valueanimation.h"
#include "gx_widget.h"

namespace gx {
class ImageAnimator : public Widget {
    GX_OBJECT

public:
    enum GX_ENUM PlayState {
        Start GX_ALIAS("start"),
        Pause GX_ALIAS("pause"),
        Stop GX_ALIAS("stop"),
    };

    explicit ImageAnimator(Widget *parent = nullptr);

    GX_NODISCARD int duration() { return m_timer.interval(); }
    GX_NODISCARD PlayState playState() const { return PlayState(m_playState); }
    GX_NODISCARD int repeat() const { return m_repeat; }
    void setDuration(int duration);
    void setImage(const Vector<String> &uri);
    void start();
    void stop();
    void pause();
    void setPlayState(PlayState state);
    void setRepeat(int repeat) { m_repeat = repeat; };

    Signal<PlayState, PlayState> changed;

    GX_PROPERTY(Vector<String> src, set setImage, uri true)
    GX_PROPERTY(int duration, set setDuration, get duration)
    GX_PROPERTY(PlayState play, set setPlayState, get playState, signal changed)
    GX_PROPERTY(int repeat, set setRepeat, get repeat)

protected:
    virtual void paintEvent(PaintEvent *event);

private:
    void onTimeout();

private:
    Vector<Image> m_images;
    Timer m_timer;
    int m_frameIndex;
    int m_repeat;
    int m_imageNumber;
    uint8_t m_playState;
};
} // namespace gx
