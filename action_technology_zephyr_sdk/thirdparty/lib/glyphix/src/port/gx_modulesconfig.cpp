/*
 * This file is part of PersimUI
 * Copyright (c) 2006-2023, RT-Thread Development Team
 */
#include "gx_variantcast.h"
#include "platform/gx_modules.h"

// font loader
#include "gx_fontmanager.h"
#include "gx_freetype.h"
#include "loader/gx_fontloader_bmf.h"

// image loader
#include "gx_imagecache.h"
#include "loader/gx_imageloader_jpeg.h"
#include "loader/gx_imageloader_pgf.h"
#include "loader/gx_imageloader_png.h"

using namespace gx;

GX_CORE_MODULES_MOUNT(app) {
    GX_UNUSED(app);
    variant::setup();

    // install font driver loaders
    app->fontManager()->install(new FontLoaderTTF(new FreeTypeLibrary));
    app->fontManager()->install(new FontLoaderBMF);

    // registered image loaders
    ImageCache::instance()->installLoader(new ImageLoaderPNG);
    ImageCache::instance()->installLoader(new ImageLoaderJPEG);
    ImageCache::instance()->installLoader(new ImageLoaderPGF);
}

GX_CORE_MODULES_UNMOUNT(app) {
    GX_UNUSED(app);
    variant::reset();
}

GX_MODULES_MOUNT(app) { GX_UNUSED(app); }

GX_MODULES_UNMOUNT(app) { GX_UNUSED(app); }
