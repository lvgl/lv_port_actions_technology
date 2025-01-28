/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_svgimage.h"

namespace gx {
class Painter;

class SVGRender {
public:
    explicit SVGRender(const SVGImage &svg);

    void render(Painter *painter);

private:
    SVGImage m_svg;
};
} // namespace gx
