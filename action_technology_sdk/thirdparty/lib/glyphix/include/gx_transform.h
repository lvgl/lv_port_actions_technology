/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_geometry.h"

namespace gx {
class Transform {
public:
    enum RotateAxis { AxisX, AxisY, AxisZ };
    struct DecomposedData {
        float sx, sy; // scale
        float angle;
        float ra, rb, rc, rd; // remainder
        float tx, ty;         // translate
    };
    Transform();
    GX_NODISCARD bool isAffine() const;
    GX_NODISCARD bool isIdentity() const;
    GX_NODISCARD bool isInvertible() const;
    GX_NODISCARD bool isTranslating() const;
    void reset();
    Transform &translate(float tx, float ty);
    Transform &translate(const PointF &offset);
    Transform &rotate(float angle);
    Transform &rotate(float angle, RotateAxis axis);
    Transform &scale(float sx, float sy);
    Transform &shear(float sh, float sv);
    Transform &skew(float angleX, float angleY);
    Transform &multiply(const Transform &rhs);
    void map(float *x, float *y) const;
    GX_NODISCARD PointF map(float x, float y) const;
    GX_NODISCARD PointF map(const PointF &point) const { return map(point.x(), point.y()); }
    GX_NODISCARD Point map(int x, int y) const;
    GX_NODISCARD Point map(const Point &point) const { return map(point.x(), point.y()); }
    GX_NODISCARD Line map(const Line &line) const;
    GX_NODISCARD Rect boundRect(const Rect &rect) const;
    GX_NODISCARD float determinant() const;
    GX_NODISCARD Transform inverse() const;
    Transform &translateReverse(float tx, float ty);
    Transform &translateReverse(const PointF &offset);

    GX_NODISCARD static Transform fromTranslate(const PointF &offset);
    GX_NODISCARD static Transform fromTranslate(float tx, float ty);

    GX_NODISCARD float m11() const { return m_matrix[0][0]; }
    GX_NODISCARD float m12() const { return m_matrix[0][1]; }
    GX_NODISCARD float m13() const { return m_matrix[0][2]; }
    GX_NODISCARD float m21() const { return m_matrix[1][0]; }
    GX_NODISCARD float m22() const { return m_matrix[1][1]; }
    GX_NODISCARD float m23() const { return m_matrix[1][2]; }
    GX_NODISCARD float m31() const { return m_matrix[2][0]; }
    GX_NODISCARD float m32() const { return m_matrix[2][1]; }
    GX_NODISCARD float m33() const { return m_matrix[2][2]; }
    void setM11(float v) { m_matrix[0][0] = v; }
    void setM12(float v) { m_matrix[0][1] = v; }
    void setM13(float v) { m_matrix[0][2] = v; }
    void setM21(float v) { m_matrix[1][0] = v; }
    void setM22(float v) { m_matrix[1][1] = v; }
    void setM23(float v) { m_matrix[1][2] = v; }
    void setM31(float v) { m_matrix[2][0] = v; }
    void setM32(float v) { m_matrix[2][1] = v; }
    void setM33(float v) { m_matrix[2][2] = v; }
    GX_NODISCARD float *data() const { return (float *)m_matrix; }

    void setMatrix(float m11, float m12, float m13, float m21, float m22, float m23, float m31,
                   float m32, float m33);

    GX_NODISCARD float xScale() const;
    GX_NODISCARD float yScale() const;
    bool decompose(DecomposedData &) const;
    void recompose(const DecomposedData &);
    void blend(const Transform &from, float progress);
    bool operator==(const Transform &rhs) const;
    bool operator!=(const Transform &rhs) const { return !(*this == rhs); }

    GX_NODISCARD static bool squareToQuad(const PointF *quad, Transform &trans);

private:
    enum Flags {
        Identity = 0,
        Translate = 0x01,
        Scale = 0x02,
        Rotate = 0x04,
        Shear = 0x08,
        Project = 0x10
    };

    float m_matrix[3][3];
    uint8_t m_flags;
};

inline bool Transform::isAffine() const { return m_flags < Project; }

inline bool Transform::isIdentity() const { return m_flags == Identity; }

inline bool Transform::isTranslating() const { return m_flags == Translate; }

inline Transform &Transform::translate(const PointF &offset) {
    return translate(offset.x(), offset.y());
}

inline Transform Transform::fromTranslate(const PointF &offset) {
    return fromTranslate(offset.x(), offset.y());
}

inline Transform Transform::fromTranslate(float tx, float ty) {
    return Transform().translate(tx, ty);
}

inline Transform &Transform::translateReverse(const PointF &offset) {
    return translateReverse(offset.x(), offset.y());
}

const Logger &operator<<(const Logger &, const Transform &);
} // namespace gx
