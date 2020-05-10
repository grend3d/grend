#include <grend/main_logic.hpp>

// TODO: move timing stuff to its own file, maybe have a profiler class...
#include <unistd.h>

#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/types.h>

#include <glm/gtx/rotate_vector.hpp>

#define IMGUI_IMPL_OPENGL_LOADER_GLEW
#include <imgui/imgui.h>
#include <imgui/examples/imgui_impl_sdl.h>
#include <imgui/examples/imgui_impl_opengl3.h>

using namespace grendx;

void camera::set_direction(glm::vec3 dir) {
	direction = glm::normalize(dir);
	right = glm::normalize(glm::cross(glm::vec3(0, 1, 0), direction));
	up    = glm::normalize(glm::cross(direction, right));
}

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

static model_map test_models = {
	{"person",       model("assets/obj/low-poly-character-rpg/boy.obj")},
	{"teapot",       model("assets/obj/teapot.obj")},
	{"smoothteapot", model("assets/obj/smooth-teapot.obj")},
	{"monkey",       model("assets/obj/suzanne.obj")},
	{"smoothmonkey", model("assets/obj/smooth-suzanne.obj")},
	{"sphere",       model("assets/obj/sphere.obj")},
	{"smoothsphere", model("assets/obj/smoothsphere.obj")},
	{"glasssphere",  model("assets/obj/smoothsphere.obj")},
	{"steelsphere",  model("assets/obj/smoothsphere.obj")},
	{"earthsphere",  model("assets/obj/smoothsphere.obj")},
	{"dragon",       model("assets/obj/tests/dragon.obj")},
	{"maptest",      model("assets/obj/tests/maptest.obj")},
	{"building",     model("assets/obj/tests/building_test/building_test.obj")},
	{"unit_cube",        generate_cuboid(1, 1, 1)},
	//{"unit_cube_wood",   generate_cuboid(1, 1, 1)},
	{"unit_cube_ground", generate_cuboid(1, 1, 1)},
	//{"grid",             generate_grid(-32, -32, 32, 32, 4)},
};

static std::list<std::string> test_libraries = {
	/*
	   "assets/obj/Modular Terrain Cliff/",
	   "assets/obj/Modular Terrain Hilly/",
	   "assets/obj/Modular Terrain Beach/",
	   */
	//"assets/obj/Dungeon Set 2/",
};

void game_state::load_models(void) {
	model_map models = test_models;
	std::list<std::string> libraries = test_libraries;

	model_map gltf = load_gltf_models("assets/obj/Duck/glTF/Duck.gltf");
	models.insert(gltf.begin(), gltf.end());

	gltf = load_gltf_models("assets/obj/tests/AnimatedMorphCube/glTF/AnimatedMorphCube.gltf");
	models.insert(gltf.begin(), gltf.end());

	gltf = load_gltf_models("assets/obj/tests/DamagedHelmet/glTF/DamagedHelmet.gltf");
	models.insert(gltf.begin(), gltf.end());

	auto [scene, gmodels] = load_gltf_scene("assets/obj/tests/test_objects.gltf");
	static_models = scene;
	models.insert(gmodels.begin(), gmodels.end());
		/*
	gltf = load_gltf_models("assets/obj/tests/test_objects.gltf");
	models.insert(gltf.begin(), gltf.end());
	*/

	gltf = load_gltf_models("assets/obj/tests/donut4.gltf");
	models.insert(gltf.begin(), gltf.end());

	for (std::string libname : libraries) {
		// inserting each library into the models map so that way
		// we can access them from the editor, but could also do another
		// compile_models() call before bind_cooked_meshes to load the models.
		auto library = load_library(libname);
		models.insert(library.begin(), library.end());
	}

	models["teapot"].meshes["default:(null)"].material = "Steel";
	//models["grid"].meshes["default"].material = "Gravel";
	//models["monkey"].meshes["Monkey"].material = "Wood";
	models["glasssphere"].meshes["Sphere:None"].material = "Glass";
	models["steelsphere"].meshes["Sphere:None"].material = "Steel";
	models["earthsphere"].meshes["Sphere:None"].material = "Earth";

	// TODO: octree for static models
	for (auto& node : static_models.nodes) {
		//static_octree.add_model(models[node.name], node.transform);
		phys.add_static_model(node.name, models[node.name], node.transform);
	}

	std::cerr << " # generated octree with " << oct.count_nodes() << " nodes\n";

	glman.compile_models(models);
	glman.bind_cooked_meshes();
	editor.update_models(this);
}

void game_state::load_shaders(void) {
	gl_manager::rhandle vertex_shader, fragment_shader;
	vertex_shader = glman.load_shader("shaders/out/skybox.vert", GL_VERTEX_SHADER);
	fragment_shader = glman.load_shader("shaders/out/skybox.frag", GL_FRAGMENT_SHADER);
	skybox_shader = glman.gen_program();

	glAttachShader(skybox_shader.first, vertex_shader.first);
	glAttachShader(skybox_shader.first, fragment_shader.first);
	DO_ERROR_CHECK();

	glBindAttribLocation(skybox_shader.first, 0, "v_position");
	DO_ERROR_CHECK();

	// TODO: function to compile shaders
	int linked_2;
	glLinkProgram(skybox_shader.first);
	glGetProgramiv(skybox_shader.first, GL_LINK_STATUS, &linked_2);

	if (!linked_2) {
		SDL_Die("couldn't link shaders (skybox)");
	}

#if 0
	vertex_shader = glman.load_shader("shaders/out/vertex-shading.vert", GL_VERTEX_SHADER);
	fragment_shader = glman.load_shader("shaders/out/vertex-shading.frag", GL_FRAGMENT_SHADER);
#else
	vertex_shader = glman.load_shader("shaders/out/pixel-shading.vert", GL_VERTEX_SHADER);
	//fragment_shader = glman.load_shader("shaders/out/pixel-shading.frag", GL_FRAGMENT_SHADER);
	fragment_shader = glman.load_shader("shaders/out/pixel-shading-metal-roughness-pbr.frag", GL_FRAGMENT_SHADER);
#endif

	main_shader = glman.gen_program();

	glAttachShader(main_shader.first, vertex_shader.first);
	glAttachShader(main_shader.first, fragment_shader.first);
	DO_ERROR_CHECK();

	// monkey business
	fprintf(stderr, " # have %lu vertices\n", glman.cooked_vertprops.size());
	glBindAttribLocation(main_shader.first, 0, "in_Position");
	glBindAttribLocation(main_shader.first, 1, "v_normal");
	glBindAttribLocation(main_shader.first, 2, "v_tangent");
	glBindAttribLocation(main_shader.first, 3, "v_bitangent");
	glBindAttribLocation(main_shader.first, 4, "texcoord");
	DO_ERROR_CHECK();

	int linked;
	glLinkProgram(main_shader.first);
	glGetProgramiv(main_shader.first, GL_LINK_STATUS, &linked);

	if (!linked) {
		SDL_Die("couldn't link shaders (shading)");
	}

	gl_manager::rhandle orig_vao = glman.current_vao;
	glman.bind_vao(glman.screenquad_vao);
	vertex_shader = glman.load_shader("shaders/out/postprocess.vert", GL_VERTEX_SHADER);
	fragment_shader = glman.load_shader("shaders/out/postprocess.frag", GL_FRAGMENT_SHADER);
	post_shader = glman.gen_program();

	glAttachShader(post_shader.first, vertex_shader.first);
	glAttachShader(post_shader.first, fragment_shader.first);
	glBindAttribLocation(post_shader.first, 0, "v_position");
	glBindAttribLocation(post_shader.first, 1, "v_texcoord");
	DO_ERROR_CHECK();

	//glBindAttribLocation(post_shader.first, glman.screenquad_vbo.first, "screenquad");
	DO_ERROR_CHECK();

	glLinkProgram(post_shader.first);
	glGetProgramiv(post_shader.first, GL_LINK_STATUS, &linked);

	if (!linked) {
		SDL_Die("couldn't link shaders (postprocess)");
	}

	//glUseProgram(shader.first);
	glman.bind_vao(orig_vao);
	set_shader(main_shader);

	// TODO: shader class with uniform location class
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

void game_state::init_framebuffers(void) {
	// set up the render framebuffer
	rend_fb = glman.gen_framebuffer();
	rend_x = screen_x, rend_y = screen_y;

	glman.bind_framebuffer(rend_fb);
	rend_tex = glman.fb_attach_texture(GL_COLOR_ATTACHMENT0,
	                 glman.gen_texture_color(rend_x, rend_y, GL_RGBA16F));
	rend_depth = glman.fb_attach_texture(GL_DEPTH_STENCIL_ATTACHMENT,
	                 glman.gen_texture_depth_stencil(rend_x, rend_y));

	// and framebuffer holding the previously drawn frame
	last_frame_fb = glman.gen_framebuffer();
	glman.bind_framebuffer(last_frame_fb);
	last_frame_tex = glman.fb_attach_texture(GL_COLOR_ATTACHMENT0,
	                       glman.gen_texture_color(rend_x, rend_y));
}

void game_state::init_test_lights(void) {
	// TODO: assert() + logger
	player_light = add_light((struct engine::light){
		.position = {0, 7, 0, 1},
		.diffuse  = {1.0, 0.9, 0.8, 0.0},
		.radius = 0.2,
		.intensity = 100.0,
	});

	add_light((struct engine::light){
		.position = {0, 7, -8, 1},
		.diffuse  = {1.0, 0.8, 0.5, 1.0},
		.radius = 0.2,
		.intensity = 100.0,
	});

	add_light((struct engine::light){
		.position = {0, 7, 8, 1},
		.diffuse  = {1.0, 0.8, 0.5, 1.0},
		.radius = 0.2,
		.intensity = 100.0,
	});

	add_light((struct engine::light){
		.position = {0, 30, 50, 0},
		.diffuse  = {0.9, 0.9, 1.0, 0.1},
		.radius = 3.0,
		.intensity = 100.0,
	});

	update_lights();
}

void game_state::init_imgui(context& ctx) {
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGui::StyleColorsDark();
	ImGui_ImplSDL2_InitForOpenGL(ctx.window, ctx.glcontext);
	// TODO: make the glsl version here depend on GL version/the string in
	//       shaders/version.glsl
	ImGui_ImplOpenGL3_Init("#version 130");
}

// TODO: should start thinking about splitting initialization into smaller functions
game_state::game_state(context& ctx) : engine(), text(this) {
	projection = glm::perspective(glm::radians(60.f),
	                             (1.f*SCREEN_SIZE_X)/SCREEN_SIZE_Y, 0.1f, 100.f);
	view = glm::lookAt(glm::vec3(0.0, 0.0, 0.0),
	                   glm::vec3(0.0, 0.0, 1.0),
	                   glm::vec3(0.0, 1.0, 0.0));

	SDL_GetWindowSize(ctx.window, &screen_x, &screen_y);

	//skybox = glman.load_cubemap("assets/tex/cubes/LancellottiChapel/");
	skybox = glman.load_cubemap("assets/tex/cubes/rocky-skyboxes/Skinnarviksberget/");
	//skybox = glman.load_cubemap("assets/tex/cubes/rocky-skyboxes/Skinnarviksberget-tiny/", ".png");
	//skybox = glman.load_cubemap("assets/tex/cubes/rocky-skyboxes/Tantolunden6/");

	load_models();
	load_shaders();
	init_framebuffers();
	init_test_lights();
	init_imgui(ctx);

	// TODO: fit the player
	player_phys_id = phys.add_sphere("person", {0, 3, 0}, 1);

	glEnable(GL_DEPTH_TEST);
	//glEnable(GL_FRAMEBUFFER_SRGB);
	glDepthFunc(GL_LESS);
	DO_ERROR_CHECK();

	glman.bind_default_framebuffer();
	DO_ERROR_CHECK();
}

game_state::~game_state() {
	puts("got here");
}

void game_state::draw_octree_leaves(octree::node *node, glm::vec3 location) {
	if (node == nullptr) {
		return;
	}

	else if (node->level == 0) {
		//glm::mat4 trans = glm::translate(glm::scale(glm::vec3(0.1)), location);
		double scale = oct.leaf_size * (1 << (node->level + 1));
		glm::mat4 trans = glm::scale(glm::translate(location*0.5f), glm::vec3(scale));
		//draw_model_lines("unit_cube", trans);
		draw_model("unit_cube", trans);
	}

	else {
		/*
		double scale = oct.leaf_size * (1 << node->level + 1);
		glm::mat4 trans = glm::scale(glm::translate(location*0.5f), glm::vec3(scale));
		draw_model_lines("unit_cube", trans);
		*/

		for (unsigned i = 0; i < 8; i++) {
			bool x = i&1;
			bool y = i&2;
			bool z = i&4;

			glm::vec3 temp = location;
			temp.x -= (x?1:-1) * oct.leaf_size * (1 << node->level);
			temp.y -= (y?1:-1) * oct.leaf_size * (1 << node->level);
			temp.z -= (z?1:-1) * oct.leaf_size * (1 << node->level);

			draw_octree_leaves(node->subnodes[x][y][z], temp);
		}
	}
}

void game_state::render_skybox(context& ctx) {
	set_shader(skybox_shader);
	glDepthMask(GL_FALSE);
	glDepthFunc(GL_LEQUAL);
#ifdef ENABLE_FACE_CULLING
	// TODO: toggle per-model
	glDisable(GL_CULL_FACE);
#endif

	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_CUBE_MAP, skybox.first);
	glUniform1i(glGetUniformLocation(shader.first, "skytexture"), 4);
	DO_ERROR_CHECK();
	set_mvp(glm::mat4(0), glm::mat4(glm::mat3(view)), projection);
	//draw_model("unit_cube", glm::mat4(1));
	draw_mesh("unit_cube.default", glm::mat4(0));
	glDepthMask(GL_TRUE);
}

void game_state::render_static(context& ctx) {
	for (auto& thing : static_models.nodes) {
		//draw_model(thing.name, thing.transform);
		dqueue_draw_model(thing.name, thing.transform);
	}
}

void game_state::render_players(context& ctx) {
	/*
	glm::vec3 hpos = glm::vec3(view_direction.x*3 + view_position.x, 0, view_direction.z*3 + view_position.z);
	glm::mat4 bizz =
		glm::translate(glm::mat4(1), hpos)
		* glm::scale(glm::vec3(0.75f, 0.75f, 0.75f));
	 */

	//glm::vec3 asdf = glm::normalize(glm::cross(player_direction, glm::vec3(0, 1, 0)));

	auto& physobj = phys.objects[player_phys_id];

	glm::vec3 tempdir = glm::normalize(
		glm::vec3(physobj.velocity.x, 0, physobj.velocity.z));

	if (fabs(tempdir.x) > 0 || fabs(tempdir.z) > 0) {
		player_direction = tempdir;
	}

	glm::mat4 bizz =
		glm::translate(glm::mat4(1), physobj.position)
		* glm::orientation(player_direction, glm::vec3(0, 0, 1))
		* glm::scale(glm::vec3(0.75f, 0.75f, 0.75f))
		;

	/*
	glUniform4fv(glGetUniformLocation(shader.first, "lightpos"),
			1, glm::value_ptr(lfoo));
			*/

	dqueue_draw_model("person", bizz);
	/*
	//draw_model("person", bizz);
	//draw_model_lines("sphere", glm::translate(bizz, {0, 1, 0}));
	DO_ERROR_CHECK();
	*/
}

void game_state::render_dynamic(context& ctx) {
	/*
	for (const auto& thing : phys_objs) {
		glm::mat4 transform = glm::translate(glm::mat4(1), thing.position);
		dqueue_draw_model(thing.model_name, transform);
	}
	*/

	for (const auto& [id, obj] : phys.objects) {
		if (id == player_phys_id)
			continue;

		if (obj.type != imp_physics::object::type::Static) {
			glm::mat4 transform =
				glm::translate(obj.position) * glm::mat4_cast(obj.rotation);

			dqueue_draw_model(obj.model_name, transform);
		}
	}
}

void game_state::render_postprocess(context& ctx) {
	glman.bind_default_framebuffer();
	glman.bind_vao(glman.screenquad_vao);
	glViewport(0, 0, screen_x, screen_y);
	set_shader(post_shader);

	glClearColor(0, 0, 0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT);
	glDepthMask(GL_FALSE);
	glDisable(GL_DEPTH_TEST);
	DO_ERROR_CHECK();

	glActiveTexture(GL_TEXTURE6);
	glBindTexture(GL_TEXTURE_2D, rend_tex.first);
	glActiveTexture(GL_TEXTURE7);
	glBindTexture(GL_TEXTURE_2D, rend_depth.first);
	glActiveTexture(GL_TEXTURE8);
	glBindTexture(GL_TEXTURE_2D, last_frame_tex.first);
	glUniform1i(glGetUniformLocation(post_shader.first, "render_fb"), 6);
	glUniform1i(glGetUniformLocation(post_shader.first, "render_depth"), 7);
	glUniform1i(glGetUniformLocation(post_shader.first, "last_frame_fb"), 8);

	// TODO: vec2
	glUniform1f(glGetUniformLocation(post_shader.first, "scale_x"),
		(round(dsr_scale_x*rend_x))/rend_x);
	glUniform1f(glGetUniformLocation(post_shader.first, "scale_y"),
		(round(dsr_scale_y*rend_y))/rend_y);

	glUniform1f(glGetUniformLocation(post_shader.first, "screen_x"), screen_x);
	glUniform1f(glGetUniformLocation(post_shader.first, "screen_y"), screen_y);

	// TODO: some sort of global variable lookup
	glUniform1f(glGetUniformLocation(post_shader.first, "exposure"), editor.exposure);

	draw_screenquad();

	/*
	// TODO: this ends up taking 100% CPU while running...
	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, last_frame_fb.first);
	glBlitFramebuffer(0, 0, screen_x, screen_y,
	                  0, 0, rend_x, rend_y,
	                  GL_COLOR_BUFFER_BIT, GL_NEAREST);
	DO_ERROR_CHECK();
	*/
}

/*
void game_state::update_dynamic_lights(context& ctx) {
	// light that follows the player
	float asdf = 0.5*(SDL_GetTicks()/1000.f);
	glm::vec4 lfoo = glm::vec4(15*cos(asdf), 5, 15*sin(asdf), 1.f);
	//glm::vec4 lfoo = glm::vec4(hpos.x, 5, hpos.z, 1.f);
	struct engine::light lit;

	get_light(player_light, &lit);
	//lit.position = lfoo;
	lit.position = glm::vec4(hpos.x, hpos.y + 4, hpos.z, 1.f);
	set_light(player_light, lit);
	update_lights();
}
*/

void game_state::render(context& ctx) {
	// TODO: if postprocessing enabled, do this, otherwise just keep
	//       the default framebuffer
	{
		glman.bind_framebuffer(rend_fb);
		glViewport(0, 0, round(rend_x*dsr_scale_x), round(rend_y*dsr_scale_y));
	}

	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);
	//glEnable(GL_FRAMEBUFFER_SRGB);
	glDepthFunc(GL_LESS);

#if 0
	// TODO: debug flag to toggle checks like this
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		SDL_Die("incomplete!");
	}
#endif

	//glClearColor(0.7, 0.9, 1, 1);
	glClearColor(0.1, 0.1, 0.1, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	set_shader(main_shader);
#ifdef ENABLE_FACE_CULLING
	// TODO: toggle per-model
	glEnable(GL_CULL_FACE);
	// make sure we always start with counter-clockwise faces
	// (also should be toggled per-model)
	glFrontFace(GL_CCW);
#endif
	DO_ERROR_CHECK();

	glUniform1i(glGetUniformLocation(shader.first, "skytexture"), 4);
	glUniform1f(glGetUniformLocation(shader.first, "time_ms"),
	                                 SDL_GetTicks() * 1.f);

	set_mvp(glm::mat4(1), view, projection);
	render_static(ctx);
	render_players(ctx);
	render_dynamic(ctx);
	editor.render_editor(this, ctx);

	assert(current_cam != nullptr);
	dqueue_sort_draws(current_cam->position);
	dqueue_flush_draws();

	render_skybox(ctx);
	render_postprocess(ctx);

	glman.bind_default_framebuffer();
	text.render({0.0, 0.9, 0},
			"scale x: " + std::to_string(dsr_scale_x) +
			", y: " + std::to_string(dsr_scale_y));

	text.render({-0.9, 0.9, 0}, "grend test v0");

	/*
	if (in_select_mode) {
		render_imgui(ctx);
	}
	*/
	if (editor.mode != game_editor::mode::Inactive) {
		editor.render_imgui(this, ctx);
	}
}

void game_state::draw_debug_string(std::string str) {
	glDisable(GL_DEPTH_TEST);
	text.render({-0.9, -0.9, 0}, str);
	DO_ERROR_CHECK();
}

// TODO: variable configuration ala quake
static const float movement_speed = 10 /* units/s */;
static const float rotation_speed = 1;

void game_state::handle_player_input(SDL_Event& ev) {
	if (ev.type == SDL_KEYDOWN) {
		switch (ev.key.keysym.sym) {
			case SDLK_w: player_move_input.z =  movement_speed; break;
			case SDLK_s: player_move_input.z = -movement_speed; break;
			case SDLK_a: player_move_input.x = -movement_speed; break;
			case SDLK_d: player_move_input.x =  movement_speed; break;
			case SDLK_q: player_move_input.y =  movement_speed; break;
			case SDLK_e: player_move_input.y = -movement_speed; break;
			case SDLK_SPACE: player_move_input.y += 5 /* m/s */; break;
			//case SDLK_m: in_select_mode = !in_select_mode; break;
			case SDLK_m: editor.set_mode(game_editor::mode::Map); break;
			//case SDLK_BACKSPACE: player_position = {0, 2, 0}; break;

			case SDLK_LEFTBRACKET: dsr_scale_x -= dsr_down_incr; break;
			case SDLK_RIGHTBRACKET: dsr_scale_x += dsr_down_incr; break;
			case SDLK_9: dsr_scale_y -= dsr_down_incr; break;
			case SDLK_0: dsr_scale_y += dsr_down_incr; break;

			case SDLK_ESCAPE: running = false; break;
		}
	}

	else if (ev.type == SDL_KEYUP) {
		switch (ev.key.keysym.sym) {
			case SDLK_w:
			case SDLK_s:
				//view_velocity.z = 0;
				player_move_input.z = 0;
				break;

			case SDLK_a:
			case SDLK_d:
				//view_velocity.x = 0;
				player_move_input.x = 0;
				break;

			case SDLK_q:
			case SDLK_e:
				//view_velocity.y = 0;
				player_move_input.y = 0;
				break;
		}
	}

	else if (ev.type == SDL_MOUSEBUTTONDOWN) {
		if (ev.button.button == SDL_BUTTON_LEFT) {
			/*
			phys_objs.push_back({
				"steelsphere",
				player_position,
				view_direction*movement_speed,
			});
			*/

			uint64_t id = phys.add_sphere("steelsphere", phys.objects[player_phys_id].position, 1);
			phys.objects[id].velocity = player_direction*movement_speed;
			// TODO: initial velocity
		}
	}
}

void game_state::input(context& ctx) {
	Uint32 cur_ticks = SDL_GetTicks();
	Uint32 ticks_delta = cur_ticks - last_frame;
	float fticks = ticks_delta / 1000.0f;
	SDL_Event ev;

	while (SDL_PollEvent(&ev)) {
		if (ev.type == SDL_QUIT) {
			running = false;
		}

		else if (ev.type == SDL_WINDOWEVENT) {
			switch (ev.window.event) {
				case SDL_WINDOWEVENT_RESIZED:
				case SDL_WINDOWEVENT_SIZE_CHANGED:
					{
						SDL_GetWindowSize(ctx.window, &screen_x, &screen_y);
						// TODO: maybe reallocate framebuffers
						//int win_x, win_y;
						//glViewport(0, 0, win_x, win_y);
						/*
						projection = glm::perspective(glm::radians(60.f),
						                              (1.f*win_x)/win_y, 0.1f, 100.f);
													  */
					}
					break;
			}
		}

		if (editor.mode != game_editor::mode::Inactive) {
			editor.handle_editor_input(this, ctx, ev);
		} else {
			handle_player_input(ev);
		}
	}

	int x, y;
	int win_x, win_y;
	Uint32 buttons = SDL_GetMouseState(&x, &y);
	buttons = buttons; // XXX: make the compiler shut up about the unused variable
	SDL_GetWindowSize(ctx.window, &win_x, &win_y);

	x = (x > 0)? x : win_x/2;
	y = (x > 0)? y : win_y/2;

	// TODO: get window resolution
	float center_x = (float)win_x / 2;
	float center_y = (float)win_y / 2;

	float rel_x = ((float)x - center_x) / center_x;
	float rel_y = ((float)y - center_y) / center_y;

	player_cam.set_direction(glm::vec3(
		rotation_speed * sin(rel_x*2*M_PI),
		rotation_speed * sin(-rel_y*M_PI/2.f),
		rotation_speed * -cos(rel_x*2*M_PI)
	));
}

void game_state::physics(context& ctx) {
	float delta = (SDL_GetTicks() - last_frame)/1000.f;

	glm::vec3 rel_view =
		glm::normalize(glm::vec3(player_cam.direction.x, 0,
		                         player_cam.direction.z));

	glm::vec3 player_right = glm::normalize(glm::cross(glm::vec3(0, 1, 0), rel_view));
	glm::vec3 player_up    = glm::normalize(glm::cross(rel_view, player_right));

	glm::vec3 player_force = {0, 0, 0};
	player_force += player_move_input.z * rel_view;
	player_force += player_move_input.y * player_up;
	player_force += player_move_input.x * glm::normalize(glm::cross(rel_view, player_up));

	phys.apply_force(player_phys_id, player_force);
	phys.solve_contraints(delta);
}

void game_state::logic(context& ctx) {
	Uint32 cur_ticks = SDL_GetTicks();
	Uint32 ticks_delta = cur_ticks - last_frame;
	float fticks = ticks_delta / 1000.0f;
	last_frame = cur_ticks;

	editor.logic(ctx, fticks);
	current_cam = editor.mode? &editor.cam : &player_cam;

	glm::vec3 player_position = phys.objects[player_phys_id].position;
	player_cam.position = player_position - player_cam.direction*5.f;

	assert(current_cam != nullptr);
	view = glm::lookAt(current_cam->position,
	                   current_cam->position + current_cam->direction,
	                   current_cam->up);

	adjust_draw_resolution();
}

void game_state::adjust_draw_resolution(void) {
	// TODO: non-linear adjustment based on frame time
	double frametime = 1.0/frame_timer.average() * 1000;

	if (frametime > dsr_scale_down) {
		if (dsr_scale_x <= dsr_min_scale_x) {
			dsr_scale_y = max(dsr_min_scale_y, dsr_scale_y - dsr_down_incr);

		} else {
			dsr_scale_x = max(dsr_min_scale_x, dsr_scale_x - dsr_down_incr);
		}
	}

	else if (frametime < dsr_scale_up) {
		if (dsr_scale_y >= 1.0) {
			dsr_scale_x = min(1.0, dsr_scale_x + dsr_up_incr);

		} else {
			dsr_scale_y = min(1.0, dsr_scale_y + dsr_up_incr);
		}
	}
}
