#pragma once

#include <grend/renderFramebuffer.hpp>
#include <grend/renderPostStage.hpp>
#include <grend/gameObject.hpp>
#include <grend/gl_manager.hpp>
#include <grend/sdl-context.hpp>
#include <grend/glm-includes.hpp>
#include <grend/gameModel.hpp>
#include <grend/texture-atlas.hpp>
#include <grend/quadtree.hpp>
#include <grend/camera.hpp>

#include <grend/geometry-gen.hpp>

#include <list>
#include <memory>
#include <tuple>

#include <stdint.h>

static const size_t MAX_LIGHTS = 16;

namespace grendx {

class skybox {
	public:
		skybox(); 

		void draw(camera::ptr cam, unsigned width, unsigned height);
		void draw(camera::ptr cam, renderFramebuffer::ptr fb);

		gameModel::ptr model;
		Program::ptr program;
		Texture::ptr map;
};

class renderAtlases {
	public:
		renderAtlases(unsigned ref_size = 2048, unsigned shadow_size = 2048) {
			reflections = atlas::ptr(new atlas(ref_size));
			shadows     = atlas::ptr(new atlas(shadow_size, atlas::mode::Depth));
		}

		atlas::ptr reflections;
		atlas::ptr shadows;
};

struct renderFlags {
	Program::ptr mainShader;
	Program::ptr skinnedShader;

	bool cull_faces = true;
	bool sort       = true;
	bool stencil    = true;
	bool depth      = true;
	bool syncshader = true;
};

class renderQueue {
	public:
		renderQueue(camera::ptr cam) { setCamera(cam); };
		renderQueue(renderQueue& other) {
			meshes        = other.meshes;
			skinnedMeshes = other.skinnedMeshes;
			lights        = other.lights;
			probes        = other.probes;
			cam           = other.cam;
		}

		void add(gameObject::ptr obj,
		         float animTime = 0.0,
		         glm::mat4 trans = glm::mat4(1),
		         bool inverted = false);
		void addSkinned(gameObject::ptr obj,
		                gameSkin::ptr skin,
		                float animTime = 0.0,
		                glm::mat4 trans = glm::mat4(1),
		                bool inverted = false);
		void updateLights(Program::ptr program, renderAtlases& atlases);
		void updateReflections(Program::ptr program,
		                       renderAtlases& atlases,
		                       skybox& sky);
		void sort(void);
		void cull(void);
		void flush(renderFramebuffer::ptr fb,
		           renderFlags& flags,
		           renderAtlases& atlases);
		void flush(unsigned width, unsigned height,
		           renderFlags& flags,
		           renderAtlases& atlases);
		void shaderSync(Program::ptr program, renderAtlases& atlases);
		void setCamera(camera::ptr newcam) { cam = newcam; };

		camera::ptr cam = nullptr;
	
		// mat4 is calculated transform for the position of the node in the tree
		// bool is inverted flag
		std::vector<std::tuple<glm::mat4, bool, gameMesh::ptr>> meshes;
		std::map<gameSkin::ptr, std::vector<std::tuple<glm::mat4, bool, gameMesh::ptr>>> skinnedMeshes;
		std::vector<std::tuple<glm::mat4, bool, gameLight::ptr>> lights;
		std::vector<std::tuple<glm::mat4, bool, gameReflectionProbe::ptr>> probes;

	private:
		gameReflectionProbe::ptr nearest_reflection_probe(glm::vec3 pos);
		void set_reflection_probe(gameReflectionProbe::ptr probe,
		                          Program::ptr program,
		                          renderAtlases& atlases);
};

void drawShadowCubeMap(renderQueue& queue,
                       gameLightPoint::ptr light,
                       Program::ptr shadowShader,
                       renderAtlases& atlases);
void drawReflectionProbe(renderQueue& queue,
                         gameReflectionProbe::ptr probe,
                         Program::ptr reflectionShader,
                         renderAtlases& atlases,
                         skybox& sky);

class renderer {
	// TODO: shouldn't be needed here anymore
	friend class text_renderer;
	friend class game_state;

	public:
		typedef std::shared_ptr<renderer> ptr;
		typedef std::weak_ptr<renderer> weakptr;

		renderer(context& ctx);
		~renderer() { };
		void loadShaders(void);

		// TODO: 
		// see drawn_entities below
		void newframe(void);
		// look up stencil buffer index in drawn objects
		gameMesh::ptr index(unsigned idx);

		/*
		void drawQueue(renderQueue& queue);
		void drawSkybox(void);
		void drawRefprobeSkybox(glm::mat4 view, glm::mat4 proj);
		*/

		// XXX
		struct renderFlags getFlags(std::string name="main");

		// TODO: swap between these
		renderFramebuffer::ptr framebuffer;
		//renderFramebuffer last_frame;

		// (actual) screen size
		int screen_x, screen_y;

		// sky box
		skybox defaultSkybox;

		// TODO: camelCase
		/*
		std::unique_ptr<atlas> reflection_atlas;
		std::unique_ptr<atlas> shadow_atlas;
		*/
		renderAtlases atlases;

		// XXX: loaded shaders, here so they can be accessed from the editor
		std::map<std::string, Program::ptr> shaders;

	protected:
		Program::ptr shader;

		/*
		gameReflectionProbe::ptr get_nearest_refprobes(glm::vec3 pos);
		// list of models to draw
		// TODO: meshes, need to pair materials with meshes though, which
		//       currently is part of the model classes
		//       possible solution is to have a global namespace for materials,
		//       which would be easier to work with probably
		//       (also might make unintentional dependencies easier though...)
		//std::vector<gameMesh::ptr> draw_queue;
		// sorted after dqueue_sort_draws(), and flushed along with
		// dqueue_flush_draws(), this is sorted back-to-front rather than
		// front-to-back
		std::vector<gameMesh::ptr> transparent_draws;
		std::vector<gameLight::ptr> active_lights;
		std::vector<gameReflectionProbe::ptr> active_refprobes;
		*/
};

float light_extent(struct point_light *p, float threshold=0.03);
float light_extent(struct spot_light *s, float threshold=0.03);
glm::mat4 model_to_world(glm::mat4 model);

void set_material(Program::ptr program,
		// TODO: keep compiled_model reference in model
                  compiled_model::ptr obj,
                  std::string mat_name);
void set_default_material(Program::ptr program);

// namespace grendx
}
