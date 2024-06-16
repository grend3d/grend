#pragma once

#include <grend/IoC.hpp>
#include <grend/renderFramebuffer.hpp>

#include <future>
#include <map>
#include <mutex>
#include <optional>

namespace grendx {

class thumbnails : public IoC::Service {
	public:
		std::optional<Texture::ptr> getThumbnail(std::string filePath);

	private:
		std::future<bool>& getThumbnailTask(std::string filePath);
		void doThumbnailRender(std::string filePath);

		std::map<std::string, Texture::ptr> thumbnailRenders;
		std::map<std::string, std::future<bool>> thumbnailTasks;
		std::mutex mtx;
};

// namespace grendx
}
