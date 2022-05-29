#pragma once

#define OFF 0
#define ON  1

uniform sampler2D render_fb;
// TODO: texture arrays for previous frames?
//       probably want like 4-8 frames
uniform sampler2D last_frame_fb;
uniform sampler2D render_depth;

#if GREND_USE_G_BUFFER
uniform sampler2D normal_fb;
uniform sampler2D position_fb;
#endif

uniform float screen_x;
uniform float screen_y;

uniform float rend_x;
uniform float rend_y;

uniform float scale_x;
uniform float scale_y;
uniform float exposure;
uniform float time_ms;
