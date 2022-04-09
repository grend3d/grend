#pragma once
#include <grend/sceneModel.hpp>

namespace grendx {

typedef float (*heightFunction)(float x, float y);

sceneModel::ptr generate_grid(int sx, int sy, int ex, int ey, int spacing);
sceneModel::ptr generate_cuboid(float width, float height, float depth);
sceneModel::ptr generateHeightmap(float width, float height, float vertPerUnit,
                                 float x, float y, heightFunction func);

sceneModel::ptr generatePlane(int sx, int sy, int w, int h);

// namespace grendx
}
