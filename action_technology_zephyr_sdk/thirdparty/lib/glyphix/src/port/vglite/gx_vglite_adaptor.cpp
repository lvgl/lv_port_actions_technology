/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#include "gx_vglite_adaptor.h"
#include "gx_transform.h"
#include "gx_vectorpath.h"
#include "gx_vglite_config.h"


#define VGLITE_FORMAT_NONE -1

using namespace vglite;
#if 0
static void pixmapClear(void *a_vptrPixels, uint32_t a_uiWidth,
                        uint32_t a_uiHeight, uint32_t a_uiPitch,
                        uint32_t a_uiFormat) {
  int format = vglite::Adaptor::fromat(a_uiFormat);
  if (a_uiPitch % GPU_VG_LITE_STRIDE_ALIGN_PX != 0)
    return;

  if (((int64_t)a_vptrPixels) %
          (int64_t)PKG_PERSIMMON_TEXTURE_PIXEL_ALLOC_ALIGN !=
      0x0U) {
    GPU_LOG_W("ptr (0x%X) not aligned to %d.", (size_t)a_vptrPixels,
              PKG_PERSIMMON_TEXTURE_PIXEL_ALLOC_ALIGN);
    return;
  }
  vg_lite_buffer_t dst_buf;
  int pixel_depth = gx::PixelFormat().depth(a_uiFormat) / 8;
  dst_buf.format = (vg_lite_buffer_format_t)format;
  dst_buf.width = a_uiPitch / pixel_depth;
  dst_buf.height = a_uiHeight;
  dst_buf.stride = a_uiPitch;
  dst_buf.image_mode = VG_LITE_NORMAL_IMAGE_MODE;
  dst_buf.transparency_mode = VG_LITE_IMAGE_OPAQUE;
  dst_buf.tiled = VG_LITE_LINEAR;
  if (memset(&dst_buf.yuv, 0, sizeof(dst_buf.yuv)) == NULL)
    return;
  dst_buf.memory = (void *)a_vptrPixels;
  // dst_buf.address = (uint32_t)dst_buf.memory;
  dst_buf.handle = NULL;
  vg_lite_map(&dst_buf, VG_LITE_MAP_USER_MEMORY, -1);

  vg_lite_rectangle_t region = {0, 0, (int)a_uiWidth, (int)a_uiHeight};
  vg_lite_clear(&dst_buf, &region, 0);
  vg_lite_finish();
  // cpu_gpu_data_cache_invalid(dst_buf.address, dst_buf.height *
  // dst_buf.stride);
  vg_lite_unmap(&dst_buf);
}
#endif
static void multiply_(vg_lite_matrix_t *matrix, vg_lite_matrix_t *mult) {
  vg_lite_matrix_t temp;
  int row, column;

  /* Process all rows. */
  for (row = 0; row < 3; row++) {
    /* Process all columns. */
    for (column = 0; column < 3; column++) {
      /* Compute matrix entry. */
      temp.m[row][column] = (matrix->m[row][0] * mult->m[0][column]) +
                            (matrix->m[row][1] * mult->m[1][column]) +
                            (matrix->m[row][2] * mult->m[2][column]);
    }
  }
  /* Copy temporary matrix into result. */
  memcpy(matrix, &temp, sizeof(temp));
}

int vglite::Adaptor::fromat(int fromat) {
  switch (fromat) {
  case gx::PixelFormat::A4:
    return VG_LITE_A4;
  case gx::PixelFormat::A8:
    return VG_LITE_A8;
  case gx::PixelFormat::RGB565:
    return VG_LITE_BGR565;
  case gx::PixelFormat::ARGB4444:
    return VG_LITE_BGRA4444;
  case gx::PixelFormat::XRGB8888:
    return VG_LITE_XRGB8888;
  case gx::PixelFormat::ARGB8888:
    return VG_LITE_BGRA8888;
  case gx::PixelFormat::RGB888:
    return VG_LITE_RGB888;
  // case gx::PixelFormat::BGR888:
  //     return VG_LITE_BGR888;
  case vglite::RGBA8888_ETC2_EAC:
    return VG_LITE_RGBA8888_ETC2_EAC;
  case gx::PixelFormat::Palette:
      return VG_LITE_INDEX_8;
  case gx::PixelFormat::Mono:
  case gx::PixelFormat::A2:
  default:
    return VGLITE_FORMAT_NONE;
  }
  return VGLITE_FORMAT_NONE;
}

Adaptor::Adaptor(const gx::Rect &_clip, const gx::Point &_origin)
    : clip(_clip), origin(_origin) {}

Adaptor::~Adaptor() {}

void Adaptor::matrix(vg_lite_matrix_t *target, const Transform &tr) {
  target->m[0][0] = tr.m11();
  target->m[1][0] = tr.m12();
  target->m[2][0] = tr.m13();
  target->m[0][1] = tr.m21();
  target->m[1][1] = tr.m22();
  target->m[2][1] = tr.m23();
  target->m[0][2] = tr.m31();
  target->m[1][2] = tr.m32();
  target->m[2][2] = tr.m33();
}

void Adaptor::multiply(vg_lite_matrix_t *target, const Transform &tr) {
  vg_lite_matrix_t m;
  matrix(&m, tr);
  multiply_(target, &m);
}

int Adaptor::init_path(vg_lite_path_t &target,
                               const gx::VectorPath &source) {
  vg_lite_error_t err = VG_LITE_SUCCESS;
  gx::Vector<gx::VectorPath::Vertex>::const_iterator it = source.path().begin();
  for (size_t i = 0; i < source.path().size(); i++) {
    switch (it[i].type) {
    case gx::VectorPath::MoveToVertex:
      paths.push_back(PathNode(VLC_OP_MOVE));
      paths.push_back(PathNode(it[i].x));
      paths.push_back(PathNode(it[i].y));
      break;
    case gx::VectorPath::LineToVertex:
      paths.push_back(PathNode(VLC_OP_LINE));
      paths.push_back(PathNode(it[i].x));
      paths.push_back(PathNode(it[i].y));
      break;
    case gx::VectorPath::CubicToVertex:
      paths.push_back(PathNode(VLC_OP_CUBIC));
      paths.push_back(PathNode(it[i].x));
      paths.push_back(PathNode(it[i].y));
      paths.push_back(PathNode(it[++i].x));
      paths.push_back(PathNode(it[i].y));
      paths.push_back(PathNode(it[++i].x));
      paths.push_back(PathNode(it[i].y));
      break;
    default:
      break;
    }
  }
  paths.push_back(PathNode(VLC_OP_END));
  gx::PointF pos = clip.topLeft() - origin;
  err = vg_lite_init_path(&target, VG_LITE_FP32, VG_LITE_MEDIUM,
                          paths.size() * 4, paths.data(),
                          (vg_lite_float_t)pos.x(), (vg_lite_float_t)pos.y(),
                          (vg_lite_float_t)pos.x() + clip.width() + 1.0f,
                          (vg_lite_float_t)pos.y() + clip.height() + 1.0f);
  if (err != VG_LITE_SUCCESS) {
    GPU_LOG_W("init_path falied.");
    return -1;
  }

  return VG_LITE_SUCCESS;
}
