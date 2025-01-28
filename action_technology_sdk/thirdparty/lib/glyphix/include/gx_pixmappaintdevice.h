/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_paintdevice.h"
#include "gx_texture.h"

namespace gx {
class PixmapPaintDevice : public PaintDevice, public Texture {
public:
    PixmapPaintDevice();
    virtual ~PixmapPaintDevice();
    virtual PaintEngine *paintEngine();
    virtual void metrics(Metric *metric) const;

    using Texture::height;
    using Texture::rect;
    using Texture::size;
    using Texture::width;
};
} // namespace gx
