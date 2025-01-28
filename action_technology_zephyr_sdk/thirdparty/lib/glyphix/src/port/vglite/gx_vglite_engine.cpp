/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#include "gx_vglite_engine.h"
#include "gx_image.h"
#include "gx_painter.h"
#include "gx_pixmap.h"
#include "gx_vglite_adaptor.h"
#include "gx_vglite_config.h"
#include "sys/_intsup.h"
#include "vg_lite.h"
#include <memory/mem_cache.h>

#define GX_LOG_TAG "vglite"
#include "gx_logger.h"

using namespace gx;
#define VGLITE_FORMAT_NONE -1
#define VG_LITE_TAKE_ERROR(func)                                               \
  if ((error = func) != VG_LITE_SUCCESS) {                                     \
    GPU_LOG_E("GPU %d:%s process failed, the error code: <%d>.\n", __LINE__,   \
              __func__, error);                                                \
  }

#define VG_LITE_RETURN_ERROR(func)                                             \
  if ((error = func) != VG_LITE_SUCCESS) {                                     \
    GPU_LOG_E("GPU %d:%s process failed, the error code: <%d>.\n", __LINE__,   \
              __func__, error);                                                \
    return error;                                                              \
  }

#define VG_LITE_JUDGE_ASSERT(func)                                             \
  if ((func) != VG_LITE_SUCCESS) {                                             \
    GPU_LOG_E(                                                                 \
        "The vglite buffer creation failed, there may not be enough heap.\n"); \
    GX_ASSERT(0);                                                              \
  }

static uint32_t gx_color_to_vglite(const Color &color) {
  uint32_t c = 0;
  uint8_t a = color.alpha();
  uint8_t r = (color.red() * a) >> 8;
  uint8_t g = (color.green() * a) >> 8;
  uint8_t b = (color.blue() * a) >> 8;
  return c = (((((c | a) << 8 | b) << 8) | g) << 8) | r;
}

VGLiteEngine::VGLiteEngine(PixmapPaintDevice *device)
    : PixmapPaintEngine(device) {
  static int inited = 0;
  if (inited == 0) {
    if (vg_lite_init(device->width(), device->height()) == VG_LITE_SUCCESS) {
      vg_lite_disable_dither();
      GPU_LOG_I("VGlite Initialization successfully.\n");
      inited = 1;
    } else {
      GPU_LOG_E("VGlite Initialization failed.\n");
    }
  }
}

VGLiteEngine::~VGLiteEngine() { vg_lite_finish(); }

void VGLiteEngine::drawRects(const Rect *rects, int count, const Brush &brush) {
  int error = VG_LITE_SUCCESS;
  vg_lite_set_scissor(clip().left(), clip().top(), clip().right() + 1,
                      clip().bottom() + 1);
  // GPU_LOG_I("GPU drawRects.\n");
  if (brush.type() == Brush::Color) {
    if (transferMode() != Painter::DstIn && brush.color().argb() < 0x01000000)
      return; // do nothing if color is transparent
    for (int i = 0; i < count && error == VG_LITE_SUCCESS; i++)
      error = vglite_draw_rect(rects[i] + origin(),
                               gx_color_to_vglite(brush.color()));
  } else if (brush.type() == Brush::Image) {
    Image::Texture texture(brush.image());
    for (int i = 0; i < count; i++)
      VGLiteEngine::drawTexture(rects[i], texture, texture.rect(), Brush());
  } else if (brush.type() == Brush::Pixmap) {
    const Texture &texture(brush.pixmap());
    for (int i = 0; i < count; i++)
      VGLiteEngine::drawTexture(rects[i], texture, texture.rect(), Brush());
  }
  if (error != VG_LITE_SUCCESS) {
    LogError() << "GPU drawRects failed:" << error;
    vg_lite_finish();
    PixmapPaintEngine::drawRects(rects, count, brush);
  }
}

void VGLiteEngine::drawPath(const VectorPath &path, const Pen &pen) {
  int error = VG_LITE_SUCCESS;
  vg_lite_set_scissor(clip().left(), clip().top(), clip().right() + 1,
                      clip().bottom() + 1);
  // GPU_LOG_I("GPU drawPath.\n");
  error = vglite_fill_path(path.outline(pen), gx_color_to_vglite(pen.color()));
  if (error != VG_LITE_SUCCESS) {
    GPU_LOG_E("GPU drawPath failed, the error code: <%d>.\n", error);
    vg_lite_finish();
    PixmapPaintEngine::drawPath(path, pen);
  }
}

void VGLiteEngine::fillPath(const VectorPath &path, const Brush &brush) {
  int error = VG_LITE_SUCCESS;
  vg_lite_set_scissor(clip().left(), clip().top(), clip().right() + 1,
                      clip().bottom() + 1);
  if (brush.type() == Brush::Color) {
    if (transferMode() != Painter::DstIn && brush.color().argb() < 0x01000000)
      return; // do nothing if color is transparent
    error = vglite_fill_path(path, gx_color_to_vglite(brush.color()));
  } else if (brush.type() == Brush::Image) {
    Image::Texture texture = brush.image().texture();
    error = vglite_fill_path_pattern(path, texture, brush.transform());
  } else if (brush.type() == Brush::Pixmap) {
    const Texture &texture(brush.pixmap());
    error = vglite_fill_path_pattern(path, texture, brush.transform());
  }
  if (error != VG_LITE_SUCCESS) {
    GPU_LOG_E("GPU fillPath failed, the error code: <%d>.\n", error);
    vg_lite_finish();
    PixmapPaintEngine::fillPath(path, brush);
  }
}

void VGLiteEngine::drawTexture(const Rect &dr, const Texture &texture,
                               const Rect &sr, const Brush &br) {
  int error = VG_LITE_SUCCESS;
  vg_lite_set_scissor(clip().left(), clip().top(), clip().right() + 1,
                      clip().bottom() + 1);
  if (transform()) {
    error = vglite_blit_transform(dr, texture, sr, br.color());
  } else {
    Rect dst = (dr + origin()) & clip();
    Point pos = (sr & (clip() - origin() - dr.topLeft())).topLeft();
    Size r = texture.size();
    if (r.width() > clip().width())
      r.setWidth(clip().width());
    if (r.height() > clip().height())
      r.setHeight(clip().height());
    Rect src = Rect(pos, r);
    error = vglite_blit(dst, texture, src, br.color());
  }
  if (error != VG_LITE_SUCCESS) {
    GPU_LOG_E("GPU drawTexture failed, the error code: <%d>.\n", error);
    vg_lite_finish();
    PixmapPaintEngine::drawTexture(dr, texture, sr, br);
  }
}

int VGLiteEngine::vglite_buffer_init(vg_lite_buffer_t &dst,
                                     const Texture &texture, bool source) {
  vg_lite_error_t error;
  int format = vglite::Adaptor::fromat(texture.pixelFormat());
  if (format == VGLITE_FORMAT_NONE) {
    GPU_LOG_W("Buffer fromat index (%d) not support with vglite.\n",
              texture.pixelFormat());
    return VG_LITE_NOT_ALIGNED;
  }

  if (format == VG_LITE_INDEX_8) {
    vg_lite_set_CLUT(256, texture.clut());
  }

  dst.format = static_cast<vg_lite_buffer_format_t>(format);
  dst.tiled = VG_LITE_LINEAR;
  if (format == VG_LITE_RGBA8888_ETC2_EAC) {
    dst.tiled = VG_LITE_TILED;
  }
  if (memset(&dst.yuv, 0, sizeof(dst.yuv)) == nullptr) {
    GPU_LOG_W("vglite_buffer_init memset failed.\n");
    return VG_LITE_NO_CONTEXT;
  }
  dst.image_mode = GPU_VG_LITE_IMAGE_MODE;
  dst.transparency_mode = GPU_VG_LITE_TRANSPARENCY_MODE;
  dst.width = texture.width();
  dst.height = texture.height();
  dst.stride = texture.pitch();
  dst.memory = reinterpret_cast<void *>(texture.pixels());
  dst.handle = nullptr;
  VG_LITE_RETURN_ERROR(vg_lite_map(&dst, VG_LITE_MAP_USER_MEMORY, -1));
  return VG_LITE_SUCCESS;
}

int VGLiteEngine::vglite_fill_rect(const Rect &rect, uint32_t color) {
  vg_lite_buffer_t dst_buf = {};
  vg_lite_error_t error = VG_LITE_SUCCESS;
  VG_LITE_JUDGE_ASSERT(vglite_buffer_init(dst_buf, *device, false));
  vg_lite_rectangle_t dst = {rect.left(), rect.top(), rect.width(),
                             rect.height()};
  VG_LITE_RETURN_ERROR(vg_lite_clear(&dst_buf, &dst, color));
  VG_LITE_RETURN_ERROR(vg_lite_flush());
  mem_dcache_invalidate((const void *)dst_buf.address,
                        dst_buf.height * dst_buf.stride);
  return error;
}

int VGLiteEngine::vglite_draw_rect(const Rect &rect, uint32_t color) {
  vg_lite_buffer_t fb = {};
  vg_lite_error_t error = VG_LITE_SUCCESS;

  VG_LITE_JUDGE_ASSERT(vglite_buffer_init(fb, *device, false));

  vg_lite_path_t path;
  int32_t path_data[] = {
      /* VG rectangular path */
      VLC_OP_MOVE, rect.left(),      rect.top(),
      VLC_OP_LINE, rect.right() + 1, rect.top(),
      VLC_OP_LINE, rect.right() + 1, rect.bottom() + 1,
      VLC_OP_LINE, rect.left(),    rect.bottom() + 1,
      VLC_OP_LINE, rect.left(),   rect.top(),
      VLC_OP_END};

  VG_LITE_RETURN_ERROR(vg_lite_init_path(
      &path, VG_LITE_S32, VG_LITE_LOW, sizeof(path_data), path_data,
      vg_lite_float_t(rect.left()), vg_lite_float_t(rect.top()),
      vg_lite_float_t(rect.right() + 1.0f),
      vg_lite_float_t(rect.bottom() + 1.0f)));

  vg_lite_matrix_t matrix;
  vg_lite_identity(&matrix);

  error = vg_lite_draw(&fb, &path, VG_LITE_FILL_NON_ZERO, &matrix,
                       VG_LITE_BLEND_SRC_OVER, color);
  if (error != VG_LITE_SUCCESS) {
    vg_lite_clear_path(&path);
    return error;
  }
  VG_LITE_RETURN_ERROR(vg_lite_flush());
  VG_LITE_RETURN_ERROR(vg_lite_clear_path(&path));
  return error;
}

int VGLiteEngine::vglite_blit(const Rect &dr, const Texture &texture,
                              const Rect &sr, Color color) {
  if (dr.isEmpty() || sr.isEmpty())
    return VG_LITE_SUCCESS;

  if (sr.x() > texture.rect().width() || sr.y() > texture.rect().height())
    return VG_LITE_SUCCESS;

  vg_lite_buffer_t src_vgbuf = {};
  vg_lite_buffer_t dst_vgbuf = {};
  int error = VG_LITE_SUCCESS;

  if (!texture.pixels())
    return error;

  VG_LITE_JUDGE_ASSERT(vglite_buffer_init(dst_vgbuf, *device, false));
  VG_LITE_JUDGE_ASSERT(vglite_buffer_init(src_vgbuf, texture, true));

  vg_lite_rectangle_t rect = {};
  rect.x = (uint32_t)(sr.x());
  rect.y = (uint32_t)(sr.y());
  rect.width = (uint32_t)(sr.width() > dr.width() ? dr.width() : sr.width());
  rect.height =
      (uint32_t)(sr.height() > dr.height() ? dr.height() : sr.height());

  vg_lite_matrix_t matrix = {};
  vg_lite_identity(&matrix);
  vg_lite_blend_t blend = VG_LITE_BLEND_SRC_OVER;

  vg_lite_translate(static_cast<vg_lite_float_t>(dr.left()),
                    static_cast<vg_lite_float_t>(dr.top()), &matrix);

  if (texture.pixelFormat() == PixelFormat::A8 ||
      texture.pixelFormat() == PixelFormat::A4) {
    src_vgbuf.image_mode = VG_LITE_MULTIPLY_IMAGE_MODE;
    src_vgbuf.transparency_mode = VG_LITE_IMAGE_TRANSPARENT;
  }

  uint32_t mix;
  if (opacity() < 0xff) {
    uint32_t alpha = opacity();
    src_vgbuf.image_mode = VG_LITE_MULTIPLY_IMAGE_MODE;
    mix = alpha << 24 | alpha << 16 | alpha << 8 | alpha;
  } else {
    mix = gx_color_to_vglite(color);
  }

#if 0
  if (texture.pixelFormat() == PixelFormat::Palette)
    vg_lite_disable_dither();
  else
    vg_lite_enable_dither();
#endif

  // TODO change to mem_dcache_clean by addr
  mem_dcache_clean_all();
  VG_LITE_RETURN_ERROR(vg_lite_blit_rect(&dst_vgbuf, &src_vgbuf, &rect, &matrix,
                                         blend, mix, VG_LITE_FILTER_POINT));
  VG_LITE_RETURN_ERROR(vg_lite_flush());
  mem_dcache_invalidate((const void *)dst_vgbuf.address,
                        dst_vgbuf.height * dst_vgbuf.stride);
  return error;
}

int VGLiteEngine::vglite_blit_transform(const Rect &dr, const Texture &texture,
                                        const Rect &sr, Color color) {
  if (dr.isEmpty() || sr.isEmpty())
    return VG_LITE_SUCCESS;

  vg_lite_buffer_t src_vgbuf = {};
  vg_lite_buffer_t dst_vgbuf = {};

  VG_LITE_JUDGE_ASSERT(vglite_buffer_init(dst_vgbuf, *device, false))

  int error = vglite_buffer_init(src_vgbuf, texture, true);
  if (error != VG_LITE_SUCCESS)
    return error;

  vg_lite_rectangle_t rect = {sr.x(), sr.y(), sr.width(), sr.height()};

  vg_lite_blend_t blend = VG_LITE_BLEND_SRC_OVER;
  if (texture.pixelFormat() >= 4) {
    blend = VG_LITE_BLEND_SRC_OVER;
    if (texture.pixelFormat() == PixelFormat::A8) {
      src_vgbuf.image_mode = VG_LITE_MULTIPLY_IMAGE_MODE;
      src_vgbuf.transparency_mode = VG_LITE_IMAGE_TRANSPARENT;
    }
  }

  uint32_t mix = 0;
  if (opacity() < 0xff) {
    uint32_t alpha = opacity();
    blend = VG_LITE_BLEND_SRC_OVER;
    src_vgbuf.image_mode = VG_LITE_MULTIPLY_IMAGE_MODE;
    mix = alpha << 24 | alpha << 16 | alpha << 8 | alpha;
  } else {
    mix = gx_color_to_vglite(color);
  }

#if 0
  if (texture.pixelFormat() == PixelFormat::Palette)
    vg_lite_disable_dither();
  else
    vg_lite_enable_dither();
#endif

  vg_lite_matrix_t matrix;
  vg_lite_identity(&matrix);
  vg_lite_translate(vg_lite_float_t(origin().x()),
                    vg_lite_float_t(origin().y()), &matrix);
  vglite::Adaptor::multiply(&matrix, *transform());
  vg_lite_translate(dr.x(), dr.y(), &matrix);

  // TODO change to mem_dcache_clean by addr
  mem_dcache_clean_all();
  VG_LITE_RETURN_ERROR(vg_lite_blit_rect(&dst_vgbuf, &src_vgbuf, &rect, &matrix,
                                         blend, mix, VG_LITE_FILTER_BI_LINEAR));
  VG_LITE_RETURN_ERROR(vg_lite_flush());
  mem_dcache_invalidate((const void *)dst_vgbuf.address,
                        dst_vgbuf.height * dst_vgbuf.stride);
  return error;
}

int VGLiteEngine::vglite_fill_path(const VectorPath &source, uint32_t color) {
  vglite::Adaptor adaptor(clip(), origin());
  vg_lite_error_t error = VG_LITE_SUCCESS;

  vg_lite_buffer_t fb = {};
  VG_LITE_JUDGE_ASSERT(vglite_buffer_init(fb, *device, false));

  vg_lite_path_t path = {};
  if (adaptor.init_path(path, source) != VG_LITE_SUCCESS) {
    GPU_LOG_W("adaptor.init_path failed.\n");
    return VG_LITE_INVALID_ARGUMENT;
  }
  vg_lite_matrix_t pathMatrix;
  vg_lite_identity(&pathMatrix);
  vg_lite_translate(static_cast<vg_lite_float_t>(origin().x()),
                    static_cast<vg_lite_float_t>(origin().y()), &pathMatrix);

  if (transform())
    vglite::Adaptor::matrix(&pathMatrix, *transform());

  // vg_lite_enable_dither();

  error = vg_lite_draw(&fb, &path, VG_LITE_FILL_NON_ZERO, &pathMatrix,
                       VG_LITE_BLEND_SRC_OVER, color);
  if (error != VG_LITE_SUCCESS) {
    vg_lite_clear_path(&path);
    GPU_LOG_W("vg_lite_draw failed.\n");
    return error;
  }
  VG_LITE_RETURN_ERROR(vg_lite_flush());
  VG_LITE_RETURN_ERROR(vg_lite_clear_path(&path));
  return error;
}

int VGLiteEngine::vglite_fill_path_pattern(const VectorPath &source,
                                           const Texture &texture,
                                           const Transform *textureTransform) {
  if (texture.isEmpty())
    return VG_LITE_SUCCESS;

  vglite::Adaptor adaptor(clip(), origin());
  vg_lite_error_t err = VG_LITE_SUCCESS;

  vg_lite_buffer_t dst_vgbuf = {}, src_vgbuf = {};

  VG_LITE_JUDGE_ASSERT(vglite_buffer_init(dst_vgbuf, *device, false))

  int error = vglite_buffer_init(src_vgbuf, texture, true);
  if (error != VG_LITE_SUCCESS)
    return error;

  vg_lite_path_t path = {};
  if (adaptor.init_path(path, source) != VG_LITE_SUCCESS)
    return VG_LITE_INVALID_ARGUMENT;

  vg_lite_matrix_t pathMatrix;
  vg_lite_matrix_t textureMatrix;
  vg_lite_identity(&pathMatrix);
  vg_lite_identity(&textureMatrix);
  vg_lite_translate(origin().x(), origin().y(), &pathMatrix);
  vg_lite_translate(origin().x(), origin().y(), &textureMatrix);
  if (transform())
    vglite::Adaptor::multiply(&pathMatrix, *transform());
  if (textureTransform)
    vglite::Adaptor::multiply(&textureMatrix, *textureTransform);

  // TODO change to mem_dcache_clean by addr
  mem_dcache_clean_all();
  VG_LITE_RETURN_ERROR(vg_lite_draw_pattern(
      &dst_vgbuf, &path, VG_LITE_FILL_NON_ZERO, &pathMatrix, &src_vgbuf,
      &textureMatrix, VG_LITE_BLEND_SRC_OVER, VG_LITE_PATTERN_COLOR,
      gx_color_to_vglite(Color()), gx_color_to_vglite(Color()),
      VG_LITE_FILTER_BI_LINEAR));
  VG_LITE_RETURN_ERROR(vg_lite_flush());
  VG_LITE_RETURN_ERROR(vg_lite_clear_path(&path));
  mem_dcache_invalidate((const void *)dst_vgbuf.address,
                        dst_vgbuf.height * dst_vgbuf.stride);
  return err;
}
