#pragma once
#include <grend/gameModel.hpp>

namespace grendx {

gameModel::ptr generate_grid(int sx, int sy, int ex, int ey, int spacing);
gameModel::ptr generate_cuboid(float width, float height, float depth);

// namespace grendx
}
