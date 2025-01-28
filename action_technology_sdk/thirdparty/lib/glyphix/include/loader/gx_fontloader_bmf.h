/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_fontloader.h"

namespace gx {
class FontLoaderBMF : public FontLoader {
public:
    explicit FontLoaderBMF();
    virtual ~FontLoaderBMF();
    virtual FontDriver *load(const String &face, const FontAttribute &attr);
};
} // namespace gx
