/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_animation.h"
#include "gx_property.h"
#include "gx_variantevaluator.h"

namespace gx {
class PropertyAnimation : public AbstractAnimation {
public:
    explicit PropertyAnimation(const Property &property);
    PropertyAnimation(PrimitiveObject *object, const char *name);
    PropertyAnimation(PrimitiveObject *object, const String &name);

    void setStartValue(const Variant &variant) { m_evaluator.setValueFrom(variant); }
    void setStopValue(const Variant &variant) { m_evaluator.setValueTo(variant); }

protected:
    virtual void playFrame(float progress);

private:
    Property m_property;
    VariantEvaluator m_evaluator;
};
} // namespace gx
