#include <grend/engine.hpp>
#include <grend/sdl-context.hpp>
#include <grend/model.hpp>
#include <grend/glm-includes.hpp>
#include <grend/geometry-gen.hpp>
#include <grend/gl_manager.hpp>
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
			{"person",       model("assets/obj/low-poly-character-rpg/boy.obj")},
			{"teapot",       model("assets/obj/teapot.obj")},
			{"smoothteapot", model("assets/obj/smooth-teapot.obj")},
			{"monkey",       model("assets/obj/suzanne.obj")},
			{"smoothmonkey", model("assets/obj/smooth-suzanne.obj")},
			{"sphere",       model("assets/obj/sphere.obj")},
			{"smoothsphere", model("assets/obj/smoothsphere.obj")},
			{"unit_cube",        generate_cuboid(1, 1, 1)},
			{"unit_cube_wood",   generate_cuboid(1, 1, 1)},
			{"unit_cube_ground", generate_cuboid(1, 1, 1)},
			{"grid",             generate_grid(-32, -32, 32, 32, 4)},
		};

		std::list<std::string> libraries = {
			/*
			"assets/obj/Modular Terrain Cliff/",
			"assets/obj/Modular Terrain Hilly/",
			"assets/obj/Modular Terrain Beach/",
			*/
			"assets/obj/Dungeon Set 2/",
		};

		// dynamic lights
		int player_light;
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

	glman.compile_models(models);
	glman.bind_cooked_meshes();

	gl_manager::rhandle vertex_shader, fragment_shader;
	/*
	vertex_shader = glman.load_shader("shaders/vertex-shading.vert", GL_VERTEX_SHADER);
	fragment_shader = glman.load_shader("shaders/vertex-shading.frag", GL_FRAGMENT_SHADER);
	*/

	vertex_shader = glman.load_shader("shaders/pixel-shading.vert", GL_VERTEX_SHADER);
	fragment_shader = glman.load_shader("shaders/pixel-shading.frag", GL_FRAGMENT_SHADER);

	shader = glman.gen_program();

	glAttachShader(shader.first, vertex_shader.first);
	glAttachShader(shader.first, fragment_shader.first);
	DO_ERROR_CHECK();

	// monkey business
	fprintf(stderr, " # have %lu vertices\n", glman.cooked_vertices.size());
	glBindAttribLocation(shader.first, glman.cooked_vert_vbo.first, "in_Position");
	glBindAttribLocation(shader.first, glman.cooked_texcoord_vbo.first, "texcoord");
	glBindAttribLocation(shader.first, glman.cooked_normal_vbo.first, "v_normal");
	glBindAttribLocation(shader.first,
	                     glman.cooked_tangent_vbo.first, "v_tangent");
	glBindAttribLocation(shader.first,
	                     glman.cooked_bitangent_vbo.first, "v_bitangent");
	DO_ERROR_CHECK();

	int linked;
	glLinkProgram(shader.first);
	glGetProgramiv(shader.first, GL_LINK_STATUS, &linked);

	if (!linked) {
		SDL_Die("couldn't link shaders");
	}

	glUseProgram(shader.first);
	init_lights();
	// TODO: assert() + logger
	player_light = add_light((struct engine::light){
		.position = {0, 0, 0, 1},
		.diffuse  = {1.0, 0.9, 0.8, 0.0},
		.const_attenuation = 0.5f,
		.linear_attenuation = 0.f,
		.quadratic_attenuation = 0.05f,
		.specular = 1.0,
	});

	add_light((struct engine::light){
		.position = {-3, 1.5, 1.1, 1},
		.diffuse  = {1.0, 0.8, 0.5, 1.0},
		.const_attenuation = 0.5f,
		.linear_attenuation = 0.f,
		.quadratic_attenuation = 0.08f,
		.specular = 1.0,
	});

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

	if ((u_ao_map = glGetUniformLocation(shader.first, "ambient_occ_map")) == -1) {
		std::cerr << "Couldn't bind ambient occulsion map" << std::endl;
		//SDL_Die("Couldn't bind normal_map");
	}
}

testscene::~testscene() {
	puts("got here");
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
	DO_ERROR_CHECK();

	glm::vec3 hpos = glm::vec3(view_direction.x*3 + view_position.x, 0, view_direction.z*3 + view_position.z);
	glm::mat4 bizz =
		glm::translate(glm::mat4(1), hpos)
		* glm::scale(glm::vec3(0.75f, 0.75f, 0.75f));
	float asdf = 0.5*(SDL_GetTicks()/1000.f);
	glm::vec4 lfoo = glm::vec4(15*cos(asdf), 5, 15*sin(asdf), 1.f);
	//glm::vec4 lfoo = glm::vec4(hpos.x, 5, hpos.z, 1.f);
	struct engine::light lit;
	get_light(player_light, &lit);
	//lit.position = lfoo;
	lit.position = glm::vec4(hpos.x, hpos.y + 4, hpos.z, 1.f);
	set_light(player_light, lit);
	update_lights();

	glUniform4fv(glGetUniformLocation(shader.first, "lightpos"),
			1, glm::value_ptr(lfoo));

	set_mvp(glm::mat4(1), view, projection);
	draw_model("person", bizz);
	DO_ERROR_CHECK();

	if (in_select_mode) {
		glm::mat4 trans = glm::translate(select_position) * select_transform;

		glFrontFace(select_inverted? GL_CW : GL_CCW);
		draw_model_lines(select_model->first, trans);
		draw_model(select_model->first, trans);
	}
	DO_ERROR_CHECK();

	for (auto& v : dynamic_models) {
		glm::mat4 trans = glm::translate(v.position) * v.transform;

		glFrontFace(v.inverted? GL_CW : GL_CCW);
		if (in_select_mode) {
			draw_model_lines(v.name, trans);
		}
		draw_model(v.name, trans);
		DO_ERROR_CHECK();
	}

	SDL_GL_SwapWindow(ctx.window);
}

void testscene::input(context& ctx) {
	Uint32 cur_ticks = SDL_GetTicks();
	Uint32 ticks_delta = cur_ticks - last_frame;

	float fticks = ticks_delta / 1000.0f;

	const float movement_speed = 5 /* units/s */;
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
		rotation_speed * sin(rel_x*2*M_PI),
		rotation_speed * sin(-rel_y*M_PI/2.f),
		rotation_speed * -cos(rel_x*2*M_PI)
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

#include <unistd.h>

#define SMA_BUFSIZE 16

static double fps_sma(uint32_t t) {
	static uint32_t frametimes[SMA_BUFSIZE];
	static uint32_t frameptr = 0;

	frametimes[frameptr] = t;
	frameptr = (frameptr + 1) % SMA_BUFSIZE;

	uint32_t sum = 0;
	for (unsigned i = 0; i < SMA_BUFSIZE; i++) {
		sum += frametimes[(frameptr + i) % SMA_BUFSIZE];
	}

	return 1.f/(sum / (float)SMA_BUFSIZE / 1000.f);
}

int main(int argc, char *argv[]) {
	context ctx("grend test");
	std::unique_ptr<engine> scene(new testscene());

	while (scene->running) {
		uint32_t a = SDL_GetTicks();
		scene->input(ctx);

		if (scene->running) {
			scene->logic(ctx);
			scene->render(ctx);

			double fps = fps_sma(SDL_GetTicks() - a);
			printf(" %0.2f FPS (%0.2fms/frame)    \r", fps, 1.f/fps * 1000);
			fflush(stdout);
		}
	}

	return 0;
}
