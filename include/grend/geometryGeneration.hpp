#pragma once
#include <grend/gameModel.hpp>

namespace grendx {

typedef float (*heightFunction)(float x, float y);

gameModel::ptr generate_grid(int sx, int sy, int ex, int ey, int spacing);
gameModel::ptr generate_cuboid(float width, float height, float depth);
gameModel::ptr generateHeightmap(float width, float height, float vertPerUnit,
                                 float x, float y, heightFunction func);

gameModel::ptr generatePlane(int sx, int sy, int w, int h);

// namespace grendx
}
