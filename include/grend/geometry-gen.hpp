#pragma once
#include <grend/model.hpp>

namespace grendx {

model generate_grid(int sx, int sy, int ex, int ey, int spacing);
model generate_cuboid(float width, float height, float depth);

// namespace grendx
}
