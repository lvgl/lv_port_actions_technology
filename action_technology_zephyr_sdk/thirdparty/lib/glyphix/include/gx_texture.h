/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_geometry.h"

namespace gx {
class Texture {
public:
    GX_NODISCARD bool isEmpty() const;
    GX_NODISCARD int pixelFormat() const { return data.pixfmt; }
    GX_NODISCARD int pitch() const { return data.pitch; }
    GX_NODISCARD int width() const { return data.width; }
    GX_NODISCARD int height() const { return data.height; }
    GX_NODISCARD uint8_t *pixels() const { return data.pixels; }
    GX_NODISCARD uint8_t *lineSpan(int line) const { return &data.pixels[line * data.pitch]; }
    GX_NODISCARD uint32_t *clut() const { return data.clut; }
    GX_NODISCARD int clutSize() const { return int(data.clutSize); }
    GX_NODISCARD Size size() const { return Size(width(), height()); }
    GX_NODISCARD Rect rect() const { return Rect(0, 0, width(), height()); }

protected:
    Texture();
    void swap(Texture &src) GX_NOEXCEPT;
    void assign(const Texture &src);
    struct Data {
        uint32_t pixfmt : 5;   //!< The pixel format, valid values are [0, 31].
        uint32_t clutSize : 9; //!< The CLUT size, 0 means no CLUT, up to 256.
        uint32_t pitch : 16;   //!< The pitch in bytes, up to 65535 bytes
        uint32_t flag1 : 1;    //<! Can be used as flag.
        uint32_t flag2 : 1;    //<! Can be used as flag.
        uint32_t extra : 4;    //!< Can be used as flags.
        uint32_t width : 14;   //!< The texture width in pixels, up to 16383
        uint32_t height : 14;  //!< The texture height in pixels, up to 16383
        uint8_t *pixels;       //!< The texture pixels data pointer.
        uint32_t *clut;        //!< The CLUT data pointer.
    } data;
};

const Logger &operator<<(const Logger &, const Texture &);
} // namespace gx
