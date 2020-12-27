#pragma once

layout (std140) uniform instanceTransforms {
	mat4 transforms[256];
};
