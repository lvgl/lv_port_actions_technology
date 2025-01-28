/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_sharedref.h"
#include "gx_transform.h"
#include "gx_vector.h"

namespace gx {
class TransformOperations {
public:
    enum Operation { Scale, Rotate, Shear, Skew, Translate, TranslatePercent };

    TransformOperations() GX_DEFAULT;
    TransformOperations(const Transform &matrix) : m_matrix(matrix) {}
    bool isEmpty() const;
    bool hasMatrix() const { return !m_matrix.isIdentity(); }
    void scale(float sx, float sy);
    void rotate(float angle, bool modulus = false);
    void shear(float sh, float sv);
    void skew(float angleX, float angleY);
    void translate(float tx, float ty);
    void translatePercent(float tx, float ty);
    const Transform &matrix(const Size &size = Size()) const;

    TransformOperations blend(const TransformOperations &to, float progress) const;
    bool operator==(const TransformOperations &rhs) const;
    bool operator!=(const TransformOperations &rhs) const { return !(*this == rhs); }

private:
    class Private;
    GX_SHARED_HELPER_DECL(Helper, Private);
    SharedRef<Private, Helper> m_d;
    mutable Transform m_matrix;
};
} // namespace gx
