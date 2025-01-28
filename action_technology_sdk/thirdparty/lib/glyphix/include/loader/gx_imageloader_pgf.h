/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_imageloader.h"

namespace gx {
class ImageLoaderPGF : public ImageLoader {
public:
    ImageLoaderPGF() : ImageLoader("pgf") {}
    virtual Error load(Image::Data &data, File &file, bool withPixel);
};
} // namespace gx
