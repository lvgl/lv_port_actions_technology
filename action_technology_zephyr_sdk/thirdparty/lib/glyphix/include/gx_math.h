/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_prelude.h"
#include <cmath>

#define GX_PI_D (3.14159265358979323846)
#define GX_PI   float(GX_PI_D)
#define GX_PI2  float(GX_PI_D * 2)

namespace gx {
inline float abs(float x) { return std::abs(x); }

inline float sin(float x) { return std::sin(x); }

inline float cos(float x) { return std::cos(x); }

inline float tan(float x) { return std::tan(x); }

inline float atan2(float y, float x) { return std::atan2(y, x); }

inline float hypot(float x, float y) { return std::sqrt(x * x + y * y); }

inline float ceil(float x) { return std::ceil(x); }

inline float floor(float x) { return std::floor(x); }

inline float round(float x) {
#ifdef __CC_ARM
    return x >= 0 ? floor(x + 0.5f) : ceil(x - 0.5f);
#else
    return std::round(x);
#endif
}

inline bool isfinite(float x) {
#ifdef __CC_ARM
    return ::isfinite(x);
#else
    return std::isfinite(x);
#endif
}

inline float invsqrt(float x) { return 1.f / std::sqrt(x); }

inline float sqrt(float x) { return std::sqrt(x); }

inline float log(float x) { return std::log(x); }

inline float pow(float x, float y) { return std::pow(x, y); }

inline float exp(float x) { return std::exp(x); }

inline float deg(float x) { return x * float(180 / GX_PI_D); }

inline float rad(float x) { return x * float(GX_PI_D / 180); }

int sqrt(int x);
int ceilPow2(int x);
float invsqrtFast(float x);
float sinFast(float x);
float cosFast(float x);
float atan2Fast(float x, float y);
float lambertW(float z);
} // namespace gx
