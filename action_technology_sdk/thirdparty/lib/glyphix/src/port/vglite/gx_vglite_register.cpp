/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#include "gx_vglite_engine.h"
#include "gx_vglite_config.h"
#include <stdio.h>

using namespace gx;

void gx_engine_register(void)
{
    PixmapPaintEngine::setFactory<VGLiteEngine>();
    GPU_LOG_I("gx using VGLite gpu.\n");
}
