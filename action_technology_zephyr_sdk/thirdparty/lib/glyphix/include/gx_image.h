/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_sharedref.h"
#include "gx_string.h"
#include "gx_texture.h"

namespace gx {
class Image {
public:
    class Data;
    class Texture;

    Image() GX_DEFAULT;
    explicit Image(const String &uri);

    GX_NODISCARD bool isEmpty() const;
    GX_NODISCARD int width() const { return size().width(); }
    GX_NODISCARD int height() const { return size().height(); }
    GX_NODISCARD Size size() const;
    GX_NODISCARD Rect rect() const { return Rect(Point(), size()); }
    GX_NODISCARD String uri() const;

    GX_NODISCARD bool isDecoded() const;
    GX_NODISCARD Texture texture() const;
    GX_NODISCARD bool asyncDecode();

private:
    SharedRef<Data> m_d;
};

template<> struct utils::SharedHelper<Image::Data> {
    static Image::Data *duplicate(const SharedValue &x) GX_DELETE;
    static void destroy(SharedValue *ptr);
};

class Image::Texture : public gx::Texture {
public:
    Texture() GX_DEFAULT;
    explicit Texture(const Image &image);
    Texture(const Texture &other);
    virtual ~Texture();

    Texture &operator=(const Texture &rhs);

private:
    Image m_image;
};

inline Image::Texture Image::texture() const { return Texture(*this); }
} // namespace gx
