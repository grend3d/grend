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

	model_map gltf;
	/*
	std::cerr << "loading duck" << std::endl;
	model_map gltf = load_gltf_models("assets/obj/Duck/glTF/Duck.gltf");
	models.insert(gltf.begin(), gltf.end());

	std::cerr << "loading morphcube" << std::endl;
	gltf = load_gltf_models("assets/obj/tests/AnimatedMorphCube/glTF/AnimatedMorphCube.gltf");
	models.insert(gltf.begin(), gltf.end());
	*/

	std::cerr << "loading helmet" << std::endl;
	gltf = load_gltf_models("assets/obj/tests/DamagedHelmet/glTF/DamagedHelmet.gltf");
	models.insert(gltf.begin(), gltf.end());

	std::cerr << "loading test objects" << std::endl;
	auto [scene, gmodels] = load_gltf_scene("assets/obj/tests/test_objects.gltf");
	static_models = scene;
	models.insert(gmodels.begin(), gmodels.end());
		/*
	gltf = load_gltf_models("assets/obj/tests/test_objects.gltf");
	models.insert(gltf.begin(), gltf.end());
	*/

	std::cerr << "loading donut" << std::endl;
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
	std::cerr << "loading shaders" << std::endl;
	skybox_shader = glman.load_program(
		"shaders/out/skybox.vert",
		"shaders/out/skybox.frag"
	);

	glBindAttribLocation(skybox_shader.first, 0, "v_position");
	glman.link_program(skybox_shader);
	std::cerr << "loaded skybox shader" << std::endl;

#if 0
	main_shader = glman.load_program(
		"shaders/out/vertex-shading.vert",
		"shaders/out/vertex-shading.frag"
	);
#else
	main_shader = glman.load_program(
		"shaders/out/pixel-shading.vert",
		//"shaders/out/pixel-shading.frag",
		"shaders/out/pixel-shading-metal-roughness-pbr.frag"
	);
#endif

	glBindAttribLocation(main_shader.first, 0, "in_Position");
	glBindAttribLocation(main_shader.first, 1, "v_normal");
	glBindAttribLocation(main_shader.first, 2, "v_tangent");
	glBindAttribLocation(main_shader.first, 3, "v_bitangent");
	glBindAttribLocation(main_shader.first, 4, "texcoord");
	glman.link_program(main_shader);
	std::cerr << "loaded main shader" << std::endl;

	refprobe_shader = glman.load_program(
		"shaders/out/ref_probe.vert",
		"shaders/out/ref_probe.frag"
	);

	glBindAttribLocation(refprobe_shader.first, 0, "in_Position");
	glBindAttribLocation(refprobe_shader.first, 1, "v_normal");
	glBindAttribLocation(refprobe_shader.first, 2, "v_tangent");
	glBindAttribLocation(refprobe_shader.first, 3, "v_bitangent");
	glBindAttribLocation(refprobe_shader.first, 4, "texcoord");
	glman.link_program(refprobe_shader);
	std::cerr << "loaded refprobe shader" << std::endl;

	refprobe_debug = glman.load_program(
		"shaders/out/ref_probe_debug.vert",
		"shaders/out/ref_probe_debug.frag"
	);

	glBindAttribLocation(refprobe_debug.first, 0, "in_Position");
	glBindAttribLocation(refprobe_debug.first, 1, "v_normal");
	glBindAttribLocation(refprobe_debug.first, 2, "v_tangent");
	glBindAttribLocation(refprobe_debug.first, 3, "v_bitangent");
	glBindAttribLocation(refprobe_debug.first, 4, "texcoord");
	glman.link_program(refprobe_debug);
	std::cerr << "loaded refprobe debug shader" << std::endl;

	gl_manager::rhandle orig_vao = glman.current_vao;
	glman.bind_vao(glman.screenquad_vao);

	post_shader = glman.load_program(
		"shaders/out/postprocess.vert",
		"shaders/out/postprocess.frag"
	);

	glBindAttribLocation(post_shader.first, 0, "v_position");
	glBindAttribLocation(post_shader.first, 1, "v_texcoord");
	DO_ERROR_CHECK();
	glman.link_program(post_shader);

	std::cerr << "loaded postprocess shader" << std::endl;

	//glUseProgram(shader.first);
	glman.bind_vao(orig_vao);
	set_shader(main_shader);
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
	player_light = add_light((struct engine::point_light){
		.position = {0, 7, -8},
		.diffuse  = {1.0, 0.8, 0.5, 1.0},
		.radius = 0.2,
		.intensity = 100.0,
	});

	add_light((struct engine::point_light){
		.position = {0, 7, 8},
		.diffuse  = {1.0, 0.8, 0.5, 1.0},
		.radius = 0.2,
		.intensity = 100.0,
	});

	add_light((struct engine::spot_light){
		.position = {0, 7, 0},
		.diffuse  = {1.0, 0.9, 0.8, 1.0},
		.direction = {0, 1, 0},
		.radius = 0.2,
		.intensity = 100.0,
		.angle = 0.7,
	});

	add_light((struct engine::directional_light){
		.position = {0, 30, 50},
		.diffuse  = {0.9, 0.9, 1.0, 0.1},
		.direction = {0, 0, 1},
		.intensity = 100.0,
	});
}

void game_state::init_imgui(context& ctx) {
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGui::StyleColorsDark();
	ImGui_ImplSDL2_InitForOpenGL(ctx.window, ctx.glcontext);
	// TODO: make the glsl version here depend on GL version/the string in
	//       shaders/version.glsl
	//ImGui_ImplOpenGL3_Init("#version 130");
	//ImGui_ImplOpenGL3_Init("#version 300 es");
	ImGui_ImplOpenGL3_Init("#version " GLSL_STRING);
}

// TODO: should start thinking about splitting initialization into smaller functions
game_state::game_state(context& ctx) : engine(), text(this) {
//game_state::game_state(context& ctx) : engine() {
	std::cerr << "got to game_state::game_state()" << std::endl;

	float fov_x = 100.f;
	float fov_y = (fov_x * SCREEN_SIZE_Y)/(float)SCREEN_SIZE_X;

	projection = glm::perspective(glm::radians(fov_y),
	                             (1.f*SCREEN_SIZE_X)/SCREEN_SIZE_Y, 0.1f, 100.f);

	view = glm::lookAt(glm::vec3(0.0, 0.0, 0.0),
	                   glm::vec3(0.0, 0.0, 1.0),
	                   glm::vec3(0.0, 1.0, 0.0));

#ifdef __EMSCRIPTEN__
	screen_x = SCREEN_SIZE_X;
	screen_y = SCREEN_SIZE_Y;
	std::cerr << "also got here" << std::endl;
#else
	SDL_GetWindowSize(ctx.window, &screen_x, &screen_y);
#endif

	//skybox = glman.load_cubemap("assets/tex/cubes/LancellottiChapel/");
	skybox = glman.load_cubemap("assets/tex/cubes/rocky-skyboxes/Skinnarviksberget/");
	std::cerr << "loaded cubemap" << std::endl;
	//skybox = glman.load_cubemap("assets/tex/cubes/rocky-skyboxes/Skinnarviksberget-tiny/", ".png");
	//skybox = glman.load_cubemap("assets/tex/cubes/rocky-skyboxes/Tantolunden6/");

	load_models();
	std::cerr << "loaded models" << std::endl;
	load_shaders();
	std::cerr << "loaded shaders" << std::endl;
	init_framebuffers();
	std::cerr << "initialized framebuffers" << std::endl;
	init_test_lights();
	std::cerr << "initialized lights" << std::endl;
	init_imgui(ctx);
	std::cerr << "loaded all the stuff" << std::endl;

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

	auto shader_obj = glman.get_shader_obj(shader);

	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_CUBE_MAP, skybox.first);
	shader_obj.set("skytexture", 4);
	DO_ERROR_CHECK();

	set_mvp(glm::mat4(0), glm::mat4(glm::mat3(view)), projection);
	//draw_model("unit_cube", glm::mat4(1));
	draw_mesh("unit_cube.default", glm::mat4(0));
	glDepthMask(GL_TRUE);
}

static const glm::vec3 cube_dirs[] = {
	// negative X, Y, Z
	{-1,  0,  0},
	{ 0, -1,  0},
	{ 0,  0, -1},

	// positive X, Y, Z
	{ 1,  0,  0},
	{ 0,  1,  0},
	{ 0,  0,  1},
};

static const glm::vec3 cube_up[] = {
	// negative X, Y, Z
	{ 0,  1,  0},
	{ 0,  0,  1},
	{ 0,  1,  0},

	// positive X, Y, Z
	{ 0,  1,  0},
	{ 0,  0,  1},
	{ 0,  1,  0},
};

void game_state::render_light_maps(context& ctx) {
	set_shader(refprobe_shader);
	update_lights();

	glEnable(GL_SCISSOR_TEST);
	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);
	glDepthFunc(GL_LESS);

	auto shader_obj = glman.get_shader_obj(shader);
	shader_obj.set("skytexture", 4);
	shader_obj.set("time_ms", SDL_GetTicks() * 1.f);

	float fov_x = 90.f;
	//float fov_y = (fov_x * 256.0)/256.0;
	float fov_y = fov_x;

	glm::mat4 view;
	glm::mat4 projection = glm::perspective(glm::radians(fov_y), 1.f, 0.1f, 20.f);

	for (auto& probe : ref_probes) {
		for (unsigned i = 0; i < 6; i++) {
			reflection_atlas->bind_atlas_fb(probe.faces[i]);

			view = glm::lookAt(probe.position - cube_dirs[i],
					probe.position,
					cube_up[i]);
			set_mvp(glm::mat4(1), view, projection);

			if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
				SDL_Die("incomplete!");
			}

			glm::vec3 bugc = (cube_dirs[i] + glm::vec3(1)) / glm::vec3(2);

			glClearColor(bugc.x, bugc.y, bugc.z, 1);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			DO_ERROR_CHECK();

			render_static(ctx);
			render_players(ctx);
			render_dynamic(ctx);
			DO_ERROR_CHECK();
			dqueue_sort_draws(probe.position);
			dqueue_flush_draws();
			DO_ERROR_CHECK();

		}
	}
}

void game_state::render_light_info(context& ctx) {
	set_shader(refprobe_debug);
	auto shader_obj = glman.get_shader_obj(shader);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, reflection_atlas->color_tex.first);
	shader_obj.set("reflection_atlas", 0);
	set_mvp(glm::mat4(1), view, projection);

	for (auto& probe : ref_probes) {
		for (unsigned i = 0; i < 6; i++) {
			glm::vec3 facevec = reflection_atlas->tex_vector(probe.faces[i]);
			std::string locstr = "cubeface[" + std::to_string(i) + "]";

			shader_obj.set(locstr, facevec);
			DO_ERROR_CHECK();
		}

		glm::mat4 trans = glm::translate(probe.position);
		draw_mesh("smoothsphere.Sphere:None", trans);
	}
}

void game_state::render_static(context& ctx) {
	for (auto& thing : static_models.nodes) {
		//draw_model(thing.name, thing.transform);
		dqueue_draw_model(thing.name, thing.transform);
	}
}

void game_state::render_players(context& ctx) {
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

	dqueue_draw_model("person", bizz);
}

void game_state::render_dynamic(context& ctx) {
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
	// TODO: should the shader_obj be automatically set the same way 'shader' is...
	auto shader_obj = glman.get_shader_obj(shader);

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

	shader_obj.set("render_fb", 6);
	shader_obj.set("render_depth", 7);
	shader_obj.set("last_frame_fb", 8);
	shader_obj.set("scale_x", (round(dsr_scale_x*rend_x))/rend_x);
	shader_obj.set("scale_y", (round(dsr_scale_y*rend_y))/rend_y);
	shader_obj.set("screen_x", screen_x);
	shader_obj.set("screen_y", screen_y);
	shader_obj.set("rend_x", rend_x);
	shader_obj.set("rend_y", rend_y);
	shader_obj.set("exposure", editor.exposure);

	DO_ERROR_CHECK();
	draw_screenquad();

	/*
	// TODO: this ends up taking 100% CPU while running...
	// TODO: why not swap rend/last framebuffers...?
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
	render_light_maps(ctx);

#ifdef NO_POSTPROCESSING
	glman.bind_default_framebuffer();
	glViewport(0, 0, screen_x, screen_y);

#else
	glman.bind_framebuffer(rend_fb);
	GLsizei width = round(rend_x*dsr_scale_x);
	GLsizei height = round(rend_y*dsr_scale_y);
	glViewport(0, 0, width, height);
	glScissor(0, 0, width, height);

	glEnable(GL_SCISSOR_TEST);
#endif

	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);
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

	render_light_info(ctx);

	set_shader(main_shader);
	update_lights();
	auto shader_obj = glman.get_shader_obj(shader);

#ifdef ENABLE_FACE_CULLING
	// TODO: toggle per-model
	glEnable(GL_CULL_FACE);
	// make sure we always start with counter-clockwise faces
	// (also should be toggled per-model)
	glFrontFace(GL_CCW);
#endif
	DO_ERROR_CHECK();

	shader_obj.set("skytexture", 4);
	shader_obj.set("time_ms", SDL_GetTicks() * 1.f);

	set_mvp(glm::mat4(1), view, projection);
	render_static(ctx);
	render_players(ctx);
	render_dynamic(ctx);
	editor.render_editor(this, &phys, ctx);

	assert(current_cam != nullptr);
	dqueue_sort_draws(current_cam->position);
	dqueue_flush_draws();

	render_skybox(ctx);

#ifndef NO_POSTPROCESSING
	glDisable(GL_SCISSOR_TEST);
	render_postprocess(ctx);
	glman.bind_default_framebuffer();
#endif

	/*
	text.render({0.0, 0.9, 0},
			"scale x: " + std::to_string(dsr_scale_x) +
			", y: " + std::to_string(dsr_scale_y));
			*/

	//text.render({-0.9, 0.9, 0}, "grend test v0");

	if (editor.mode != game_editor::mode::Inactive) {
		editor.render_imgui(this, ctx);
	}
}

void game_state::draw_debug_string(std::string str) {
	return;
	glDisable(GL_DEPTH_TEST);
	//text.render({-0.9, -0.9, 0}, str);
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
			case SDLK_m: editor.set_mode(game_editor::mode::View); break;
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

	phys.set_acceleration(player_phys_id, player_force);
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
