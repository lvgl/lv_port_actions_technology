/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_color.h"
#include "gx_geometry.h"

namespace gx {
class LinearGradient : public Gradient {
public:
    LinearGradient();
    LinearGradient(float x1, float y1, float x2, float y2);
    LinearGradient(const PointF &start, const PointF &final);

    GX_NODISCARD const PointF &start() const;
    void setStart(const PointF &point);
    GX_NODISCARD const PointF & final() const;
    void setFinal(const PointF &point);

private:
    class LinearGradientData;
};
} // namespace gx
