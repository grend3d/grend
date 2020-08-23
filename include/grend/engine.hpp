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

	/*
struct point_light {
	glm::vec3 position;
	glm::vec4 diffuse;
	float radius;
	float intensity;
	bool casts_shadows = false;
	quadtree::node_id shadowmap[6];

	// meta info not passed to the shader
	bool changed = true;
	bool static_shadows = false;
	bool shadows_rendered = false;
};

struct spot_light {
	glm::vec3 position;
	glm::vec4 diffuse;
	glm::vec3 direction;
	float radius; // bulb radius
	float intensity;
	float angle;
	bool casts_shadows = false;
	quadtree::node_id shadowmap;

	// meta
	bool changed = true;
	bool static_shadows = false;
	bool shadows_rendered = false;
};

struct directional_light {
	glm::vec3 position;
	glm::vec4 diffuse;
	glm::vec3 direction;
	float intensity;
	bool casts_shadows = false;
	quadtree::node_id shadowmap;

	// meta
	bool changed = true;
	bool static_shadows = false;
	bool shadows_rendered = false;
};
*/

// TODO: revisit parabolic maps at some point, but for now they're
//       not ideal:
//       - transformation of vertices means that depth information is lost,
//         so rendering is less efficient and depth maps can't be generated
//         the simple way
//       - storage is not actually much lower, similar quality parabolic
//         maps are the same or larger than cubemaps, lots of wasted space
//         in the rendered images (roughly a circle of usable data per side,
//         which means a ratio of around PI/4 of the space is actually used)
//       - with MRTs rendering the faces is a non-issue, the thing can be
//         rendered in one shot, real-time reflections can just be disabled
//         on gles2 (and the platforms that gles2 would be used for probably
//         can't handle rendering real-time reflections to begin with anyway)
/*
struct reflection_probe {
	glm::vec3 position;
	quadtree::node_id faces[6];
	bool changed = true;
	bool is_static = true;
	bool have_map = false;
	// TODO: maybe non-static with update frequency
};

// indicators for objects (to help identify objects from the stencil index
// after drawing all the meshes)
enum drawattr_class {
	DRAWATTR_CLASS_NONE,
	DRAWATTR_CLASS_MAP,
	DRAWATTR_CLASS_PHYSICS,
	DRAWATTR_CLASS_UI,
	DRAWATTR_CLASS_UI_LIGHT,
	DRAWATTR_CLASS_UI_REFPROBE,
};

struct draw_class {
	unsigned class_id = 0;
	unsigned obj_id = 0;
};
*/

/*
struct draw_attributes {
	std::string name;
	glm::mat4 transform;
	GLenum face_order = GL_CCW;
	bool cull_faces = true;
};
*/

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

class renderQueue {
	public:
		renderQueue(camera::ptr cam) { setCamera(cam); };
		renderQueue(renderQueue& other) {
			meshes = other.meshes;
			lights = other.lights;
			probes = other.probes;
			cam    = other.cam;
		}

		void add(gameObject::ptr obj,
		         glm::mat4 trans = glm::mat4(1),
		         bool inverted = false);
		void updateLights(Program::ptr program, renderAtlases& atlases);
		void updateReflections(Program::ptr program,
		                       renderAtlases& atlases,
		                       skybox& sky);
		void sort(void);
		void cull(void);
		void flush(renderFramebuffer::ptr fb, Program::ptr program,
		           renderAtlases& atlases);
		void flush(unsigned width, unsigned height,
		           Program::ptr program, renderAtlases& atlases);
		void shaderSync(Program::ptr program, renderAtlases& atlases);
		void setCamera(camera::ptr newcam) { cam = newcam; };

		camera::ptr cam = nullptr;
	
		// mat4 is calculated transform for the position of the node in the tree
		// bool is inverted flag
		std::vector<std::tuple<glm::mat4, bool, gameMesh::ptr>> meshes;
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

		void drawQueue(renderQueue& queue);

		void drawSkybox(void);
		void drawRefprobeSkybox(glm::mat4 view, glm::mat4 proj);

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
		gameReflectionProbe::ptr get_nearest_refprobes(glm::vec3 pos);
		Program::ptr shader;

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
