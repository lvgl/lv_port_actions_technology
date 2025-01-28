/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_easecurve.h"
#include "gx_style.h"
#include "gx_valueanimation.h"
#include "gx_variantevaluator.h"

namespace gx {
class Widget;

class StyleAnimation : public AbstractAnimation {
public:
    class Keyframes;
    virtual ~StyleAnimation();
    Keyframes &keyframes(const char *role);
    Keyframes &keyframes(Style::Role role);
    void clear();

    Signal<float> update;

protected:
    virtual void playFrame(float progress);
    virtual void stopped();

private:
    explicit StyleAnimation(Style *style);
    Style *m_style;
    Vector<Keyframes> m_group;
    friend class Style;
};

class StyleAnimation::Keyframes {
public:
    /**
     * Set a keyframe at the given progress.
     * @param progress The percentage (integer) of the animation to evaluate.
     * @param value The key-frame value to evaluate at the given progress.
     * @return This keyframe object, used by chaining call.
     */
    Keyframes &setKeyframe(int progress, const Variant &value);
    Style::Role role() const { return m_role; }
    Variant evaluate(float progress) const;

private:
    explicit Keyframes(Style::Role role);
    Style::Role m_role;
    mutable VariantEvaluator m_evaluator;
    Vector<std::pair<int, Variant> > m_keyframes;

    friend class StyleAnimation;
};

template<> struct ValueEvaluator<Style::Border> {
    typedef Style::Border value_type;
    Style::Border operator()(const Style::Border &a, const Style::Border &b, float progress);
};

inline StyleAnimation::Keyframes &StyleAnimation::keyframes(const char *role) {
    return keyframes(Style::role(role).first);
}

class StyleKeyframes {
public:
    typedef Vector<std::pair<int, Style> >::const_iterator const_iterator;

    StyleKeyframes() : m_repeat(), m_duration(1000) {}
    ~StyleKeyframes() { clear(); }

    GX_NODISCARD const_iterator begin() const { return m_keyframes.begin(); };
    GX_NODISCARD const_iterator end() const { return m_keyframes.end(); };
    GX_NODISCARD Style &keyframe(int progress);
    GX_NODISCARD Style &keyframe(int progress) const;
    GX_NODISCARD int repeat() const { return m_repeat; };
    GX_NODISCARD int duration() const { return m_duration; }
    GX_NODISCARD AbstractEaseCurve *easeCurve() const { return m_easeCurve.easeCurve(); }
    void setDuration(int msec) { m_duration = msec; }
    void setRepeat(int repeat) { m_repeat = repeat; };
    void setEaseCurve(const EaseCurveFactory &curve) { m_easeCurve = curve; }
    void setEaseCurve(EaseCurveFactory::EaseFunction function, float a, float b, float c, float d) {
        m_easeCurve.set(function, a, b, c, d);
    }
    void clear();

private:
    Vector<std::pair<int, Style> > m_keyframes;
    int m_repeat, m_duration;
    EaseCurveFactory m_easeCurve;
};
} // namespace gx
