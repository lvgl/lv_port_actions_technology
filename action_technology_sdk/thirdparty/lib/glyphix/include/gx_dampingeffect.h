/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_prelude.h"

namespace gx {
//! The damping effect solver.
class DampingEffect {
public:
    /**
     * The damping effect solver is constructed from the initial parameters.
     * @param v0 The modulus of the initial velocity vector (unit: pixels/sec). Throughout the
     * damping movement, the velocity will gradually decrease from \p v0 to 0.
     * @param d The damping factor determines how fast the velocity attenuates (default: 1.5). The
     * damping factor does not attenuation the velocity linearly, in fact, the higher the velocity,
     * the faster the attenuation.
     * @param a A fixed acceleration (unit: pixels/sec^2, default: 10). The fixed acceleration stops
     * the animation when the velocity is very small, in which case the damping factor is very weak.
     */
    DampingEffect(float v0, float d, float a) : v0(v0), d(d), a(a) {}
    //! Get the displacement at time \p t.
    GX_NODISCARD float position(float t) const;
    //! Get the velocity at time \p t.
    GX_NODISCARD float velocity(float t) const;
    //! Get the duration it takes for the movement to maximum position.
    GX_NODISCARD float duration() const;
    //! Get the duration at the specified position \p x.
    GX_NODISCARD float duration(float x) const;

private:
    float v0, d, a; // NOLINT(readability-*)
};

class BounceDampingEffect {
public:
    BounceDampingEffect();
    void setDamping(int velocity, int distance);
    void setDamping(float damping, int velocity, int distance);
    void setDuration(int duration) { m_duration = duration; }
    GX_NODISCARD int duration() const { return m_duration; }
    GX_NODISCARD int distance() const { return int(m_distance); }
    GX_NODISCARD float progress(float value) const;
    GX_NODISCARD float progress(float damping, float value) const;

private:
    uint16_t m_duration; // sec
    uint16_t m_bounceDuration, m_bounceVelocity;
    uint16_t m_velocity;
    float m_distance;
};
} // namespace gx
