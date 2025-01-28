/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_variant.h"

namespace gx {
class VariantEvaluator {
    typedef Variant (*Evaluator)(const Variant &, const Variant &, float);

public:
    explicit VariantEvaluator(Variant::type_t type);
    GX_NODISCARD Variant::type_t type() const { return m_type; }
    void setValueFrom(const Variant &a);
    void setValueTo(const Variant &b);
    GX_NODISCARD Variant evaluate(float progress) const;

private:
    Variant::type_t m_type;
    Variant m_a, m_b;
    Evaluator m_evaluator;
};
} // namespace gx
