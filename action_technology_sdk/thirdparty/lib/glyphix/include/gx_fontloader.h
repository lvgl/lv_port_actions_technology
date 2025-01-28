/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_string.h"

namespace gx {
class FontDriver;
struct FontAttribute;

class FontLoader {
public:
    virtual ~FontLoader();
    virtual FontDriver *load(const String &face, const FontAttribute &attr) = 0;

protected:
    FontLoader() : m_next() {}

private:
    FontLoader *m_next;
    friend class FontManager;
    friend class FontLoaderFamily;
};
} // namespace gx
