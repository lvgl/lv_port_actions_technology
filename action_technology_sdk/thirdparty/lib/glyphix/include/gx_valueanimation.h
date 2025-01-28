/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_animation.h"

namespace gx {
class Pen;
class Brush;
class Color;
class Length;
class Transform;
class TransformOperations;

// This is the default evaluator algorithm. For values a, b and progress,
// the evaluation method is: a * (1-progress) + b * progress.
// Therefore, type T needs to support addition and scalar multiplication.
template<typename T> struct ValueEvaluator {
    typedef T value_type;
    T operator()(const T &a, const T &b, float progress) {
        return T(a * (1 - progress) + b * progress);
    }
};

template<> struct ValueEvaluator<Pen> {
    typedef Pen value_type;
    Pen operator()(const Pen &a, const Pen &b, float progress);
};

template<> struct ValueEvaluator<Brush> {
    typedef Brush value_type;
    Brush operator()(const Brush &a, const Brush &b, float progress);
};

template<> struct ValueEvaluator<Color> {
    typedef Color value_type;
    Color operator()(const Color &a, const Color &b, float progress);
};

template<> struct ValueEvaluator<Length> {
    typedef Length value_type;
    Length operator()(const Length &a, const Length &b, float progress);
};

template<> struct ValueEvaluator<Transform> {
    typedef Transform value_type;
    Transform operator()(const Transform &a, const Transform &b, float progress);
};

template<> struct ValueEvaluator<TransformOperations> {
    typedef TransformOperations value_type;
    value_type operator()(const TransformOperations &a, const TransformOperations &b,
                          float progress);
};

template<class T1, class T2> struct ValueEvaluator<std::pair<T1, T2> > {
    typedef std::pair<T1, T2> value_type;
    value_type operator()(const value_type &a, const value_type &b, float progress) {
        return value_type(ValueEvaluator<T1>()(a.first, b.first, progress),
                          ValueEvaluator<T2>()(a.second, b.second, progress));
    }
};

template<typename T> class ValueAnimation : public AbstractAnimation {
public:
    GX_NODISCARD const T &startValue() const { return m_start; }
    GX_NODISCARD const T &stopValue() const { return m_stop; }
    GX_NODISCARD T evaluator(float progress) const {
        return ValueEvaluator<T>()(startValue(), stopValue(), progress);
    }
    void setStartValue(const T &x) { m_start = x; }
    void setStopValue(const T &x) { m_stop = x; }
    void setValueLimits(const T &start, const T &stop) {
        m_start = start;
        m_stop = stop;
    }
    void setValueDelta(const T &start, const T &delta) {
        m_start = start;
        m_stop = start + delta;
    }

    Signal<T> value;

protected:
    virtual void playFrame(float progress) { value(evaluator(progress)); }

private:
    T m_start, m_stop;
};
} // namespace gx
