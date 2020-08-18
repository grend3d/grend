#pragma once

#include <grend/gameObject.hpp>
#include <grend/gl_manager.hpp>
#include <grend/sdl-context.hpp>
#include <grend/glm-includes.hpp>
#include <grend/gameModel.hpp>
#include <grend/texture-atlas.hpp>
#include <grend/quadtree.hpp>
#include <grend/camera.hpp>

#include <list>
#include <memory>

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

class renderQueue {
	public:
		renderQueue(camera::ptr cam) { setCamera(cam); };
		renderQueue(renderQueue& other) {
			meshes = other.meshes;
			lights = other.lights;
			probes = other.probes;
			cam    = other.cam;
		}

		void add(gameObject::ptr obj);
		void sort(void);
		void cull(void);
		void flush(Program::ptr program);
		void shaderSync(Program::ptr program);
		void setCamera(camera::ptr newcam) { cam = newcam; };

		camera::ptr cam = nullptr;
	
		// mat4 is calculated transform for the position of the node in the tree
		std::vector<std::pair<glm::mat4, gameMesh::ptr>> meshes;
		std::vector<std::pair<glm::mat4, gameLight::ptr>> lights;
		std::vector<std::pair<glm::mat4, gameReflectionProbe::ptr>> probes;
};

class renderer {
	friend class text_renderer;
	friend class game_state;

	public:
		renderer(context& ctx);
		~renderer() { };
		void loadShaders(void);
		void initFramebuffers(void);

		// TODO: 
		// see drawn_entities below
		void newframe(void);
		// look up stencil buffer index in drawn objects
		gameMesh::ptr index(unsigned idx);

		/*
		void draw_mesh(std::string mesh, const struct draw_attributes *attr);
		void draw_mesh_lines(std::string mesh, const struct draw_attributes *attr);
		void draw_model(const struct draw_attributes *attr);
		void draw_model(struct draw_attributes attr);
		void draw_model_lines(const struct draw_attributes *attr);
		void draw_model_lines(struct draw_attributes attr);
		void draw_screenquad(void);

		void dqueue_draw_mesh(std::string mesh, const struct draw_attributes *attr);
		void dqueue_draw_model(const struct draw_attributes *attr);
		void dqueue_draw_model(struct draw_attributes attr);
		void dqueue_sort_draws(glm::vec3 camera);
		void dqueue_cull_models(glm::vec3 camera);
		void dqueue_flush_draws(void);
		*/

		void draw_screenquad(void);

		void updateObjects(gameObject::ptr root);
		void draw(gameMesh::ptr mesh);
		void draw(gameModel::ptr mesh);
		void drawLines(gameMesh::ptr mesh);
		void drawLines(gameModel::ptr mesh);
		void cullDraws(void);
		void sortDraws(void);
		void flushDraws(void);

		void drawSkybox(void);
		void drawRefprobeSkybox(glm::mat4 view, glm::mat4 proj);
		void drawShadowCubeMap(gameLightPoint::ptr light);
		void drawShadowMap(gameLightSpot::ptr light);
		void drawShadowMap(gameLightDirectional::ptr light);
		void drawReflectionProbe(gameReflectionProbe::ptr probe);

		void set_material(gl_manager::compiled_model& obj, std::string mat_name);
		void set_default_material(std::string mat_name);

		// init_lights() will need to be called after the shader is bound
		// (in whatever subclasses the engine)
		//void init_lights(void);
		void compile_lights(void);
		void update_lights(void);
		void syncLights(void);

		/* * TODO: touch maps when i
		void touch_shadowmaps(void);
		void touch_refprobes(void);
		*/

		uint32_t add_reflection_probe(struct reflection_probe ref);
		void free_reflection_probe(uint32_t id);

		void set_shader(Program::ptr shd);
		void set_mvp(glm::mat4 mod, glm::mat4 view, glm::mat4 projection);
		void set_m(glm::mat4 mod);
		void set_reflection_probe(glm::vec3 position);
		gl_manager& get_glman(void){ return glman; };

		// main rendering framebuffer
		Framebuffer::ptr rend_fb;
		Texture::ptr     rend_tex;
		Texture::ptr     rend_depth;
		int rend_x, rend_y; // framebuffer dimensions

		// (actual) screen size
		int screen_x, screen_y;

		// previous frame info
		Uint32 last_frame;
		Framebuffer::ptr last_frame_fb; // same dimensions as rend_fb
		Texture::ptr     last_frame_tex; // same dimensions as rend_fb

		// sky box
		Texture::ptr skybox;

		std::unique_ptr<atlas> reflection_atlas;
		std::unique_ptr<atlas> shadow_atlas;

		// XXX: loaded shaders, here so they can be accessed from the editor
		std::map<std::string, Program::ptr> shaders;

	protected:
		gameReflectionProbe::ptr get_nearest_refprobes(glm::vec3 pos);
		Program::ptr shader;
		gl_manager glman;

		// list of models to draw
		// TODO: meshes, need to pair materials with meshes though, which
		//       currently is part of the model classes
		//       possible solution is to have a global namespace for materials,
		//       which would be easier to work with probably
		//       (also might make unintentional dependencies easier though...)
		std::vector<gameMesh::ptr> draw_queue;
		// sorted after dqueue_sort_draws(), and flushed along with
		// dqueue_flush_draws(), this is sorted back-to-front rather than
		// front-to-back
		std::vector<gameMesh::ptr> transparent_draws;
		std::vector<gameLight::ptr> active_lights;
		std::vector<gameReflectionProbe::ptr> active_refprobes;

		// lookup indexes for meshes drawn to the framebuffer
		unsigned drawn_counter = 0;
		gameMesh::ptr drawn_entities[256];

		std::string fallback_material = "(null)";

		// handles for default material textures
		std::map<std::string, Texture::ptr> diffuse_handles;
		std::map<std::string, Texture::ptr> specular_handles;
		std::map<std::string, Texture::ptr> normmap_handles;
		std::map<std::string, Texture::ptr> aomap_handles;
};

float light_extent(struct point_light *p, float threshold=0.03);
float light_extent(struct spot_light *s, float threshold=0.03);

// namespace grendx
}
