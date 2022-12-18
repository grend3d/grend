#include <grend/audioMixer.hpp>
#include <grend/sdlContext.hpp>
#include <grend/utility.hpp>
#include <grend/logger.hpp>
#include <stdint.h>

using namespace grendx;

// non-pure virtual destructors for rtti
audioChannel::~audioChannel() {};
stereoAudioChannel::~stereoAudioChannel() {};
spatialAudioChannel::~spatialAudioChannel() {};

void audioChannel::restart(void) {
	audioPosition = 0;
	playState = state::Playing;
}

stereoAudioChannel::stereoAudioChannel(channelBuffers_ptr channels,
                                       enum audioChannel::mode m)
	: audioChannel(m)
{
	assert(channels != nullptr && channels->size() > 0);

	// use given mono channel for both stereo channels
	if (channels->size() == 1) {
		auto& p = (*channels)[0];
		bufs = {p, p};

	} else {
		bufs = {(*channels)[0], (*channels)[1]};
	}
}

std::pair<int16_t, int16_t>
stereoAudioChannel::getSample(camera::ptr cam) {
	if (audioPosition >= bufs.first->size()
	    || audioPosition >= bufs.second->size())
	{
		if (loopMode == mode::Loop) {
			restart();

		} else {
			playState = state::Ended;
			return {0, 0};
		}
	}

	size_t p = audioPosition++;
	return {0.8*(*bufs.first)[p], 0.8*(*bufs.second)[p]};
}

static void mixCallback(void *userdata, uint8_t *stream, int len) {
	audioMixer *mix = reinterpret_cast<audioMixer*>(userdata);
	assert(mix != nullptr);

	if (mix->currentCam == nullptr) {
		puts("nullptr!");
		memset(stream, 0, len);
		return;
	}

	int16_t *s16strm = reinterpret_cast<int16_t*>(stream);

	for (int i = 0; i < len/2; i += 2) {
		// TODO: interpolate over velocity
		auto sample = mix->getSample(mix->currentCam);
		s16strm[i]   = sample.first;
		s16strm[i+1] = sample.second;
	}
}

audioMixer::audioMixer(SDLContext *ctx) {
	ctx->setAudioCallback(this, mixCallback);
}

void audioMixer::setCamera(camera::ptr cam) {
	currentCam = cam;
}

std::pair<int16_t, int16_t>
audioMixer::getSample(camera::ptr cam) {
	std::pair<int32_t, int32_t> accum;
	std::lock_guard<std::mutex> lock(mtx);

	for (auto it = channels.begin(); it != channels.end();) {
		auto& [id, chan] = *it;
		auto sample = chan->getSample(cam);

		accum.first  += sample.first;
		accum.second += sample.second;

		if (chan->playState == audioChannel::state::Ended) {
			channels.erase(it++);

		} else {
			it++;
		}
	}

	// TODO: limiter in case of clipping
	return {accum.first, accum.second};
}

size_t audioMixer::add(audioChannel::ptr channel) {
	std::lock_guard<std::mutex> lock(mtx);

	size_t ret = chanids++;
	channels[ret] = channel;
	return ret;
}

void audioMixer::remove(size_t id) {
	auto it = channels.find(id); if (it != channels.end()) {
		channels.erase(it);
	}
}

#include <stb/stb_vorbis.h>
static std::map<std::string, channelBuffers_weakptr> filecache;

channelBuffers_ptr grendx::openAudio(std::string filename) {
	auto it = filecache.find(filename);

	if (it != filecache.end()) {
		auto& [name, ptr] = *it;
		if (auto sptr = ptr.lock()) {
			return sptr;
		}
	}

	int channels, rate;
	int16_t *ibuf;
	int len = stb_vorbis_decode_filename(filename.c_str(), &channels, &rate, &ibuf);

	LogFmt("{}: loading audio: {}, {}, {}\n", filename.c_str(), len, channels, rate);

	if (len > 0 && channels > 0) {
		auto ret = std::make_shared<channelBuffers>();

		for (int c = 0; c < channels; c++) {
			ret->push_back(std::make_shared<audioBuffer>());
		}

		for (int i = 0; i < len; i++) {
			for (int c = 0; c < channels; c++) {
				(*ret)[c]->push_back(ibuf[channels*i+c]);
			}
		}

		filecache[filename] = ret;
		return ret;
	}

	return nullptr;
}

// convenience functions, wrap some of the verbose make_shared...()
spatialAudioChannel::ptr grendx::openSpatialLoop(std::string filename) {
	auto ptr = openAudio(filename);
	return std::make_shared<spatialAudioChannel>(ptr, audioChannel::mode::Loop);
}

spatialAudioChannel::ptr grendx::openSpatialChannel(std::string filename) {
	auto ptr = openAudio(filename);
	return std::make_shared<spatialAudioChannel>
		(ptr, audioChannel::mode::OneShot);
}

stereoAudioChannel::ptr grendx::openStereoLoop(std::string filename) {
	auto ptr = openAudio(filename);
	return std::make_shared<stereoAudioChannel>(ptr, audioChannel::mode::Loop);
}

stereoAudioChannel::ptr grendx::openStereoChannel(std::string filename) {
	auto ptr = openAudio(filename);
	return std::make_shared<stereoAudioChannel>
		(ptr, audioChannel::mode::OneShot);
}
