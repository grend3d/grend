#pragma once

#include <grend/gl_manager.hpp>
#include <grend/sdl-context.hpp>
#include <grend/glm-includes.hpp>
#include <grend/model.hpp>
#include <grend/texture-atlas.hpp>
#include <grend/quadtree.hpp>

#include <list>
#include <memory>

#include <stdint.h>

static const size_t MAX_LIGHTS = 16;

namespace grendx {

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
struct reflection_probe {
	glm::vec3 position;
	quadtree::node_id faces[6];
	bool changed = true;
	bool is_static = true;
	bool have_map = false;
	// TODO: maybe non-static with update frequency
};

class renderer {
	friend class text_renderer;
	friend class game_state;

	public:
		renderer();
		~renderer() { };

		struct draw_attributes {
			std::string name;
			glm::mat4 transform;
			GLenum face_order = GL_CCW;
			bool cull_faces = true;
		};

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

		void set_material(gl_manager::compiled_model& obj, std::string mat_name);
		void set_default_material(std::string mat_name);

		// init_lights() will need to be called after the shader is bound
		// (in whatever subclasses the engine)
		//void init_lights(void);
		void compile_lights(void);
		void update_lights(void);
		void touch_light_shadowmaps(void);
		void touch_light_refprobes(void);
		void sync_point_lights(const std::vector<uint32_t>& lights);
		void sync_spot_lights(const std::vector<uint32_t>& lights);
		void sync_directional_lights(const std::vector<uint32_t>& lights);

		bool is_valid_light(uint32_t id);
		uint32_t alloc_light(void) { return ++light_ids; };
		void free_light(uint32_t id);

		uint32_t add_light(struct point_light lit);
		uint32_t add_light(struct spot_light lit);
		uint32_t add_light(struct directional_light lit);
		uint32_t add_reflection_probe(struct reflection_probe ref);
		void free_reflection_probe(uint32_t id);

		struct point_light       get_point_light(uint32_t id);
		struct spot_light        get_spot_light(uint32_t id);
		struct directional_light get_directional_light(uint32_t id);

		void set_point_light(uint32_t id, struct point_light *lit);
		void set_spot_light(uint32_t id, struct spot_light *lit);
		void set_directional_light(uint32_t id, struct directional_light *lit);

		void set_shader(Program::ptr shd);
		void set_mvp(glm::mat4 mod, glm::mat4 view, glm::mat4 projection);
		void set_m(glm::mat4 mod);
		gl_manager& get_glman(void){ return glman; };

		bool running = true;

		// map light IDs to light structures
		uint32_t light_ids = 0;
		struct std::map<uint32_t, struct point_light> point_lights;
		struct std::map<uint32_t, struct spot_light> spot_lights;
		struct std::map<uint32_t, struct directional_light> directional_lights;

		struct {
			// index into light struct maps
			std::vector<uint32_t> point;
			std::vector<uint32_t> spot;
			std::vector<uint32_t> directional;
		} active_lights;

		uint32_t refprobe_ids = 0;
		std::map<uint32_t, struct reflection_probe> ref_probes;
		std::unique_ptr<atlas> reflection_atlas;
		std::unique_ptr<atlas> shadow_atlas;

		// XXX: loaded shaders, here so they can be accessed from the editor
		std::map<std::string, Program::ptr> shaders;

	protected:
		Program::ptr shader;
		gl_manager glman;

		// list of models to draw
		// TODO: meshes, need to pair materials with meshes though, which
		//       currently is part of the model classes
		//       possible solution is to have a global namespace for materials,
		//       which would be easier to work with probably
		//       (also might make unintentional dependencies easier though...)
		std::vector<std::pair<std::string, draw_attributes>> draw_queue;
		// sorted after dqueue_sort_draws(), and flushed along with
		// dqueue_flush_draws(), this is sorted back-to-front rather than
		// front-to-back
		std::vector<std::pair<std::string, draw_attributes>> transparent_draws;

		std::string fallback_material = "(null)";

		std::map<std::string, Texture::ptr> diffuse_handles;
		std::map<std::string, Texture::ptr> specular_handles;
		std::map<std::string, Texture::ptr> normmap_handles;
		std::map<std::string, Texture::ptr> aomap_handles;
};

float light_extent(struct point_light *p, float threshold=0.03);
float light_extent(struct spot_light *s, float threshold=0.03);

// namespace grendx
}
