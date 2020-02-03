#pragma once

#include <grend/gl_manager.hpp>
#include <grend/sdl-context.hpp>
#include <grend/glm-includes.hpp>
#include <grend/model.hpp>

namespace grendx {

class engine {
	public:
		engine();
		~engine() { };

		virtual void render(context& ctx) = 0;
		virtual void logic(context& ctx) = 0;
		virtual void physics(context& ctx) = 0;
		virtual void input(context& ctx) = 0;

		void draw_mesh(std::string name, glm::mat4 transform);
		void draw_mesh_lines(std::string name, glm::mat4 transform);
		void draw_model(std::string name, glm::mat4 transform);
		void draw_model_lines(std::string name, glm::mat4 transform);

		void set_material(gl_manager::compiled_model& obj, std::string mat_name);
		void set_default_material(std::string mat_name);

		void set_mvp(glm::mat4 mod, glm::mat4 view, glm::mat4 projection);
		void set_m(glm::mat4 mod);

		bool running = true;

	protected:
		gl_manager::rhandle shader;
		gl_manager glman;
		GLint u_diffuse_map;
		GLint u_specular_map;
		GLint u_normal_map;
		GLint u_ao_map;

		std::string fallback_material = "(null)";
		//std::string fallback_material = "Rock";

		std::map<std::string, gl_manager::rhandle> diffuse_handles;
		std::map<std::string, gl_manager::rhandle> specular_handles;
		std::map<std::string, gl_manager::rhandle> normmap_handles;
		std::map<std::string, gl_manager::rhandle> aomap_handles;
};

// namespace grendx
}
