/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_application.h"
#include "gx_paintengine.h"
#include "gx_pixmappaintdevice.h"
#include "gx_pixmappaintengine.h"
#include "gx_string.h"
#include "gx_touchevent.h"
#include "gx_transform.h"
#include "gx_vectorpath.h"
#include "vg_lite.h"


namespace gx {
class VGLiteEngine : public PixmapPaintEngine {
public:
  VGLiteEngine(PixmapPaintDevice *device);
  virtual ~VGLiteEngine();
  // virtual void drawPoints(const Point *points, int count, const Pen &pen);
  // virtual void drawLines(const Line *lines, int count, const Pen &pen);
  virtual void drawRects(const Rect *rects, int count, const Brush &brush);
  virtual void drawPath(const VectorPath &path, const Pen &pen);
  virtual void fillPath(const VectorPath &path, const Brush &brush);
  virtual void drawTexture(const Rect &dr, const Texture &texture,
                           const Rect &sr, const Brush &br);

private:
  int vglite_buffer_init(vg_lite_buffer_t &dst, const Texture &ptr,
                         bool source);
  int vglite_draw_rect(const Rect &rect, uint32_t color);
  int vglite_fill_rect(const Rect &rect, uint32_t color);
  int vglite_blit(const Rect &dr, const Texture &texture, const Rect &sr,
                  Color color);
  int vglite_blit_transform(const Rect &dr, const Texture &texture,
                            const Rect &sr, Color color);
  int vglite_fill_path(const VectorPath &source, uint32_t color);
  int vglite_fill_path_pattern(const VectorPath &source, const Texture &texture,
                               const Transform *textureTransform);
};
} // namespace gx
