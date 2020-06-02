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

class engine {
	friend class text_renderer;

	public:
		struct point_light {
			glm::vec3 position;
			glm::vec4 diffuse;
			float radius;
			float intensity;
			quadtree::node_id shadowmap[6];
			bool changed = true;
		};

		struct spot_light {
			glm::vec3 position;
			glm::vec4 diffuse;
			glm::vec3 direction;
			float radius; // bulb radius
			float intensity;
			float angle;
			quadtree::node_id shadowmap;
			bool changed = true;
		};

		struct directional_light {
			glm::vec3 position;
			glm::vec4 diffuse;
			glm::vec3 direction;
			float intensity;
			quadtree::node_id shadowmap;
			bool changed = true;
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
		};

		engine();
		~engine() { };

		// TODO: maybe this should be split out of this class, have some sort of
		//       render loop class (rename that to engine?) and have this
		//       be the "render manager" class
		virtual void render(context& ctx) = 0;
		virtual void logic(context& ctx) = 0;
		virtual void physics(context& ctx) = 0;
		virtual void input(context& ctx) = 0;

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

		void set_shader(gl_manager::rhandle& shd);
		void set_mvp(glm::mat4 mod, glm::mat4 view, glm::mat4 projection);
		void set_m(glm::mat4 mod);
		const gl_manager& get_glman(void){ return glman; };

		bool running = true;


		/*
		int add_light(struct light lit);
		int set_light(int id, struct light lit);
		int get_light(int id, struct light *lit);
		void remove_light(int id);
		*/

		// TODO: make an accessor function for this, for debug drawing in the
		//       editor
		// TODO: need to keep the max number of lights synced between
		//       the engine code and shader code, something to keep in mind
		//       if/when writing a shader preprocessor language
		//unsigned active_lights = 0;
		//struct light lights[MAX_LIGHTS];

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

		std::vector<struct reflection_probe> ref_probes;
		std::unique_ptr<atlas> reflection_atlas;
		std::unique_ptr<atlas> shadow_atlas;

	protected:
		gl_manager::rhandle shader;
		gl_manager glman;

		GLint u_diffuse_map;
		GLint u_specular_map;
		GLint u_normal_map;
		GLint u_ao_map;

		// list of models to draw
		// TODO: meshes, need to pair materials with meshes though, which
		//       currently is part of the model classes
		//       possible solution is to have a global namespace for materials,
		//       which would be easier to work with probably
		//       (also might make unintentional dependencies easier though...)
		//std::vector<std::pair<std::string, glm::mat4>> draw_queue;
		std::vector<draw_attributes> draw_queue;

		std::string fallback_material = "(null)";
		//std::string fallback_material = "Rock";

		std::map<std::string, gl_manager::rhandle> diffuse_handles;
		std::map<std::string, gl_manager::rhandle> specular_handles;
		std::map<std::string, gl_manager::rhandle> normmap_handles;
		std::map<std::string, gl_manager::rhandle> aomap_handles;
};

float light_extent(struct engine::point_light *p, float threshold=0.03);
float light_extent(struct engine::spot_light *s, float threshold=0.03);

// namespace grendx
}
