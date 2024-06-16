#include <grend/thumbnails.hpp>
#include <grend/gameMain.hpp>
#include <grend/jobQueue.hpp>
#include <grend/logger.hpp>
#include <grend/renderQueue.hpp>
#include <grend/compiledModel.hpp>
#include <grend/loadScene.hpp>

#include <grend/utility.hpp>

#include <string>
#include <filesystem>

using namespace grendx;
using namespace grendx::engine;

namespace fs = std::filesystem;

const unsigned THUMBNAIL_SIZE = 128;

std::optional<Texture::ptr>
thumbnails::getThumbnail(std::string filePath) {
	auto it = thumbnailRenders.find(filePath);

	if (it != thumbnailRenders.end()) {
		return it->second;

	} else {
		auto& task = this->getThumbnailTask(filePath);

		if (jobFinished(task)) {
			return nullptr;
		}
	}

	return std::nullopt;
}

static bool isRenderableExt(const std::string& ext) {
	return ext == ".gltf"
		|| ext == ".glb"
	    || ext == ".obj";
	// TODO: maybe scenes
}

static bool isImageExt(const std::string& ext) {
	return ext == ".jpg"
		|| ext == ".jpeg"
		|| ext == ".png"
		|| ext == ".hdr"
		|| ext == ".tga"
		|| ext == ".gif"
		|| ext == ".ppm"
		|| ext == ".bmp"
		;
}

std::future<bool>& thumbnails::getThumbnailTask(std::string filePath) {
	std::lock_guard g(mtx);
	auto it = thumbnailTasks.find(filePath);

	if (it != thumbnailTasks.end()) {
		return it->second;

	} else {
		auto jobs = Resolve<jobQueue>();
		std::string strPath(filePath);
		const std::string ext = filename_extension(filePath);

		if (isRenderableExt(ext)) {
			thumbnailTasks[strPath] = jobs->/*addAsync*/addDeferred([=, this] () {
				this->doThumbnailRender(filePath);
				return true;
			});

		} else if (isImageExt(ext)) {
			thumbnailTasks[strPath] = jobs->/*addAsync*/addDeferred([=, this] () {
				auto data = std::make_shared<textureData>(filePath, true);
				Texture::ptr tex = texcache(data);

				{
					std::lock_guard g(mtx);
					//this->thumbnailRenders[filePath] = nullptr;
					this->thumbnailRenders[filePath] = tex;
				}

				return true;
			});

		} else {
			thumbnailTasks[strPath] = jobs->/*addAsync*/addDeferred([=, this] () {
				Texture::ptr tex;

				if (fs::is_directory(filePath)) {
					auto data = std::make_shared<textureData>(GR_PREFIX "assets/tex/ui/folder.png", true);
					tex = texcache(data);

				} else {
					auto data = std::make_shared<textureData>(GR_PREFIX "assets/tex/ui/document.png", true);
					tex = texcache(data);
				}

				{
					std::lock_guard g(mtx);
					//this->thumbnailRenders[filePath] = nullptr;
					this->thumbnailRenders[filePath] = tex;
				}

				return true;
			});
		}

		return thumbnailTasks[strPath];
	}
}

void thumbnails::doThumbnailRender(std::string filePath) {
	auto jobs = Resolve<jobQueue>();
	auto rend = Resolve<renderContext>();
	auto ecs  = Resolve<ecs::entityManager>();

	if (auto res = loadSceneData(filePath)) {
		auto [obj, models] = *res;

		/*jobs->addDeferred([=, this] ()*/ {
			compileModels(models);

			auto origSize = rend->getDrawSize();
			auto origOffset = rend->getDrawOffset();

			rend->setViewport({0, 0}, {THUMBNAIL_SIZE, THUMBNAIL_SIZE});

			camera::ptr cam = std::make_shared<camera>();
			sceneNode::ptr root = ecs->construct<sceneNode>();
			renderFramebuffer::ptr fb = std::make_shared<renderFramebuffer>(THUMBNAIL_SIZE, THUMBNAIL_SIZE);
			sceneLightPoint::ptr light = ecs->construct<sceneLightPoint>();
			renderFlags flags = rend->getLightingFlags("unshaded");
			renderQueue que;
			renderOptions options;

			setNode("light", root, light);
			setNode("model", root, obj);

			light->transform.setPosition({0, -1, 2});
			light->intensity = 100;
			cam->setViewport(THUMBNAIL_SIZE, THUMBNAIL_SIZE);
			cam->setPosition({0, 1.4, -1.4});
			cam->setDirection({0, -1, 1}, {0, 1, 0});
			que.add(root);

			fb->clear();
			flush(que, cam, fb, rend, flags, options);
			//rend->defaultSkybox->draw(cam, fb);

			rend->setViewport(origOffset, origSize);

			{
				std::lock_guard g(mtx);
				//this->thumbnailRenders[filePath] = nullptr;
				this->thumbnailRenders[filePath] = fb->color;
			}

			//return true;
		}/*)*/;

	} else {
		LogFmt("ERROR LOADING! {}", filePath);
	}
}
