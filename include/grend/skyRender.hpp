#include <grend/renderFramebuffer.hpp>
#include <grend/camera.hpp>
#include <grend/sceneModel.hpp>
#include <grend/glManager.hpp>

#include <string>

namespace grendx {

class skyRender {
	public:
		virtual ~skyRender();

		virtual void draw(camera::ptr cam, unsigned width, unsigned height) {};

		virtual void draw(camera::ptr cam, renderFramebuffer::ptr fb) {
			draw(cam, fb->width, fb->height);
		}
};

class skyRenderCube : public skyRender {
	public:
		skyRenderCube(const std::string& cubepath  = GR_PREFIX "assets/tex/cubes/default/",
		              const std::string& extension = ".png");
		virtual ~skyRenderCube();

		void draw(camera::ptr cam, unsigned width, unsigned height);

		sceneModel::ptr model;
		Program::ptr program;
		Texture::ptr map;
};

class skyRenderHDRI : public skyRender {
	public:
		skyRenderHDRI(const std::string& hdrPath);
		virtual ~skyRenderHDRI();

		void draw(camera::ptr cam, unsigned width, unsigned height);

		sceneModel::ptr model;
		Program::ptr program;
		Texture::ptr map;
};

// namespace grendx
}
