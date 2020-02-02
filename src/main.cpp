#include <grend/sdl-context.hpp>
#include <grend/model.hpp>
#include <grend/glm-includes.hpp>
#include <grend/geometry-gen.hpp>
#include <grend/grend.hpp>
#include <grend/utility.hpp>

#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/types.h>

#include <tuple>
#include <vector>
#include <fstream>
#include <iostream>
#include <sstream>
#include <iterator>
#include <string>
#include <exception>
#include <memory>
#include <utility>
#include <map>
#include <list>

using namespace grendx;

std::map<std::string, material> default_materials = {
	{"(null)", {
				   .diffuse = {0.75, 0.75, 0.75, 1},
				   .ambient = {1, 1, 1, 1},
				   .specular = {0.5, 0.5, 0.5, 1},
				   .shininess = 15,

				   .diffuse_map  = "assets/tex/white.png",
				   .specular_map = "assets/tex/white.png",
				   .normal_map   = "assets/tex/lightblue-normal.png",
			   }},

	{"Black",  {
				   .diffuse = {1, 0.8, 0.2, 1},
				   .ambient = {1, 0.8, 0.2, 1},
				   .specular = {1, 1, 1, 1},
				   .shininess = 50
			   }},

	{"Grey",   {
				   .diffuse = {0.1, 0.1, 0.1, 0.5},
				   .ambient = {0.1, 0.1, 0.1, 0.2},
				   .specular = {0.0, 0.0, 0.0, 0.05},
				   .shininess = 15
			   }},

	{"Yellow", {
				   .diffuse = {0.01, 0.01, 0.01, 1},
				   .ambient = {0, 0, 0, 1},
				   .specular = {0.2, 0.2, 0.2, 0.2},
				   .shininess = 20,
			   }},

	{"Gravel",  {
				   .diffuse = {1, 1, 1, 1},
				   .ambient = {1, 1, 1, 1},
				   .specular = {1, 1, 1, 1},
				   .shininess = 5,

				   .diffuse_map  = "assets/tex/dims/Textures/Gravel.JPG",
				   .specular_map = "assets/tex/white.png",
				   .normal_map   = "assets/tex/dims/Textures/Textures_N/Gravel_N.jpg",

				   // materials from freepbr.com are way higher quality, but I'm not sure how
				   // github would feel about hosting a few hundred megabytes to gigabytes of
				   // textures
				   /*
				   .diffuse_map  = "assets/tex/octostone-Unreal-Engine/octostoneAlbedo.png",
				   .specular_map = "assets/tex/octostone-Unreal-Engine/octostoneMetallic.png",
				   .normal_map   = "assets/tex/octostone-Unreal-Engine/octostoneNormalc.png",
				   */
			   }},

	{"Steel",  {
				   .diffuse = {1, 1, 1, 1},
				   .ambient = {1, 1, 1, 1},
				   .specular = {1, 1, 1, 1},
				   .shininess = 35,

				   .diffuse_map  = "assets/tex/rubberduck-tex/199.JPG",
				   .specular_map = "assets/tex/white.png",
				   .normal_map   = "assets/tex/rubberduck-tex/199_norm.JPG",

				   /*
				   .diffuse_map  = "assets/tex/iron-rusted4-Unreal-Engine/iron-rusted4-basecolor.png",
				   .specular_map = "assets/tex/iron-rusted4-Unreal-Engine/iron-rusted4-metalness.png",
				   .normal_map   = "assets/tex/iron-rusted4-Unreal-Engine/iron-rusted4-normal.png",
				   */
			   }},

	{"Brick",  {
				   .diffuse = {1, 1, 1, 1},
				   .ambient = {1, 1, 1, 1},
				   .specular = {1, 1, 1, 1},
				   .shininess = 3,

				   .diffuse_map  = "assets/tex/rubberduck-tex/179.JPG",
				   .specular_map = "assets/tex/white.png",
				   .normal_map   = "assets/tex/rubberduck-tex/179_norm.JPG",
				   /*
				   .diffuse_map  = "assets/tex/dims/Textures/Chimeny.JPG",
				   .specular_map = "assets/tex/white.png",
				   .normal_map   = "assets/tex/dims/Textures/Textures_N/Chimeny_N.jpg",
				   */
			   }},


	{"Rock",  {
				   .diffuse = {1, 1, 1, 1},
				   .ambient = {1, 1, 1, 1},
				   .specular = {1, 1, 1, 0.5},
				   .shininess = 5,

				   .diffuse_map  = "assets/tex/rubberduck-tex/165.JPG",
				   .specular_map = "assets/tex/white.png",
				   .normal_map   = "assets/tex/rubberduck-tex/165_norm.JPG",
			   }},

	{"Wood",  {
				   .diffuse = {1, 1, 1, 1},
				   .ambient = {1, 1, 1, 1},
				   .specular = {1, 1, 1, 0.3},
				   .shininess = 5,

				   .diffuse_map  = "assets/tex/dims/Textures/Boards.JPG",
				   .specular_map = "assets/tex/white.png",
				   .normal_map   = "assets/tex/dims/Textures/Textures_N/Boards_N.jpg",
			   }},

	{"Clover",  {
				   .diffuse = {1, 1, 1, 1},
				   .ambient = {1, 1, 1, 1},
				   .specular = {1, 1, 1, 0.5},
				   .shininess = 1,

				   .diffuse_map  = "assets/tex/dims/Textures/GroundCover.JPG",
				   .specular_map = "assets/tex/white.png",
				   .normal_map   = "assets/tex/dims/Textures/Textures_N/GroundCover_N.jpg",
			   }},
};

model_map load_library(std::string dir) {
	model_map ret;
	struct dirent *dirent;
	DIR *dirp;

	if (!(dirp = opendir(dir.c_str()))) {
		std::cerr << "Warning: couldn't load models from " << dir << std::endl;
		return ret;
	}

	while ((dirent = readdir(dirp))) {
		std::string fname = dirent->d_name;
		std::size_t pos = fname.rfind('.');

		if (pos == std::string::npos) {
			// no extension
			continue;
		}

		if (fname.substr(pos) == ".obj") {
			std::cerr << "   - " << fname << std::endl;;
			ret[fname] = model(dir + "/" + fname);
		}
	}

	closedir(dirp);
	return ret;
}

static glm::mat4 model_to_world(glm::mat4 model) {
	glm::quat rotation = glm::quat_cast(model);
	//rotation = -glm::conjugate(rotation);

	return glm::mat4_cast(rotation);
}

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

		void set_material(grend::compiled_model& obj, std::string mat_name);
		void set_default_material(std::string mat_name);

		void set_mvp(glm::mat4 mod, glm::mat4 view, glm::mat4 projection);
		void set_m(glm::mat4 mod);

		bool running = true;

	protected:
		grend::rhandle shader;
		grend glman;
		GLint u_diffuse_map;
		GLint u_specular_map;
		GLint u_normal_map;

		std::string fallback_material = "(null)";
		//std::string fallback_material = "Rock";

		std::map<std::string, grend::rhandle> diffuse_handles;
		std::map<std::string, grend::rhandle> specular_handles;
		std::map<std::string, grend::rhandle> normmap_handles;
};

engine::engine() {
	for (auto& thing : default_materials) {
		if (!thing.second.diffuse_map.empty()) {
			diffuse_handles[thing.first] = glman.load_texture(thing.second.diffuse_map);
		}

		if (!thing.second.specular_map.empty()) {
			specular_handles[thing.first] = glman.load_texture(thing.second.specular_map);
		}

		if (!thing.second.normal_map.empty()) {
			normmap_handles[thing.first] = glman.load_texture(thing.second.normal_map);
		}
	}
}

void engine::set_material(grend::compiled_model& obj, std::string mat_name) {
	if (obj.materials.find(mat_name) == obj.materials.end()) {
		// TODO: maybe show a warning
		set_default_material(mat_name);

	} else {
		material& mat = obj.materials[mat_name];

		glUniform4f(glGetUniformLocation(shader.first, "anmaterial.diffuse"),
				mat.diffuse.x, mat.diffuse.y, mat.diffuse.z, mat.diffuse.w);
		glUniform4f(glGetUniformLocation(shader.first, "anmaterial.ambient"),
				mat.ambient.x, mat.ambient.y, mat.ambient.z, mat.ambient.w);
		glUniform4f(glGetUniformLocation(shader.first, "anmaterial.specular"),
				mat.specular.x, mat.specular.y, mat.specular.z, mat.specular.w);
		glUniform1f(glGetUniformLocation(shader.first, "anmaterial.shininess"),
				mat.shininess);

		glActiveTexture(GL_TEXTURE0);
		if (!mat.diffuse_map.empty()) {
			glBindTexture(GL_TEXTURE_2D, obj.mat_textures[mat_name].first);
			glUniform1i(u_diffuse_map, 0);

		} else {
			glBindTexture(GL_TEXTURE_2D, diffuse_handles[fallback_material].first);
			glUniform1i(u_diffuse_map, 0);
		}

		glActiveTexture(GL_TEXTURE1);
		if (!mat.specular_map.empty()) {
			// TODO: specular maps
		} else {
			glBindTexture(GL_TEXTURE_2D, specular_handles[fallback_material].first);
			glUniform1i(u_specular_map, 1);
		}

		glActiveTexture(GL_TEXTURE2);
		if (!mat.normal_map.empty()) {
			// TODO: normal maps

		} else {
			glBindTexture(GL_TEXTURE_2D, normmap_handles[fallback_material].first);
			glUniform1i(u_normal_map, 2);
		}
	}
}

void engine::set_default_material(std::string mat_name) {
	if (default_materials.find(mat_name) == default_materials.end()) {
		// TODO: really show an error here
		mat_name = fallback_material;
		puts("asdf");
	}

	material& mat = default_materials[mat_name];

	glUniform4f(glGetUniformLocation(shader.first, "anmaterial.diffuse"),
			mat.diffuse.x, mat.diffuse.y, mat.diffuse.z, mat.diffuse.w);
	glUniform4f(glGetUniformLocation(shader.first, "anmaterial.ambient"),
			mat.ambient.x, mat.ambient.y, mat.ambient.z, mat.ambient.w);
	glUniform4f(glGetUniformLocation(shader.first, "anmaterial.specular"),
			mat.specular.x, mat.specular.y, mat.specular.z, mat.specular.w);
	glUniform1f(glGetUniformLocation(shader.first, "anmaterial.shininess"),
			mat.shininess);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, diffuse_handles[mat.diffuse_map.empty()?
	                                                 fallback_material
	                                                 : mat_name].first);
	glUniform1i(u_diffuse_map, 0);


	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, specular_handles[mat.specular_map.empty()?
	                                                 fallback_material
	                                                 : mat_name].first);
	glUniform1i(u_specular_map, 1);

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, normmap_handles[mat.normal_map.empty()?
	                                                 fallback_material
	                                                 : mat_name].first);
	glUniform1i(u_normal_map, 2);
}

void engine::set_mvp(glm::mat4 mod, glm::mat4 view, glm::mat4 projection) {
	glm::mat4 v_inv = glm::inverse(view);

	glUniformMatrix4fv(glGetUniformLocation(shader.first, "v"),
			1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(glGetUniformLocation(shader.first, "p"),
			1, GL_FALSE, glm::value_ptr(projection));

	glUniformMatrix4fv(glGetUniformLocation(shader.first, "v_inv"),
			1, GL_FALSE, glm::value_ptr(v_inv));
}

void engine::set_m(glm::mat4 mod) {
	glm::mat3 m_3x3_inv_transp = glm::transpose(glm::inverse(model_to_world(mod)));

	glUniformMatrix4fv(glGetUniformLocation(shader.first, "m"),
			1, GL_FALSE, glm::value_ptr(mod));
	glUniformMatrix3fv(glGetUniformLocation(shader.first, "m_3x3_inv_transp"),
			1, GL_FALSE, glm::value_ptr(m_3x3_inv_transp));
}

class testscene : public engine {
	public:
		testscene();
		virtual ~testscene();
		virtual void render(context& ctx);
		virtual void logic(context& ctx);
		virtual void physics(context& ctx);
		virtual void input(context& ctx);

		void save_map(std::string name="save.map");
		void load_map(std::string name="save.map");

		glm::mat4 projection, view;

		glm::vec3 view_position = glm::vec3(0, 6, 5);
		glm::vec3 view_velocity;

		glm::vec3 view_direction = glm::vec3(0, 0, -1);
		glm::vec3 view_up = glm::vec3(0, 1, 0);
		glm::vec3 view_right = glm::vec3(1, 0, 0);

		struct editor_entry {
			std::string name;
			glm::vec3   position;
			// TODO: full rotation
			glm::mat4   transform;
			bool        inverted;

			// TODO: might be a good idea to keep track of whether face culling is enabled for
			//       this model, although that might be better off in the model class itself...
		};

		// Map editing things
		std::vector<editor_entry> dynamic_models;
		model_map::iterator select_model;

		bool      in_select_mode = false;
		glm::vec3 select_position = glm::vec3(0, 0, 0);
		glm::mat4 select_transform = glm::mat4(1);
		float     select_distance = 5;
		// this keeps track of the current face order after flipping along an axis
		bool      select_inverted = false;


		Uint32 last_frame;

		// models
		model_map models = {
			{"person",    model("assets/obj/low-poly-character-rpg/boy.obj")},
			{"teapot",    model("assets/obj/teapot.obj")},
			{"monkey",    model("assets/obj/suzanne.obj")},
			{"unit_cube", generate_cuboid(1, 1, 1)},
			{"unit_cube_wood", generate_cuboid(1, 1, 1)},
			{"unit_cube_ground", generate_cuboid(1, 1, 1)},
			{"grid",      generate_grid(-32, -32, 32, 32, 4)},
		};

		std::list<std::string> libraries = {
			/*
			"assets/obj/Modular Terrain Cliff/",
			"assets/obj/Modular Terrain Hilly/",
			"assets/obj/Modular Terrain Beach/",
			"assets/obj/Dungeon Set 2/",
			*/
		};

		grend::rhandle textures[8] = {
			glman.load_texture("assets/tex/white.png"),
		};
};

testscene::testscene() : engine() {
	projection = glm::perspective(glm::radians(60.f),
	                             (1.f*SCREEN_SIZE_X)/SCREEN_SIZE_Y, 0.1f, 100.f);
	view = glm::lookAt(glm::vec3(0.0, 1.0, 5.0),
	                   glm::vec3(0.0, 0.0, 0.0),
	                   glm::vec3(0.0, 1.0, 0.0));
	select_model = models.begin();

	for (std::string libname : libraries) {
		// inserting each library into the models map so that way
		// we can access them from the editor, but could also do another
		// compile_models() call before bind_cooked_meshes to load the models.
		auto library = load_library(libname);
		models.insert(library.begin(), library.end());
	}

	models["teapot"].meshes["default"].material = "Steel";
	models["grid"].meshes["default"].material = "Gravel";
	models["monkey"].meshes["Monkey"].material = "Wood";
	models["unit_cube"].meshes["default"].material = "Rock";
	models["unit_cube_wood"].meshes["default"].material = "Wood";
	models["unit_cube_ground"].meshes["default"].material = "Clover";

	glman.compile_models(models);
	glman.bind_cooked_meshes();

	/*
	vertex_shader = load_shader("assets/shaders/vertex-shading.vert", GL_VERTEX_SHADER);
	fragment_shader = load_shader("assets/shaders/vertex-shading.frag", GL_FRAGMENT_SHADER);
	*/

	grend::rhandle vertex_shader, fragment_shader;
	vertex_shader = glman.load_shader("assets/shaders/pixel-shading.vert", GL_VERTEX_SHADER);
	fragment_shader = glman.load_shader("assets/shaders/pixel-shading.frag", GL_FRAGMENT_SHADER);

	shader = glman.gen_program();

	glAttachShader(shader.first, vertex_shader.first);
	glAttachShader(shader.first, fragment_shader.first);

	// monkey business
	fprintf(stderr, " # have %lu vertices\n", glman.cooked_vertices.size());
	glBindAttribLocation(shader.first, glman.cooked_vert_vbo.first, "in_Position");
	glBindAttribLocation(shader.first, glman.cooked_texcoord_vbo.first, "texcoord");
	glBindAttribLocation(shader.first, glman.cooked_normal_vbo.first, "v_normal");
	glBindAttribLocation(shader.first,
	                     glman.cooked_tangent_vbo.first, "v_tangent");
	glBindAttribLocation(shader.first,
	                     glman.cooked_bitangent_vbo.first, "v_bitangent");
	glLinkProgram(shader.first);

	int linked;

	glGetProgramiv(shader.first, GL_LINK_STATUS, &linked);
	if (!linked) {
		SDL_Die("couldn't link shaders");
	}

	glUseProgram(shader.first);

	if ((u_diffuse_map = glGetUniformLocation(shader.first, "diffuse_map")) == -1) {
		//SDL_Die("Couldn't bind diffuse_map");
		std::cerr << "Couldn't bind diffuse map" << std::endl;
	}

	if ((u_specular_map = glGetUniformLocation(shader.first, "specular_map")) == -1) {
		std::cerr << "Couldn't bind specular map" << std::endl;
		//SDL_Die("Couldn't bind specular_map");
	}

	if ((u_normal_map = glGetUniformLocation(shader.first, "normal_map")) == -1) {
		std::cerr << "Couldn't bind normal map" << std::endl;
		//SDL_Die("Couldn't bind normal_map");
	}
}

testscene::~testscene() {
	puts("got here");
}

void engine::draw_mesh(std::string name, glm::mat4 transform) {
	grend::compiled_mesh& foo = glman.cooked_meshes[name];
	set_m(transform);

	glman.bind_vao(foo.vao);
	glDrawElements(GL_TRIANGLES, foo.elements_size, GL_UNSIGNED_SHORT, foo.elements_offset);
}

// TODO: overload of this that takes a material
void engine::draw_mesh_lines(std::string name, glm::mat4 transform) {
	grend::compiled_mesh& foo = glman.cooked_meshes[name];

	set_m(transform);
	glman.bind_vao(foo.vao);

	glDrawElements(GL_LINES, foo.elements_size, GL_UNSIGNED_SHORT, foo.elements_offset);
}

void engine::draw_model(std::string name, glm::mat4 transform) {
	grend::compiled_model& obj = glman.cooked_models[name];

	for (std::string& name : obj.meshes) {
		set_material(obj, glman.cooked_meshes[name].material);
		draw_mesh(name, transform);
	}
}

void engine::draw_model_lines(std::string name, glm::mat4 transform) {
	grend::compiled_model& obj = glman.cooked_models[name];
	// TODO: need a set_material() function to handle stuff
	//       and we need to explicitly set a material

	set_default_material("(null)");
	for (std::string& name : obj.meshes) {
		draw_mesh_lines(name, transform);
	}
}

void testscene::render(context& ctx) {
	//glClearColor(0.7, 0.9, 1, 1);
	glClearColor(0.1, 0.1, 0.1, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	/*
	glActiveTexture(GL_TEXTURE0);

	glBindTexture(GL_TEXTURE_2D, textures[0].first);
	glUniform1i(u, 0);
	*/

#ifdef ENABLE_FACE_CULLING
	// TODO: toggle per-model
	glEnable(GL_CULL_FACE);
	// make sure we always start with counter-clockwise faces
	// (also should be toggled per-model)
	glFrontFace(GL_CCW);
#endif

	glm::vec3 hpos = glm::vec3(view_direction.x*5 + view_position.x, 0, view_direction.z*5 + view_position.z);
	glm::mat4 bizz = glm::translate(glm::mat4(1), hpos);
	float asdf = 0.5*(SDL_GetTicks()/1000.f);
	glm::vec4 lfoo = glm::vec4(15*cos(asdf), 5, 15*sin(asdf), 1.f);

	glUniform4fv(glGetUniformLocation(shader.first, "lightpos"),
			1, glm::value_ptr(lfoo));

	set_mvp(glm::mat4(1), view, projection);
	draw_model("person", bizz);

	if (in_select_mode) {
		glm::mat4 trans = glm::translate(select_position) * select_transform;

		glFrontFace(select_inverted? GL_CW : GL_CCW);
		draw_model_lines(select_model->first, trans);
		draw_model(select_model->first, trans);
	}

	for (auto& v : dynamic_models) {
		glm::mat4 trans = glm::translate(v.position) * v.transform;

		glFrontFace(v.inverted? GL_CW : GL_CCW);
		draw_model(v.name, trans);
	}

	SDL_GL_SwapWindow(ctx.window);
}

void testscene::input(context& ctx) {
	Uint32 cur_ticks = SDL_GetTicks();
	Uint32 ticks_delta = cur_ticks - last_frame;

	float fticks = ticks_delta / 1000.0f;

	const float movement_speed = 10 /* units/s */;
	const float rotation_speed = 1;

	SDL_Event ev;

	while (SDL_PollEvent(&ev)) {
		if (ev.type == SDL_QUIT) {
			running = false;
		}

		else if (ev.type == SDL_KEYDOWN) {
			switch (ev.key.keysym.sym) {
				case SDLK_w: view_velocity.z =  movement_speed; break;
				case SDLK_s: view_velocity.z = -movement_speed; break;
				case SDLK_a: view_velocity.x = -movement_speed; break;
				case SDLK_d: view_velocity.x =  movement_speed; break;
				case SDLK_q: view_velocity.y =  movement_speed; break;
				case SDLK_e: view_velocity.y = -movement_speed; break;
				case SDLK_m: in_select_mode = !in_select_mode; break;

				case SDLK_ESCAPE: running = false; break;
			}
		}

		else if (ev.type == SDL_KEYUP) {
			switch (ev.key.keysym.sym) {
				case SDLK_w:
				case SDLK_s:
					view_velocity.z = 0;
					break;

				case SDLK_a:
				case SDLK_d:
					view_velocity.x = 0;
					break;

				case SDLK_q:
				case SDLK_e:
					view_velocity.y = 0;
					break;
			}
		}

		else if (ev.type == SDL_WINDOWEVENT) {
			switch (ev.window.event) {
				case SDL_WINDOWEVENT_RESIZED:
				case SDL_WINDOWEVENT_SIZE_CHANGED:
					{
						int win_x, win_y;
						SDL_GetWindowSize(ctx.window, &win_x, &win_y);
						std::cerr << " * resized window: " << win_x << ", " << win_y << std::endl;
						glViewport(0, 0, win_x, win_y);
						projection = glm::perspective(glm::radians(60.f),
						                              (1.f*win_x)/win_y, 0.1f, 100.f);

					}
					break;
			}
		}

		// keeping editing commands seperate from the main input handling
		if (in_select_mode) {
			if (ev.type == SDL_KEYDOWN) {
				switch (ev.key.keysym.sym) {
					case SDLK_i: load_map(); break;
					case SDLK_o: save_map(); break;

					case SDLK_z:
						//select_rotation -= M_PI/4;
						select_transform *= glm::rotate((float)-M_PI/4.f, glm::vec3(0, 1, 0));
						break;

					case SDLK_c:
						//select_rotation += M_PI/4;
						select_transform *= glm::rotate((float)M_PI/4.f, glm::vec3(0, 1, 0));
						break;

					case SDLK_x:
						// flip horizontally (along the X axis)
						select_transform *= glm::scale(glm::vec3(-1, 1, 1));
						select_inverted = !select_inverted;
						break;

					case SDLK_b:
						// flip horizontally (along the Z axis)
						select_transform *= glm::scale(glm::vec3( 1, 1, -1));
						select_inverted = !select_inverted;
						break;

					case SDLK_v:
						// flip vertically
						select_transform *= glm::scale(glm::vec3( 1, -1, 1));
						select_inverted = !select_inverted;
						break;

					case SDLK_r:
						if (select_model == models.begin()) {
							select_model = models.end();
						}
						select_model--;
						break;

					case SDLK_f:
						select_model++;
						if (select_model == models.end()) {
							select_model = models.begin();
						}
						break;

					case SDLK_DELETE:
						// undo, basically
						if (!dynamic_models.empty()) {
							dynamic_models.pop_back();
						}
						break;
				}
			}

			else if (ev.type == SDL_MOUSEWHEEL) {
				select_distance -= ev.wheel.y/10.f /* fidelity */;
			}

			else if (ev.type == SDL_MOUSEBUTTONDOWN) {
				if (ev.button.button == SDL_BUTTON_LEFT) {
					dynamic_models.push_back({
						select_model->first,
						select_position,
						select_transform,
						select_inverted,
					});
				}
			}
		}
	}

	int x, y;
	int win_x, win_y;
	Uint32 buttons = SDL_GetMouseState(&x, &y);
	SDL_GetWindowSize(ctx.window, &win_x, &win_y);

	x = (x > 0)? x : win_x/2;
	y = (x > 0)? y : win_y/2;

	// TODO: get window resolution
	float center_x = (float)win_x / 2;
	float center_y = (float)win_y / 2;

	float rel_x = ((float)x - center_x) / center_x;
	float rel_y = ((float)y - center_y) / center_y;

	// adjust view
	view_direction = glm::normalize(glm::vec3(
		rotation_speed * rel_x,
		rotation_speed *-rel_y,
		rotation_speed * -cos(rel_x*M_PI)
	));

	view_right = glm::normalize(glm::cross(glm::vec3(0, 1, 0), view_direction));
	view_up    = glm::normalize(glm::cross(view_direction, view_right));

	view_position += view_velocity.z * view_direction * fticks;
	view_position += view_velocity.y * view_up * fticks;
	view_position += view_velocity.x * glm::normalize(glm::cross(view_direction, view_up)) * fticks;

	// TODO: "fidelity" parameter to help align objects
	float fidelity = 10.f;
	auto align = [&] (float x) { return floor(x * fidelity)/fidelity; };

	select_position = glm::vec3(align(view_direction.x*select_distance + view_position.x),
	                            align(view_direction.y*select_distance + view_position.y),
	                            align(view_direction.z*select_distance + view_position.z));
}

void testscene::physics(context& ctx) {
	// nothing to do yet
}

void testscene::logic(context& ctx) {
	Uint32 cur_ticks = SDL_GetTicks();
	Uint32 ticks_delta = cur_ticks - last_frame;

	float fticks = ticks_delta / 1000.0f;
	float angle = fticks * glm::radians(-45.f);

	last_frame = cur_ticks;

	view = glm::lookAt(view_position, view_position + view_direction, view_up);
}

void testscene::save_map(std::string name) {
	std::ofstream foo(name);
	std::cerr << "saving map " << name << std::endl;

	if (!foo.good()) {
		std::cerr << "couldn't open save file" << name << std::endl;
		return;
	}

	foo << "### test scene save file" << std::endl;

	for (std::string& lib : libraries) {
		foo << "library\t" << lib << std::endl;
	}

	for (auto& v : dynamic_models) {
		foo << "entity\t" << v.name << "\t"
			<< v.position.x << "," << v.position.y << "," << v.position.z << "\t";

		for (unsigned y = 0; y < 4; y++) {
			for (unsigned x = 0; x < 4; x++) {
				foo << v.transform[y][x] << ",";
			}
		}

		foo << "\t" << v.inverted;
		foo << std::endl;
	}
}

void testscene::load_map(std::string name) {
	std::ifstream foo(name);
	std::cerr << "loading map " << name << std::endl;

	if (!foo.good()) {
		std::cerr << "couldn't open save file" << name << std::endl;
		return;
	}

	std::string line;
	while (std::getline(foo, line)) {
		auto statement = split_string(line, '\t');
		if (line[0] == '#' || line[0] == '\n') {
			continue;
		}

		if (statement[0] == "entity" && statement.size() >= 5) {
			auto posvec = split_string(statement[2], ',');
			auto matvec = split_string(statement[3], ',');

			editor_entry v;
			v.name = statement[1];
			v.position = glm::vec3(std::stof(posvec[0]), std::stof(posvec[1]), std::stof(posvec[2]));
			v.inverted = std::stoi(statement[4]);

			for (unsigned i = 0; i < 16; i++) {
				v.transform[i/4][i%4] = std::stof(matvec[i]);
			}

			dynamic_models.push_back(v);
			std::cerr << "# loaded a " << v.name << std::endl;
		}
	}
}

int main(int argc, char *argv[]) {
	context ctx("grend test");
	std::unique_ptr<engine> scene(new testscene());

	while (scene->running) {
		scene->input(ctx);

		if (scene->running) {
			scene->logic(ctx);
			scene->render(ctx);
		}
	}

	return 0;
}
