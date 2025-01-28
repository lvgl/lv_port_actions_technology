/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_animation.h"

namespace gx {
namespace easing {
/**
 * The `CubicBezier` class is designed for interpolation animations, specifically to represents
 * easing effects. It implements a cubic Bezier curve with a fixed starting point `(0, 0)` and an
 * ending point `(1, 1)`. Users can customize the curve's behavior by specifying two control
 * points within a limited range that influences the curve's shape.
 *
 * This class enables the creation of smooth transitions between animation states by adjusting the
 * control points, which control the rate of change over time. The parameter `t` in the `operator()`
 * function typically varies from 0 to 1, representing the progression along the curve. By
 * manipulating the control points, developers can achieve various easing effects, such as
 * acceleration or deceleration, for their animations.
 */
class CubicBezier {
public:
    /**
     * Default constructor, creating a cubic Bezier curve for linear interpolation.
     */
    CubicBezier() : m_x1(0), m_y1(0), m_x2(1), m_y2(1) {}

    /**
     * Constructor, creates a cubic Bezier curve using specified control points.
     * @param x1 The x-coordinate of the first control point, where `0 <= x1 <= 1`.
     * @param y1 The y-coordinate of the first control point.
     * @param x2 The x-coordinate of the second control point, where `0 <= x2 <= 1`.
     * @param y2 The y-coordinate of the second control point.
     * @note The control points (x1, y1) and (x2, y2) define the shape of the Bézier curve, with
     * (x1, y1) being the starting point and (x2, y2) influencing the curve's direction and shape
     * near the ending point. The coordinates should be in the range [0, 1].
     */
    CubicBezier(float x1, float y1, float x2, float y2) : m_x1(x1), m_y1(y1), m_x2(x2), m_y2(y2) {}

    /**
     * This function call operator overload computes the y-coordinate of a point on the cubic Bezier
     * curve for the given \p x coordinate. Internally, it solves for the curve parameter t
     * implicitly based on the \p x.
     * @param x The x-coordinate at which to evaluate the curve, where `0 <= x <= 1`.
     * @return The corresponding y-coordinate on the curve.
     */
    float operator()(float x) const;

private:
    float m_x1, m_y1, m_x2, m_y2;
};

/**
 * \brief Representing a linear easing curve.
 *
 * The Linear class provides a simple overload of the function call operator, which, in the context
 * of animation easing or similar scenarios, implements an identity transformation where the output
 * value is identical to the input value. This is particularly useful for creating smooth and
 * consistent motion in animations where no acceleration or deceleration is desired.
 */
struct Linear {
    /**
     * Overloaded function call operator that serves as the linear transformation.
     * @param x The input float value representing the current state in the animation.
     * @return Returns the input float value unchanged, reflecting a linear progression.
     */
    float operator()(float x) { return x; }
};

/**
 * \brief Representing a cubic-bezier easing curve equivalent to cubic-bezier(0.25, 0.1, 0.25, 1).
 *
 * The curve defined by these control points creates a smooth transition with a slight ease-in at
 * the beginning and a standard ease-out towards the end, providing a gentle and aesthetically
 * pleasing motion profile.
 */
struct Ease {
    /**
     * Applies the cubic-bezier easing to a given input value.
     * @param x The input value, usually in the range [0, 1], representing the normalized time of
     * the animation or transition.
     * @return The eased value, which has been transformed according to the cubic-bezier easing
     * curve, suitable for interpolating animation properties.
     */
    float operator()(float x);
};

/**
 * \brief Representing a cubic-bezier easing curve equivalent to cubic-bezier(0.42, 0, 1, 1).
 *
 * The curve defined by these control points results in a strong initial push, making the animated
 * object appear to spring into action quickly, followed by a smooth completion.
 */
struct EaseIn {
    /**
     * Applies the cubic-bezier ease-in effect to a given input value.
     * @param x The input value, usually in the range [0, 1], representing the normalized time of
     * the animation or transition.
     * @return The eased value, transformed according to the cubic-bezier ease-in curve, suitable
     * for interpolating animation properties.
     */
    float operator()(float x);
};

/**
 * \brief Representing a cubic-bezier easing curve equivalent to cubic-bezier(0, 0, 0.58, 1).
 *
 * The curve defined by these control points ensures that the animated object slows down smoothly
 * near the end, giving the impression of a natural and controlled termination.
 */
struct EaseOut {
    /**
     * Applies the cubic-bezier ease-out effect to a given input value.
     * @param x The input value, usually in the range [0, 1], representing the normalized time of
     * the animation or transition.
     * @return The eased value, transformed according to the cubic-bezier ease-out curve, suitable
     * for interpolating animation properties.
     */
    float operator()(float x);
};

/**
 * \brief Representing a cubic-bezier easing curve equivalent to cubic-bezier(0.42, 0, 0.58, 1).
 *
 * The curve defined by these control points offers a balanced and visually appealing motion
 * profile, suitable for scenarios where a natural and seamless transition is desired.
 */
struct EaseInOut {
    /**
     * Applies the cubic-bezier ease-in-out effect to a given input value.
     * @param x The input value, usually in the range [0, 1], representing the normalized time of
     * the animation or transition.
     * @return The eased value, transformed according to the cubic-bezier ease-in-out curve,
     * suitable for interpolating animation properties.
     */
    float operator()(float x);
};

/**
 * \brief Class simulating spring-damper motion for animation easing.
 *
 * The Spring class models the behavior of a spring oscillator, emulating the natural oscillation
 * and damping effects of a spring system in physics. It can be used to create dynamic and realistic
 * animations, giving the appearance of a spring's movement when transitioning between states.
 *
 * The class takes into account three main physical parameters: spring constant, damping
 * coefficient, and mass. These parameters affect the speed and amplitude of the oscillation, with a
 * higher spring constant resulting in more rapid oscillations, increased damping leading to quicker
 * decay, and a larger mass causing slower motion.
 */
class Spring {
public:
    /**
     * Constructs a Spring object with the specified spring, damping, and mass coefficients.
     * @param spring The spring constant (k) that determines the stiffness of the spring. Higher
     * values result in more rapid oscillation.
     * @param damping The damping ratio (c) that affects how quickly the spring comes to rest. A
     * higher value means faster damping.
     * @param mass The mass (m) attached to the spring. A larger mass results in slower oscillation.
     */
    explicit Spring(float spring = 1, float damping = 1, float mass = 1);
    /**
     * Applies the spring-damper easing effect to a given input value.
     * @param x The input value, usually in the range [0, 1], representing the normalized time of
     * the animation or transition.
     * @return The eased value, transformed according to the spring-damper model, suitable for
     * interpolating animation properties.
     */
    float operator()(float x) const;

private:
    float m_tau, m_omega;
};
} // namespace easing

/**
 * Defines the fundamental behavior for easing curves, which are instrumental in determining the
 * rate of change of an object's motion during animations. This is an abstract class meant to be
 * subclassed with specific easing behaviors.
 */
class AbstractEaseCurve {
public:
    AbstractEaseCurve() GX_DEFAULT;
    virtual ~AbstractEaseCurve() GX_DEFAULT;

    /**
     * Computes the value of the easing curve at a given input time.
     * @param x Represents the normalized time in the animation sequence.
     * @return The eased value at the specified time point, depending on the curve's definition.
     */
    virtual float evaluate(float x) = 0;
};

/**
 * @tparam T A template parameter representing a functor type that defines the actual easing
 * behavior for this curve, e.g. \ref easing::Ease, \ref easing::EaseIn.
 * @brief An extension of the \ref AbstractEaseCurve class using template specialization to allow
 * custom easing functions to be used directly as easing curves.
 *
 * This class acts as a wrapper around a user-defined functor, which computes the easing value
 * for a given time input. It simplifies the process of creating and using custom easing curves
 * without needing to create a separate derived class for each curve.
 *
 * @see easing::Linear, easing::Ease, easing::EaseIn, easing::EaseOut, easing::EaseInOut
 */
template<class T> struct EaseCurve : AbstractEaseCurve {
    virtual float evaluate(float x) { return T()(x); }
};

/**
 * @brief A concrete implementation of the AbstractEaseCurve class that represents a cubic Bézier
 * easing curve.
 *
 * This class calculates the eased value of an animation using a cubic Bezier curve defined by
 * control points. The Bezier curve provides a flexible way to create complex and smooth transitions
 * between the start and end of an animation.
 * @see easing::CubicBezier
 */
class CubicBezierEaseCurve : public AbstractEaseCurve {
public:
    /**
     * Constructs a CubicBezierEaseCurve with the specified control points.
     * @param x1 The x-coordinate of the first control point, where `0 <= x1 <= 1`.
     * @param y1 The y-coordinate of the first control point.
     * @param x2 The x-coordinate of the second control point, where `0 <= x2 <= 1`.
     * @param y2 The y-coordinate of the second control point.
     */
    CubicBezierEaseCurve(float x1, float y1, float x2, float y2)
        : m_x1(x1), m_y1(y1), m_x2(x2), m_y2(y2) {}
    /**
     * Evaluates the cubic Bezier curve at the given time x.
     * @param x Representing the normalized time in the animation sequence, where 0 signifies the
     * start and 1 represents the end of the animation.
     * @return The eased value at time x, calculated using the cubic Bezier curve defined by the
     * control points.
     */
    virtual float evaluate(float x);

private:
    float m_x1, m_y1, m_x2, m_y2;
};

/**
 * @brief A concrete implementation of the AbstractEaseCurve class that simulates a spring-damper
 * system for easing.
 *
 * This class models the easing behavior of a spring with configurable spring constant, damping
 * ratio, and mass, providing a realistic and smooth transition effect for animations. The easing
 * curve mimics the oscillatory motion of a spring as it approaches equilibrium.
 * @see easing::Spring
 */
class SpringEaseCurve : public AbstractEaseCurve {
public:
    /**
     * Constructs a SpringEaseCurve with the specified spring, damping, and mass parameters.
     * @param spring The spring constant (k) that determines the stiffness of the spring. Higher
     * values result in more rapid oscillation.
     * @param damping The damping ratio (c) that affects how quickly the spring comes to rest. A
     * higher value means faster damping.
     * @param mass The mass (m) attached to the spring. A larger mass results in slower oscillation.
     */
    explicit SpringEaseCurve(float spring = 0, float damping = 0, float mass = 1)
        : m_spring(spring, damping, mass) {}
    /**
     * Evaluates the spring-damper system at the given time x.
     * @param x Representing the normalized time in the animation sequence, where 0 signifies the
     * start and 1 represents the end of the animation.
     * @return The eased value at time x, calculated using the simulated spring-damper system.
     */
    virtual float evaluate(float x) { return m_spring(x); }

private:
    easing::Spring m_spring;
};

/**
 * @brief A factory class for creating various types of easing curves.
 *
 * This class provides a convenient interface to generate different easing curves, including linear,
 * various types of Bezier-based easing, and a spring-damper system. It allows users to configure
 * the easing parameters and obtain the corresponding AbstractEaseCurve objects for use in
 * animations.
 *
 */
class EaseCurveFactory {
public:
    //! Enumerates the available easing functions.
    enum EaseFunction {
        Linear,      //!< No easing, uniform speed.
        Ease,        //!< A smooth acceleration and deceleration.
        EaseIn,      //!< Accelerates from zero to maximum speed.
        EaseOut,     //!< Decelerates from maximum speed to zero.
        EaseInOut,   //!< Combines EaseIn and EaseOut, accelerating and then decelerating.
        CubicBezier, //!< Custom cubic Bezier easing defined by control points.
        Spring       //!< Spring-damper system easing.
    };
    /**
     * Constructs an EaseCurveFactory with the specified easing function and parameters.
     * @param function The easing function to use. Defaults to Ease if not specified.
     * @param a Parameters for the selected easing function.
     * - For Linear, Ease, EaseIn, EaseOut, EaseInOut: Not applicable.
     * - For CubicBezier: The x-coordinate of the first control point.
     * - For Spring: The spring constant (k).
     * @param b Parameters for the selected easing function.
     * - For Linear, Ease, EaseIn, EaseOut, EaseInOut: Not applicable.
     * - For CubicBezier: The y-coordinate of the first control point.
     * - For Spring: The damping ratio (c).
     * @param c Parameters for the selected easing function.
     * - For Linear, Ease, EaseIn, EaseOut, EaseInOut: Not applicable.
     * - For CubicBezier: The x-coordinate of the second control point.
     * - For Spring: The mass (m) attached to the spring.
     * @param d Parameters for the selected easing function.
     * - For Linear, Ease, EaseIn, EaseOut, EaseInOut: Not applicable.
     * - For CubicBezier: The y-coordinate of the second control point.
     * - For Spring: Not applicable.
     * @see set()
     */
    explicit EaseCurveFactory(EaseFunction function = Ease, float a = 0, float b = 0, float c = 0,
                              float d = 0);
    /**
     * Sets the easing function and parameters for the factory. The arguments are interpreted
     * differently depending on the selected easing function, see the construct function.
     * @see EaseCurveFactory()
     */
    void set(EaseFunction function = Ease, float a = 0, float b = 0, float c = 0, float d = 0);
    /**
     * Creates an AbstractEaseCurve instance based on the current function and parameters.
     * @return A pointer to the new AbstractEaseCurve object.
     */
    GX_NODISCARD AbstractEaseCurve *easeCurve() const;

private:
    int16_t m_a, m_b, m_c, m_d; // Q6.10
};
} // namespace gx
