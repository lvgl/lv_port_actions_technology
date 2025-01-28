/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_dampingeffect.h"
#include "gx_geometry.h"
#include "gx_valueanimation.h"
#include "gx_widget.h"

namespace gx {
class AbstractScrollArea;
class ScrollArea;

class AbstractScrollEffect {
public:
    virtual ~AbstractScrollEffect() {}

    ValueAnimation<PointF> *animation() { return &m_animation; }
    virtual void setDamping(float value) = 0;
    virtual void scroll(AbstractScrollArea *widget, const Point &offset) = 0;
    virtual void resolve(AbstractScrollArea *widget, const Point &velocity) = 0;
    virtual PointF evaluate(const PointF &progress) = 0;

protected:
    AbstractScrollEffect();

private:
    ValueAnimation<PointF> m_animation;
};

class ScrollEffectList : public AbstractScrollEffect {
public:
    ScrollEffectList();
    GX_NODISCARD float damping() const;
    virtual void setDamping(float value);
    virtual void scroll(AbstractScrollArea *widget, const Point &offset);
    virtual void resolve(AbstractScrollArea *widget, const Point &velocity);
    GX_NODISCARD virtual PointF evaluate(const PointF &progressVec);

protected:
    struct AdjustData {
        AbstractScrollArea *widget;
        Point start, stop, velocity;
    };

    virtual void adjust(AdjustData &data);

private:
    GX_NODISCARD float evaluateX(float progress) const;
    GX_NODISCARD float evaluateY(float progress) const;
    GX_NODISCARD float evaluateEffect(float x, int rebound,
                                      const BounceDampingEffect &effect) const;

protected:
    uint8_t flags;
    uint16_t dampingValue;
    BounceDampingEffect effectX, effectY;
};

class ScrollEffectSnap : public ScrollEffectList {
public:
    ScrollEffectSnap() GX_DEFAULT;

protected:
    virtual void adjust(AdjustData &d);

private:
    template<class T> void scrollSnapEdge(AdjustData &d);
    template<class T> void scrollSnapCenter(AdjustData &d);
    template<class T> GX_NODISCARD Point boundaryAdjust(const AdjustData &d, const Point &stop2);
    void snapAdjust(AdjustData &d, Point stop2, bool snapped);
};

class ScrollEffectSwiper : public AbstractScrollEffect {
public:
    ScrollEffectSwiper();
    virtual void setDamping(float value);
    virtual void scroll(AbstractScrollArea *widget, const Point &offset);
    virtual void resolve(AbstractScrollArea *widget, const Point &velocity);
    GX_NODISCARD virtual PointF evaluate(const PointF &progressVec);

private:
    bool m_rebound;
};
} // namespace gx
