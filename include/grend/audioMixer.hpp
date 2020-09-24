#pragma once
#include <grend/sdl-context.hpp>
#include <grend/glm-includes.hpp>
#include <grend/camera.hpp>
#include <memory>
#include <utility>
#include <map>
#include <vector>
#include <string>

namespace grendx {

class audioBuffer : public std::vector<int16_t> {
	public:
		typedef std::shared_ptr<audioBuffer> ptr;
		typedef std::weak_ptr<audioBuffer> weakptr;
};

typedef audioBuffer::ptr monoBuffer;
typedef std::pair<audioBuffer::ptr, audioBuffer::ptr> stereoBuffer;

typedef std::vector<audioBuffer::ptr>   channelBuffers;
typedef std::shared_ptr<channelBuffers> channelBuffers_ptr;
typedef std::weak_ptr<channelBuffers>   channelBuffers_weakptr;

class audioChannel {
	public:
		enum mode {
			OneShot,
			Loop,
		};

		enum state {
			Playing,
			Paused,
			Ended,
		};

		typedef std::shared_ptr<audioChannel> ptr;
		typedef std::weak_ptr<audioChannel> weakptr;
		audioChannel(enum mode m) : loopMode(m) {};

		virtual std::pair<int16_t, int16_t> getSample(camera::ptr cam) = 0;
		virtual void restart(void);

		enum mode loopMode;
		enum state playState = state::Playing;

		size_t audioPosition = 0;
		glm::vec3 worldPosition = glm::vec3(0);
};

class stereoAudioChannel : public audioChannel {
	public:
		typedef std::shared_ptr<stereoAudioChannel> ptr;
		typedef std::weak_ptr<stereoAudioChannel> weakptr;

		stereoAudioChannel(channelBuffers_ptr channels,
		                   enum audioChannel::mode m = mode::OneShot);

		virtual std::pair<int16_t, int16_t> getSample(camera::ptr cam);

		stereoBuffer bufs;
};

class spatialAudioChannel : public audioChannel {
	public:
		typedef std::shared_ptr<spatialAudioChannel> ptr;
		typedef std::weak_ptr<spatialAudioChannel> weakptr;

		spatialAudioChannel(channelBuffers_ptr buffer,
		                    enum mode m = mode::OneShot);

		virtual std::pair<int16_t, int16_t> getSample(camera::ptr cam);

		// mono input
		monoBuffer buf;
};

class audioMixer {
	public:
		typedef std::shared_ptr<audioMixer> ptr;
		typedef std::weak_ptr<audioMixer> weakptr;

		audioMixer(context& ctx);

		void setCamera(camera::ptr cam);
		std::pair<int16_t, int16_t> getSample(camera::ptr cam);
		size_t add(audioChannel::ptr channel);
		void   remove(size_t id);

		std::map<size_t, audioChannel::ptr> channels;
		size_t chanids = 0;
		camera::ptr currentCam;
		glm::vec3 currentPos;
};

channelBuffers_ptr openAudio(std::string filename);

// convenience functions, wrap some of the verbose make_shared...()
spatialAudioChannel::ptr openSpatialLoop(std::string filename);
spatialAudioChannel::ptr openSpatialChannel(std::string filename);
stereoAudioChannel::ptr  openStereoLoop(std::string filename);
stereoAudioChannel::ptr  openStereoChannel(std::string filename);

// namespace grendx
}
