/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_pixmappaintdevice.h"
#include "gx_texture.h"

namespace gx {
/**
 * A pixmap stored in RAM that can be used as a Texture or PixmapPaintDevice. The pixels in
 * Pixmap are arranged contiguously with row-major order, and only the built-in enumerates in
 * PixelFormat::Format are supported.
 * @note The Pixmap is often used for screenshots because it can be used as both paint device and
 * source texture.
 */
class Pixmap : public PixmapPaintDevice {
public:
    //! Constructs an empty Pixmap.
    Pixmap();
    /**
     * Constructs a Pixmap object with the specified parameters. All pixels of the pixmap are
     * allocated and initialized after construction.
     * @param width The width of the pixmap.
     * @param height The height of the pixmap.
     * @param format The pixel format of the pixmap. Defaults to PixelFormat::UnknownFormat and it
     * will be converted to `Application::defaultPixelFormat(false)`.
     * @see Application::defaultPixelFormat()
     * @see Application::setDefaultPixelFormat()
     */
    Pixmap(int width, int height, int format = PixelFormat::UnknownFormat);
    Pixmap(const Pixmap &other);
#ifndef GX_COPMATIBLE_CXX_98
    Pixmap(Pixmap &&other) noexcept;
#endif
    virtual ~Pixmap();
    Pixmap &operator=(const Pixmap &other);
    GX_NODISCARD Pixmap copy() const;
    void clear();
    void blur(int radius);

private:
    struct Data;
    struct Data *m_data;
};
} // namespace gx
