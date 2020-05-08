#pragma once

#include <grend/gl_manager.hpp>
#include <grend/sdl-context.hpp>
#include <grend/glm-includes.hpp>
#include <grend/model.hpp>
#include <list>

#define MAX_LIGHTS 32

namespace grendx {

class engine {
	friend class text_renderer;

	public:
		struct light {
			glm::vec4 position;
			glm::vec4 diffuse;
			float const_attenuation, linear_attenuation, quadratic_attenuation;
			float specular;
			bool changed = false;
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

		void draw_mesh(std::string name, glm::mat4 transform);
		void draw_mesh_lines(std::string name, glm::mat4 transform);
		void draw_model(std::string name, glm::mat4 transform);
		void draw_model_lines(std::string name, glm::mat4 transform);
		void draw_screenquad(void);

		void dqueue_draw_model(std::string name, glm::mat4 transform);
		void dqueue_sort_draws(glm::vec3 camera);
		void dqueue_cull_models(glm::vec3 camera);
		void dqueue_flush_draws(void);

		void set_material(gl_manager::compiled_model& obj, std::string mat_name);
		void set_default_material(std::string mat_name);

		// init_lights() will need to be called after the shader is bound
		// (in whatever subclasses the engine)
		void init_lights(void);
		void sync_light(unsigned id);
		void update_lights(void);

		bool is_valid_light(int id);
		int add_light(struct light lit);
		int set_light(int id, struct light lit);
		int get_light(int id, struct light *lit);
		void remove_light(int id);

		void set_shader(gl_manager::rhandle& shd);
		void set_mvp(glm::mat4 mod, glm::mat4 view, glm::mat4 projection);
		void set_m(glm::mat4 mod);

		const gl_manager& get_glman(void){ return glman; };

		bool running = true;

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
		std::vector<std::pair<std::string, glm::mat4>> draw_queue;

		std::string fallback_material = "(null)";
		//std::string fallback_material = "Rock";

		std::map<std::string, gl_manager::rhandle> diffuse_handles;
		std::map<std::string, gl_manager::rhandle> specular_handles;
		std::map<std::string, gl_manager::rhandle> normmap_handles;
		std::map<std::string, gl_manager::rhandle> aomap_handles;

		// TODO: need to keep the max number of lights synced between
		//       the engine code and shader code, something to keep in mind
		//       if/when writing a shader preprocessor language
		struct light lights[MAX_LIGHTS];
		unsigned active_lights = 0;

		// TODO: maybe have a bitmap to track changes to lights (so )

};



// namespace grendx
}
