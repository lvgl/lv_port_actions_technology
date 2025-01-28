/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_image.h"
#include "gx_svgimage.h"
#include "gx_widget.h"

namespace gx {
class ImageBox : public Widget {
    GX_OBJECT

public:
    explicit ImageBox(Widget *parent = nullptr);
    virtual ~ImageBox();

    GX_NODISCARD Size imageSize() const;
    GX_NODISCARD ObjectFitMode objectFit() const { return ObjectFitMode(m_objectFit); }
    GX_NODISCARD bool isCacheable() const { return m_cacheable; }
    GX_NODISCARD bool isAsyncDecode() const { return m_asyncDecode; }
    GX_NODISCARD String imageUri() const;
    void setImage(const Image &image);
    void setImage(const SVGImage &image);
    void setImage(const String &uri);
    void setObjectFit(ObjectFitMode fit);
    void setCacheable(bool enabled);
    void setAsyncDecode(bool enabled);
    GX_NODISCARD virtual Size sizeHint() const;

    GX_PROPERTY(String src, set setImage, get imageUri, uri true)
    GX_PROPERTY(ObjectFitMode objectFit, get objectFit, set setObjectFit)
    GX_PROPERTY(bool noCache, set setNoCache, get isNoCache)
    GX_PROPERTY(bool async, set setAsyncDecode, get isAsyncDecode)

protected:
    virtual int baseline(int) const;
    virtual void updateStyle();
    virtual void paintEvent(PaintEvent *event);

private:
    GX_NODISCARD bool isNoCache() const { return !isCacheable(); }
    void setNoCache(bool enabled) { setCacheable(!enabled); }
    void destroy();

private:
    struct Data;
    union {
        uint8_t image[sizeof(Image)];
        uint8_t svg[sizeof(SVGImage)];
    } u;
    uint8_t m_type : 2;
    uint8_t m_cacheable : 1;
    uint8_t m_asyncDecode : 1;
    uint8_t m_decoding : 1;
    uint16_t m_objectFit;
};
} // namespace gx
