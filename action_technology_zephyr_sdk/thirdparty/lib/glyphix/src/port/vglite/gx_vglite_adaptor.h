/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#include "gx_pixmappaintengine.h"
#include "gx_vector.h"
#include "vg_lite.h"

namespace vglite {
using namespace gx;
class Adaptor {
public:
  union PathNode {
    int op;
    float point;
    explicit PathNode(int op) : op(op) {}
    explicit PathNode(float point) : point(point) {}
  };

  Adaptor(const Rect &_clip, const Point &_origin);
  ~Adaptor();
  static int fromat(int fromat);
  static void matrix(vg_lite_matrix_t *target, const Transform &tr);
  static void multiply(vg_lite_matrix_t *target, const Transform &tr);
  int init_path(vg_lite_path_t &target, const gx::VectorPath &source);

private:
  Rect clip;
  Point origin;
  Vector<PathNode> paths;
};
enum {
  RGBA8888_ETC2_EAC = gx::PixelFormat::UserFormat,
};
} // namespace vglite
