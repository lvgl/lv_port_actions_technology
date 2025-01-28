/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_jsvm.h"

namespace gx {
class Style;
class Styles;
class Object;

namespace js {
Style parseStyle(const JsValue &value, Object *owner = nullptr);
Styles parseStyles(const JsValue &value, Object *owner = nullptr);
JsValue compileStyles(const JsValue &value, Object *owner = nullptr);
} // namespace js
} // namespace gx
