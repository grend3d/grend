#include <grend/audioMixer.hpp>
#include <grend/sdlContext.hpp>
#include <grend/utility.hpp>
#include <stdint.h>

using namespace grendx;

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <math.h>
#include <fcntl.h>

#define BUFSIZE (1 << 16)
//#define SAMPLE_RATE 96000
//#define SAMPLE_RATE 48000
#define SAMPLE_RATE 44100

typedef struct sample_c2 {
	int16_t left;
	int16_t right;
} sample_c2;

static inline
int16_t cbuf_rindex(audioBuffer& buf, size_t pos, size_t n) {
	// ptr points to one past the last sample, so -1 to make it so rindex(0)
	// will get the last sample
	return buf[(pos - n - 1) % buf.size()];
}

static inline
double cbuf_rindex_interpolate(audioBuffer& buf, size_t pos, double n) {
	int16_t a = cbuf_rindex(buf, pos, ceil(n));
	int16_t b = cbuf_rindex(buf, pos, floor(n));
	double mix = n - floor(n);

	//mix = 1 - mix*mix;
	mix = 1 - mix;
	return a*mix + b*(1 - mix);
}

static inline
int16_t cbuf_arindex(audioBuffer& buf, size_t pos, size_t n) {
	return buf[(pos - n) % buf.size()];
}

static inline
int16_t cbuf_aindex(audioBuffer& buf, size_t pos, size_t n) {
	return buf[(pos + n) % buf.size()];
}

static inline
int16_t sma(audioBuffer& buf, size_t pos, int32_t level) {
	int32_t sum = 0;

	for (int32_t i = 0; i < level; i++) {
		sum += cbuf_arindex(buf, pos, i);
	}

	return sum / max(1, level); 
}

static inline
int16_t sma_rindex(audioBuffer& buf, size_t pos, int32_t level, size_t idx) {
	int32_t sum = 0;

	for (int32_t i = 0; i < level; i++) {
		sum += cbuf_arindex(buf, pos, i + idx);
	}

	return sum / max(1, level); 
}


static inline
int16_t sma_interpolate(audioBuffer& buf, size_t pos, double amount) {
	double a = sma(buf, pos, ceil(amount));
	double b = sma(buf, pos, floor(amount));
	double mix = amount - floor(amount);

	return ((a * (1 - mix)) + (b * mix));
}

static inline
int16_t sma_rindex_interpolate(audioBuffer& buf, size_t pos,
                               double amount, size_t idx)
{
	double a = sma_rindex(buf, pos, ceil(amount), idx);
	double b = sma_rindex(buf, pos, floor(amount), idx);
	double mix = amount - floor(amount);

	return ((a * (1 - mix)) + (b * mix));
}

static const double sound_speed = 34027;
static const double inch_to_cm  = 2.54;
#define INCHES 2.54

static const double direction = -3.1415926;
static const double driver_radius = 3.0*INCHES;
static const double front_radius = 6.0*INCHES;
static const double back_radius = 3.75*INCHES;
static const double echo_ext_param = (M_PI/2.0);
static const double echo_int_param = 340.27/1482.0 * ((M_SQRT2/2)/(M_PI/4));
//static const double echo_int_param = 340.27/1482.0;
static const double echo_strength = 0.15;
static const double phase_cof = 8;
static const double distance_falloff_factor = 8.0;

static inline double sound_dist(double cm) {
	return cm / sound_speed;
}

static inline
double dist_quad(double x) {
	double m = x;
	return driver_radius +
		fabs(m) * ((x <= 0)
			? fabs(x)*(back_radius - driver_radius)
			: fabs(x)*(front_radius - driver_radius));
}

sample_c2 sample_audio_stream(audioBuffer& buf, size_t pos, glm::vec2 rot) {
	//double ticks = samples / (double)SAMPLE_RATE / direction;
	//double it = sin(ticks);
	//double foo = cos(ticks);
	glm::vec2 temp = glm::normalize(rot);
	double it = temp.y;
	double foo = temp.x;

	double thing =
		(sound_dist(driver_radius)
		 + sound_dist((front_radius - driver_radius))*fabs(foo)*(foo>0)
		 + sound_dist((back_radius - driver_radius))*fabs(foo)*(foo<=0));

	double frig =
		(sound_dist(driver_radius * echo_int_param)
		 + sound_dist((front_radius - driver_radius) * echo_int_param)*fabs(foo)*(foo<=0)
		 + sound_dist((back_radius - driver_radius) * echo_int_param)*fabs(foo)*(foo>0));

	double ext_echo =
		(sound_dist(driver_radius * echo_ext_param)
		 + sound_dist((front_radius - driver_radius) * echo_ext_param)*fabs(foo)*(foo<=0)
		 + sound_dist((back_radius - driver_radius) * echo_ext_param)*fabs(foo)*(foo>0));
	
	double offset = thing * SAMPLE_RATE * it;
	double foobar = frig * SAMPLE_RATE * it;
	double ext_offset = ext_echo * SAMPLE_RATE * it;
	double diff = foobar - offset;
	double ext_diff = ext_offset - offset;
	double distance = dist_quad(foo);
	double center = BUFSIZE / 2 + phase_cof*distance /* phase shift */;
	double rad = distance_falloff_factor/(4*M_PI*distance*distance);
	//double sqrad = sqrt(rad);
	double sqrad = rad;
	double vol = (0.90 + sqrad);

	//double dir = (it > 0.0)? 1.0 : -1.0;
	double dir = it;
	//double dir = (it*it*it);
	double lvol = vol + 0.5*sqrad*dir;
	double rvol = vol - 0.5*sqrad*dir;

	double foo_lvol = vol + 0.5*sqrad*dir;
	double foo_rvol = vol - 0.5*sqrad*dir;

	double pre_left  = lvol*sma_rindex_interpolate(buf, pos, max(1.0, 1.01 - rad*dir), center - offset);
	double pre_right = rvol*sma_rindex_interpolate(buf, pos, max(1.0, 1.01 + rad*dir), center + offset);

	double foo_left  = foo_lvol*sma_rindex_interpolate(buf, pos, max(1.0, 1.15 - rad*dir), center - foobar + diff);
	double foo_right = foo_rvol*sma_rindex_interpolate(buf, pos, max(1.0, 1.15 + rad*dir), center + foobar - diff);

	double ext_left  = foo_lvol*sma_rindex_interpolate(buf, pos, max(1.0, 1.5 - rad*dir), center - ext_offset + ext_diff);
	double ext_right = foo_rvol*sma_rindex_interpolate(buf, pos, max(1.0, 1.5 + rad*dir), center + ext_offset - ext_diff);

	int16_t f_left  = (pre_left*(1.0 - echo_strength)
					   + foo_left*echo_strength*0.50
					   + ext_left*echo_strength*0.50);
	int16_t f_right = (pre_right*(1.0 - echo_strength)
					   + foo_right*echo_strength*0.50
					   + ext_right*echo_strength*0.50);

	int16_t left = f_left;
	int16_t right = f_right;

	return (sample_c2) {
		.left = left,
		.right = right
	};
}

spatialAudioChannel::spatialAudioChannel(channelBuffers_ptr channels,
                                         enum audioChannel::mode m)
	: audioChannel(m)
{
	assert(channels != nullptr && channels->size() > 0);
	buf = (*channels)[0];
}

std::pair<int16_t, int16_t>
spatialAudioChannel::getSample(camera::ptr cam) {
	if (audioPosition >= buf->size()) {
		if (loopMode == mode::Loop) {
			restart();

		} else {
			playState = state::Ended;
			return {0, 0};
		}
	}

	audioPosition++;
	float r = glm::distance(cam->position(), worldPosition);

	// TODO: fine-tuned attenuation (can just do constant/linear/quad), volume
	//float atten = min(1.0, (100.0 / (4*3.1415926*r*r)));
	float atten = min(0.8f, 2.f / (r));
	glm::vec3 diff = worldPosition - cam->position();
	glm::vec2 dir = {glm::dot(cam->direction(), diff), glm::dot(cam->right(), diff)};

	sample_c2 samp = sample_audio_stream(*buf, audioPosition, dir);
	return {samp.left * atten, samp.right * atten};
}
