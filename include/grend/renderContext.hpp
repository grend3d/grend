#pragma once
#include <grend/renderData.hpp>
#include <grend/renderSettings.hpp>
#include <grend/renderFramebuffer.hpp>

#include <grend/textureAtlas.hpp>

// skybox
#include <grend/camera.hpp>

namespace grendx {
// TODO: where should this be moved to?
class skybox {
	public:
		skybox(); 

		void draw(camera::ptr cam, unsigned width, unsigned height);
		void draw(camera::ptr cam, renderFramebuffer::ptr fb);

		sceneModel::ptr model;
		Program::ptr program;
		Texture::ptr map;
};

// namespace grendx
}

namespace grendx {

class renderAtlases {
	public:
		renderAtlases(unsigned refSize    = 2048,
		              unsigned shadowSize = 4096,
		              unsigned irradSize  = 1024,
		              unsigned coeffSize  = 1024)
		{
			reflections = atlas::ptr(new atlas(refSize));
			shadows     = atlas::ptr(new atlas(shadowSize, atlas::mode::Depth));
			irradiance  = atlas::ptr(new atlas(irradSize));
			irradianceCoefficients = atlas::ptr(new atlas(coeffSize));
		}

		atlas::ptr reflections;
		atlas::ptr shadows;
		atlas::ptr irradiance;
		atlas::ptr irradianceCoefficients;
};

class renderContext {
	public:
		typedef std::shared_ptr<renderContext> ptr;
		typedef std::weak_ptr<renderContext> weakptr;

		enum lightingModes {
			Clustered,
			Tiled,
			PlainArray,
		};

		renderContext(context& ctx, renderSettings _settings = renderSettings());
		~renderContext() { };

		void applySettings(renderSettings& settings);
		void loadShaders(void);

		// TODO: 
		// see drawn_entities below
		void newframe(void);
		// look up stencil buffer index in drawn objects
		sceneMesh::ptr index(unsigned idx);
		void setArrayMode(enum lightingModes mode);

		// XXX
		struct renderFlags getLightingFlags(std::string name="");
		void setReflectionProbe(sceneReflectionProbe::ptr probe,
		                        Program::ptr program);
		void setIrradianceProbe(sceneIrradianceProbe::ptr probe,
		                        Program::ptr program);

		// TODO: would be ideal to have "low", "medium", "high" or something
		//       categories, so lighting settings could be easily mapped to
		//       graphics settings
		void setDefaultLightModel(std::string name);

		// TODO: swap between these
		renderFramebuffer::ptr framebuffer;
		//renderFramebuffer last_frame;

		// (actual) screen size
		int screen_x, screen_y;

		// sky box
		skybox defaultSkybox;

		// global rendering state
		renderAtlases atlases;
		Shader::parameters globalShaderOptions;
		enum lightingModes lightingMode = lightingModes::Tiled;

		// maps 
		std::string defaultLighting = "main";
		std::map<std::string, renderFlags> lightingShaders;
		std::map<std::string, renderFlags> probeShaders;
		std::map<std::string, Program::ptr> postShaders;
		std::map<std::string, Program::ptr> internalShaders;

		Buffer::ptr lightBuffer;
		Buffer::ptr pointTiles;
		Buffer::ptr spotTiles;

		lights_std140      lightBufferCtx;
		light_tiles_std140 pointTilesCtx;
		light_tiles_std140 spotTilesCtx;

		float lightThreshold = 0.05;
		float exposure       = 1.f;
		renderSettings settings;

	protected:
		Program::ptr shader;

		/*
		sceneReflectionProbe::ptr get_nearest_refprobes(glm::vec3 pos);
		// list of models to draw
		// TODO: meshes, need to pair materials with meshes though, which
		//       currently is part of the model classes
		//       possible solution is to have a global namespace for materials,
		//       which would be easier to work with probably
		//       (also might make unintentional dependencies easier though...)
		//std::vector<sceneMesh::ptr> draw_queue;
		// sorted after dqueue_sort_draws(), and flushed along with
		// dqueue_flush_draws(), this is sorted back-to-front rather than
		// front-to-back
		std::vector<sceneMesh::ptr> transparent_draws;
		std::vector<sceneLight::ptr> active_lights;
		std::vector<sceneReflectionProbe::ptr> active_refprobes;
		*/
};

// namespace grendx
}
