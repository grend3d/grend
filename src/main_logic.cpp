#include <grend/main_logic.hpp>

// TODO: move timing stuff to its own file, maybe have a profiler class...
#include <unistd.h>

#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/types.h>

#include <glm/gtx/rotate_vector.hpp>

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
	{"unit_cube_wood",   generate_cuboid(1, 1, 1)},
	{"unit_cube_ground", generate_cuboid(1, 1, 1)},
	{"grid",             generate_grid(-32, -32, 32, 32, 4)},
};

static std::list<std::string> test_libraries = {
	/*
	   "assets/obj/Modular Terrain Cliff/",
	   "assets/obj/Modular Terrain Hilly/",
	   "assets/obj/Modular Terrain Beach/",
	   */
	//"assets/obj/Dungeon Set 2/",
};

void testscene::load_models(void) {
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
	models["grid"].meshes["default"].material = "Gravel";
	models["monkey"].meshes["Monkey"].material = "Wood";
	models["glasssphere"].meshes["Sphere:None"].material = "Glass";
	models["steelsphere"].meshes["Sphere:None"].material = "Steel";
	models["earthsphere"].meshes["Sphere:None"].material = "Earth";

	// TODO: octree for static models
	for (auto& node : static_models.nodes) {
		for (auto& meshkey : models[node.name].meshes) {
			auto& verts = models[node.name].vertices;
			auto& mesh = meshkey.second;

			for (unsigned i = 0; i < mesh.faces.size(); i += 3) {
				glm::vec3 tri[3] = {
					verts[mesh.faces[i]],
					verts[mesh.faces[i+1]],
					verts[mesh.faces[i+2]]
				};

				for (auto& t : tri) {
					glm::vec4 m = node.transform * glm::vec4(t, 1);
					t = glm::vec3(m) / m.w;
				}

				static_octree.add_tri(tri);
			}
		}
	}

	std::cerr << " # generated octree with " << oct.count_nodes() << " nodes\n";

	glman.compile_models(models);
	glman.bind_cooked_meshes();
	select_model = glman.cooked_models.begin();
}

void testscene::load_shaders(void) {
	gl_manager::rhandle vertex_shader, fragment_shader;
	vertex_shader = glman.load_shader("shaders/skybox.vert", GL_VERTEX_SHADER);
	fragment_shader = glman.load_shader("shaders/skybox.frag", GL_FRAGMENT_SHADER);
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
		SDL_Die("couldn't link shaders");
	}

#if 0
	vertex_shader = glman.load_shader("shaders/vertex-shading.vert", GL_VERTEX_SHADER);
	fragment_shader = glman.load_shader("shaders/vertex-shading.frag", GL_FRAGMENT_SHADER);
#else
	vertex_shader = glman.load_shader("shaders/pixel-shading.vert", GL_VERTEX_SHADER);
	//fragment_shader = glman.load_shader("shaders/pixel-shading.frag", GL_FRAGMENT_SHADER);
	fragment_shader = glman.load_shader("shaders/pixel-shading-metal-roughness-pbr.frag", GL_FRAGMENT_SHADER);
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
		SDL_Die("couldn't link shaders");
	}

	gl_manager::rhandle orig_vao = glman.current_vao;
	glman.bind_vao(glman.screenquad_vao);
	vertex_shader = glman.load_shader("shaders/postprocess.vert", GL_VERTEX_SHADER);
	fragment_shader = glman.load_shader("shaders/postprocess.frag", GL_FRAGMENT_SHADER);
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
		SDL_Die("couldn't link shaders");
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

void testscene::init_framebuffers(void) {
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

void testscene::init_test_lights(void) {
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

	add_light((struct engine::light){
		.position = {0, 30, 50, 0},
		.diffuse  = {0.9, 0.9, 1.0, 1.0},
		.const_attenuation = 1.f,
		.linear_attenuation = 0.f,
		.quadratic_attenuation = 0.00f,
		.specular = 1.0,
	});

	update_lights();
}

// TODO: should start thinking about splitting initialization into smaller functions
testscene::testscene(context& ctx) : engine(), text(this) {
	projection = glm::perspective(glm::radians(60.f),
	                             (1.f*SCREEN_SIZE_X)/SCREEN_SIZE_Y, 0.1f, 100.f);
	view = glm::lookAt(glm::vec3(0.0, 0.0, 0.0),
	                   glm::vec3(0.0, 0.0, 1.0),
	                   glm::vec3(0.0, 1.0, 0.0));

	SDL_GetWindowSize(ctx.window, &screen_x, &screen_y);

	//skybox = glman.load_cubemap("assets/tex/cubes/LancellottiChapel/");
	skybox = glman.load_cubemap("assets/tex/cubes/rocky-skyboxes/Skinnarviksberget/");
	//skybox = glman.load_cubemap("assets/tex/cubes/rocky-skyboxes/Tantolunden6/");

	load_models();
	load_shaders();
	init_framebuffers();
	init_test_lights();

	glEnable(GL_DEPTH_TEST);
	//glEnable(GL_FRAMEBUFFER_SRGB);
	glDepthFunc(GL_LESS);
	DO_ERROR_CHECK();

	glman.bind_default_framebuffer();
	DO_ERROR_CHECK();
}

testscene::~testscene() {
	puts("got here");
}

void testscene::draw_octree_leaves(octree::node *node, glm::vec3 location) {
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

void testscene::render_skybox(context& ctx) {
	set_shader(skybox_shader);
	glDepthMask(GL_FALSE);
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

void testscene::render_static(context& ctx) {
	for (auto& thing : static_models.nodes) {
		draw_model(thing.name, thing.transform);
	}
}

void testscene::render_players(context& ctx) {
	/*
	glm::vec3 hpos = glm::vec3(view_direction.x*3 + view_position.x, 0, view_direction.z*3 + view_position.z);
	glm::mat4 bizz =
		glm::translate(glm::mat4(1), hpos)
		* glm::scale(glm::vec3(0.75f, 0.75f, 0.75f));
	 */

	//glm::vec3 asdf = glm::normalize(glm::cross(player_direction, glm::vec3(0, 1, 0)));

	glm::mat4 bizz =
		glm::translate(glm::mat4(1), player_position)
		* glm::orientation(player_direction, glm::vec3(0, 0, 1))
		* glm::scale(glm::vec3(0.75f, 0.75f, 0.75f))
		;

	/*
	glUniform4fv(glGetUniformLocation(shader.first, "lightpos"),
			1, glm::value_ptr(lfoo));
			*/

	set_mvp(glm::mat4(1), view, projection);
	draw_model("person", bizz);
	//draw_model_lines("sphere", glm::translate(bizz, {0, 1, 0}));
	DO_ERROR_CHECK();
}

void testscene::render_editor(context& ctx) {
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
}

void testscene::render_postprocess(context& ctx) {
	glman.bind_default_framebuffer();
	glman.bind_vao(glman.screenquad_vao);
	glViewport(0, 0, screen_x, screen_y);
	set_shader(post_shader);

	glClearColor(0, 0, 0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT);

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

	//glClearColor(1.0, 0, 0, 1.0);

	glDisable(GL_DEPTH_TEST);
	DO_ERROR_CHECK();
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
void testscene::update_dynamic_lights(context& ctx) {
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

void testscene::render(context& ctx) {
	// TODO: if postprocessing enabled, do this, otherwise just keep
	//       the default framebuffer
	{
		glman.bind_framebuffer(rend_fb);
		glViewport(0, 0, round(rend_x*dsr_scale_x), round(rend_y*dsr_scale_y));
	}

	glEnable(GL_DEPTH_TEST);
	//glEnable(GL_FRAMEBUFFER_SRGB);
	glDepthFunc(GL_LESS);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		SDL_Die("incomplete!");
	}

	//glClearColor(0.7, 0.9, 1, 1);
	glClearColor(0.1, 0.1, 0.1, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	render_skybox(ctx);

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

	//draw_model("testbox-mesh", glm::mat4(1));
	//draw_model("teapot", glm::mat4(1));
	//draw_octree_leaves(oct.root, glm::vec3(0));
	//draw_octree_leaves(static_octree.root, glm::vec3(0));

	render_static(ctx);
	render_players(ctx);
	render_editor(ctx);
	render_postprocess(ctx);

	glman.bind_default_framebuffer();
	text.render({0.0, 0.9, 0},
			"scale x: " + std::to_string(dsr_scale_x) +
			", y: " + std::to_string(dsr_scale_y));

	text.render({-0.9, 0.9, 0}, "grend test v0");
}

void testscene::draw_debug_string(std::string str) {
	glDisable(GL_DEPTH_TEST);
	text.render({-0.9, -0.9, 0}, str);
	DO_ERROR_CHECK();
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
				/*
				case SDLK_w: view_velocity.z =  movement_speed; break;
				case SDLK_s: view_velocity.z = -movement_speed; break;
				case SDLK_a: view_velocity.x = -movement_speed; break;
				case SDLK_d: view_velocity.x =  movement_speed; break;
				case SDLK_q: view_velocity.y =  movement_speed; break;
				case SDLK_e: view_velocity.y = -movement_speed; break;
				*/
				case SDLK_w: player_move_input.z =  movement_speed; break;
				case SDLK_s: player_move_input.z = -movement_speed; break;
				case SDLK_a: player_move_input.x = -movement_speed; break;
				case SDLK_d: player_move_input.x =  movement_speed; break;
				case SDLK_q: player_move_input.y =  movement_speed; break;
				case SDLK_e: player_move_input.y = -movement_speed; break;
				case SDLK_SPACE: player_move_input.y += 5 /* m/s */; break;
				case SDLK_m: in_select_mode = !in_select_mode; break;

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

		else if (ev.type == SDL_WINDOWEVENT) {
			switch (ev.window.event) {
				case SDL_WINDOWEVENT_RESIZED:
				case SDL_WINDOWEVENT_SIZE_CHANGED:
					{
						SDL_GetWindowSize(ctx.window, &screen_x, &screen_y);
						//std::cerr << " * resized window: " << win_x << ", " << win_y << std::endl;

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

		// keeping editing commands seperate from the main input handling
		if (in_select_mode) {
			if (ev.type == SDL_KEYDOWN) {
				switch (ev.key.keysym.sym) {
					case SDLK_i: load_map(); break;
					case SDLK_o: save_map(); break;

					case SDLK_g:
						//select_rotation -= M_PI/4;
						select_transform *= glm::rotate((float)-M_PI/4.f, glm::vec3(1, 0, 0));
						break;

					case SDLK_h:
						//select_rotation -= M_PI/4;
						select_transform *= glm::rotate((float)M_PI/4.f, glm::vec3(1, 0, 0));
						break;

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

					case SDLK_j:
						// scale down
						select_transform *= glm::scale(glm::vec3(0.9));
						break;

					case SDLK_k:
						// scale up
						select_transform *= glm::scale(glm::vec3(1/0.9));
						break;

					case SDLK_r:
						if (select_model == glman.cooked_models.begin()) {
							select_model = glman.cooked_models.end();
						}
						select_model--;
						break;

					case SDLK_f:
						select_model++;
						if (select_model == glman.cooked_models.end()) {
							select_model = glman.cooked_models.begin();
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
	buttons = buttons; // XXX: make the compiler shut up about the unused variable
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
	float delta = (SDL_GetTicks() - last_frame)/1000.f;

	glm::vec3 rel_view =
		glm::normalize(glm::vec3(view_direction.x, 0, view_direction.z));

	//player_direction = glm::normalize(glm::vec3(view_direction.x, 0, view_direction.z));
	//player_position += player_velocityplayer_direction*delta;
	//player_position += player_velocityplayer_direction*delta;
	glm::vec3 player_right = glm::normalize(glm::cross(glm::vec3(0, 1, 0), rel_view));
	glm::vec3 player_up    = glm::normalize(glm::cross(rel_view, player_right));

	//glm::vec3 incr = glm::vec3(0);
	//player_velocity = glm::vec3(0);
	player_velocity += player_move_input.z *delta* rel_view;
	player_velocity += player_move_input.y *delta* player_up;
	player_velocity += player_move_input.x *delta* glm::normalize(glm::cross(rel_view, player_up));

	/*
	if ((player_position + player_velocity*delta*2.f).y < 0) {
		player_move_input.y *= -0.5;

	} else if (player_position.y > 0) {
		player_move_input.y -= 9.81*delta;
	}
	*/

	glm::vec3 next_pos = player_position + player_velocity*delta*2.f;
	auto [collided, normal] = static_octree.collides(player_position, next_pos);

	if (collided) {
		player_position += normal*(float)static_octree.leaf_size;
		player_velocity.y *= -0.50;

	} else {
		player_velocity.y -= 9.81*delta;
	}

	player_position += player_velocity*delta;
	glm::vec3 tempdir = glm::normalize(
		glm::vec3(player_velocity.x, 0, player_velocity.z));

	if (fabs(tempdir.x) > 0 || fabs(tempdir.z) > 0) {
		player_direction = tempdir;
	}

	//player_direction = glm::normalize(glm::vec3(player_velocity.x, -0.1*fabs(glm::length(player_velocity)), player_velocity.z));
	//player_direction = glm::normalize(player_velocity);
}

void testscene::logic(context& ctx) {
	Uint32 cur_ticks = SDL_GetTicks();
	last_frame = cur_ticks;

	if (player_position.y < -25) {
		// TODO: player class, with position set/reset etc
		player_position = {0, 2, 0};
	}

	//player_direction = view_direction;

	//view = glm::lookAt(view_position, view_position + view_direction, view_up);
	view = glm::lookAt(player_position - view_direction*5.f, player_position, view_up);
	adjust_draw_resolution();
}

void testscene::adjust_draw_resolution(void) {
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

void testscene::save_map(std::string name) {
	std::ofstream foo(name);
	std::cerr << "saving map " << name << std::endl;

	if (!foo.good()) {
		std::cerr << "couldn't open save file" << name << std::endl;
		return;
	}

	foo << "### test scene save file" << std::endl;

	/*
	for (std::string& lib : libraries) {
		foo << "library\t" << lib << std::endl;
	}
	*/

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

