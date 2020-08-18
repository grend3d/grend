#include <grend/gameState.hpp>

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

// TODO: probably won't be used, convenience here is pretty minimal
// TODO: move to gameModel stuff
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
			//ret[fname] = model(dir + "/" + fname);
			ret[fname] = load_object(dir + "/" + fname);
		}
	}

	closedir(dirp);
	return ret;
}

static std::pair<std::string, std::string> obj_models[] = {
	{"person",                  "assets/obj/low-poly-character-rpg/boy.obj"},
	{"smoothsphere",            "assets/obj/smoothsphere.obj"},
	{"X-Axis-Pointer",          "assets/obj/UI/X-Axis-Pointer.obj"},
	{"Y-Axis-Pointer",          "assets/obj/UI/Y-Axis-Pointer.obj"},
	{"Z-Axis-Pointer",          "assets/obj/UI/Z-Axis-Pointer.obj"},
	{"X-Axis-Rotation-Spinner", "assets/obj/UI/X-Axis-Rotation-Spinner.obj"},
	{"Y-Axis-Rotation-Spinner", "assets/obj/UI/Y-Axis-Rotation-Spinner.obj"},
	{"Z-Axis-Rotation-Spinner", "assets/obj/UI/Z-Axis-Rotation-Spinner.obj"},
};

static model_map gen_internal_models(void) {
	return {
		{"unit_cube",        generate_cuboid(1, 1, 1)},
	};
}

#if 0
void game_state::load_models(void) {
	for (auto& [name, path] : obj_models) {
		gameModel::ptr m = load_object(path);
		loadedModels[name] = m;
		//rend.glman.compile_model(name, m);
	}

	/*
	for (auto& g : gltf_models) {
		std::cerr << "loading morphcube" << std::endl;
		model_map models = load_gltf_models(g);
		glman.compile_models(models);
	}
	*/

	/*
	auto [scene, gmodels] = load_gltf_scene("assets/obj/tests/test_objects.gltf");
	static_models = scene;
	glman.compile_models(gmodels);

	for (auto& node : static_models.nodes) {
		//static_octree.add_model(models[node.name], node.transform);
		phys.add_static_model(node.name, gmodels[node.name], node.transform);
	}
	*/

	model_map intern_models = gen_internal_models();
	//rend.glman.compile_models(intern_models);

	//rend.glman.bind_cooked_meshes();
	//editor.update_models(&rend);
}
#endif

// TODO: should start thinking about splitting initialization into smaller functions
game_state::game_state() {
}

game_state::~game_state() {
}

/*
void game_state::render_light_maps(context& ctx) {
	float fov_x = 90.f;
	//float fov_y = (fov_x * 256.0)/256.0;
	float fov_y = fov_x;

	glm::mat4 view;
	glm::mat4 projection = glm::perspective(glm::radians(fov_y), 1.f, 0.1f, 100.f);
}
*/

/*
// TODO: move this to the editor
void game_state::render_light_info(context& ctx) {
	rend.set_shader(rend.shaders["refprobe_debug"]);

	glActiveTexture(GL_TEXTURE0);
	rend.reflection_atlas->color_tex->bind();
	rend.shader->set("reflection_atlas", 0);
	rend.set_mvp(glm::mat4(1), view, projection);

	for (auto& [id, probe] : rend.ref_probes) {
		for (unsigned i = 0; i < 6; i++) {
			glm::vec3 facevec = rend.reflection_atlas->tex_vector(probe.faces[i]);
			std::string locstr = "cubeface[" + std::to_string(i) + "]";

			rend.shader->set(locstr, facevec);
			DO_ERROR_CHECK();
		}

		struct draw_attributes foo = {
			.name = "smoothsphere",
			.transform = glm::translate(probe.position),
		};
		rend.draw_mesh("smoothsphere.Sphere:None", &foo);
	}
}
*/

/*
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

	rend.dqueue_draw_model({
		.name = "person",
		.transform = bizz,
		.dclass = (struct draw_class){DRAWATTR_CLASS_PHYSICS, player_phys_id}
	});
}
*/

/*
void game_state::render_postprocess(context& ctx) {
	Framebuffer().bind();
	rend.glman.bind_vao(rend.glman.screenquad_vao);
	glViewport(0, 0, screen_x, screen_y);
	rend.set_shader(rend.shaders["post"]);
	// TODO: should the shader_obj be automatically set the same way 'shader' is...

	glClearColor(0, 0, 0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT);
	glDepthMask(GL_FALSE);
	rend.glman.disable(GL_DEPTH_TEST);
	DO_ERROR_CHECK();

	glActiveTexture(GL_TEXTURE6);
	rend_tex->bind();
	glActiveTexture(GL_TEXTURE7);
	rend_depth->bind();
	glActiveTexture(GL_TEXTURE8);
	last_frame_tex->bind();

	rend.shader->set("render_fb", 6);
	rend.shader->set("render_depth", 7);
	rend.shader->set("last_frame_fb", 8);
	rend.shader->set("scale_x", (round(dsr_scale_x*rend_x))/rend_x);
	rend.shader->set("scale_y", (round(dsr_scale_y*rend_y))/rend_y);
	rend.shader->set("screen_x", (float)screen_x);
	rend.shader->set("screen_y", (float)screen_y);
	rend.shader->set("rend_x", (float)rend_x);
	rend.shader->set("rend_y", (float)rend_y);
	//rend.shader->set("exposure", editor.exposure);

	DO_ERROR_CHECK();
	rend.draw_screenquad();
}
*/

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

#if 0
void game_state::render(context& ctx) {
	render_light_maps(ctx);

#ifdef NO_POSTPROCESSING
	Framebuffer().bind();
	//rend.glman.bind_default_framebuffer();
	glViewport(0, 0, screen_x, screen_y);

#else
	rend_fb->bind();
	GLsizei width = round(rend_x*dsr_scale_x);
	GLsizei height = round(rend_y*dsr_scale_y);
	glViewport(0, 0, width, height);
	glScissor(0, 0, width, height);

	rend.glman.enable(GL_SCISSOR_TEST);
#endif

	rend.newframe();
	rend.glman.enable(GL_DEPTH_TEST);
	rend.glman.enable(GL_STENCIL_TEST);
	glDepthMask(GL_TRUE);
	glDepthFunc(GL_LESS);
	glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

#if 0
	// TODO: debug flag to toggle checks like this
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		SDL_Die("incomplete!");
	}
#endif

	//glClearColor(0.7, 0.9, 1, 1);
	glClearColor(0.1, 0.1, 0.1, 1);
	glClearStencil(0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	// XXX: left here for debugging in the future
	//render_light_info(ctx);

	rend.set_shader(rend.shaders["main"]);
	rend.update_lights();

#ifdef ENABLE_FACE_CULLING
	// TODO: toggle per-model
	rend.glman.enable(GL_CULL_FACE);
	// make sure we always start with counter-clockwise faces
	// (also should be toggled per-model)
	glFrontFace(GL_CCW);
#endif
	DO_ERROR_CHECK();

	glActiveTexture(GL_TEXTURE6);
	rend.reflection_atlas->color_tex->bind();
	glActiveTexture(GL_TEXTURE7);
	rend.shadow_atlas->depth_tex->bind();

	rend.shader->set("reflection_atlas", 6);
	rend.shader->set("shadowmap_atlas", 7);
	rend.shader->set("skytexture", 4);
	rend.shader->set("time_ms", SDL_GetTicks() * 1.f);

	rend.set_mvp(glm::mat4(1), view, projection);
	render_static(ctx);
	render_players(ctx);
	render_dynamic(ctx);
	//editor.render_map_models(&rend, ctx);
	//editor.render_editor(&rend, &phys, ctx);

	assert(current_cam != nullptr);
	rend.dqueue_sort_draws(current_cam->position);
	rend.dqueue_flush_draws();

	render_skybox(ctx);

#ifndef NO_POSTPROCESSING
	rend.glman.disable(GL_SCISSOR_TEST);
	render_postprocess(ctx);
	Framebuffer().bind();
#endif

	/*
	text.render({0.0, 0.9, 0},
			"scale x: " + std::to_string(dsr_scale_x) +
			", y: " + std::to_string(dsr_scale_y));
			*/

	//text.render({-0.9, 0.9, 0}, "grend test v0");

	/*
	if (editor.mode != game_editor::mode::Inactive) {
		editor.render_imgui(&rend, ctx);
	}
	*/
}
#endif

/*
void game_state::draw_debug_string(std::string str) {
	return;
	rend.glman.disable(GL_DEPTH_TEST);
	//text.render({-0.9, -0.9, 0}, str);
	DO_ERROR_CHECK();
}
*/

#if 0
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
			//case SDLK_m: editor.set_mode(game_editor::mode::View); break;
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

			uint64_t id = phys.add_sphere("smoothsphere",
			                              phys.objects[player_phys_id].position, 1);
			phys.objects[id].velocity = player_direction*100.f;
			// TODO: initial velocity
		}
	}
}
#endif

#if 0
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

		else if (ev.type == SDL_MOUSEBUTTONDOWN) {
			if (ev.button.button == SDL_BUTTON_LEFT) {
				GLbyte color[4];
				GLfloat depth;
				GLuint index;

				// TODO: move this into renderer, since this all depends
				//       on render state
				/*
				int x, y;
				int win_x, win_y;
				Uint32 buttons = SDL_GetMouseState(&x, &y);
				buttons = buttons; // XXX: make the compiler shut up about the unused variable
				SDL_GetWindowSize(ctx.window, &win_x, &win_y);

				// adjust coordinates for window/resolution scaling
				x = rend_x * ((1.*x)/(1.*win_x)) * dsr_scale_x;
				y = (rend_y - (rend_y * ((1.*y)/(1.*win_y))) - 1) * dsr_scale_y;

				rend_fb->bind();
				glReadPixels(x, y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, color);
				glReadPixels(x, y, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &depth);
				glReadPixels(x, y, 1, 1, GL_STENCIL_INDEX, GL_UNSIGNED_INT, &index);

				struct draw_class obj = rend.index(index);
				fprintf(stderr, "clicked %d, %d, class: %u, id: %u, color: #%02x%02x%02x:%u, depth: %f, index: %u\n",
						x, y,
						obj.class_id, obj.obj_id,
						color[0]&0xff, color[1]&0xff, color[2]&0xff, color[3]&0xff,
						depth, index);

				// XXX: TODO: not this
				switch (obj.class_id) {
					case DRAWATTR_CLASS_MAP:
						editor.selected_object = obj.obj_id;
						editor.selected_light = -1;
						editor.selected_refprobe = -1;
						break;

					case DRAWATTR_CLASS_PHYSICS:
						editor.selected_object = -(int)obj.obj_id - 0x8000;
						editor.selected_light = -1;
						editor.selected_refprobe = -1;
						break;

					case DRAWATTR_CLASS_UI_LIGHT:
						editor.selected_light = obj.obj_id;
						editor.selected_refprobe = -1;
						editor.selected_object = -1;
						break;

					case DRAWATTR_CLASS_UI_REFPROBE:
						editor.selected_refprobe = obj.obj_id;
						editor.selected_object = -1;
						editor.selected_light = -1;
						break;

					case DRAWATTR_CLASS_UI:
						{
							static const char *axis[] = {"X", "Y", "Z"};
							static const char *indicator[] = {
								"pointer", 
								"rotation spinner"
							};

							fprintf(stderr, "clicked %s axis %s\n",
								axis[obj.obj_id % 3], indicator[obj.obj_id / 3]);
						}
						break;

					default:
						editor.selected_object = -1;
						editor.selected_light = -1;
						editor.selected_refprobe = -1;
						break;
				}
				*/
			}
		}

		// TODO: these should be placed in their respective view classes
		/*
		if (editor.mode != game_editor::mode::Inactive) {
			editor.handle_editor_input(&rend, ctx, ev);
		} else {
			handle_player_input(ev);
		}
		*/
		handle_player_input(ev);
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
#endif

#if 0
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
#endif

#if 0
void game_state::logic(context& ctx) {
	Uint32 cur_ticks = SDL_GetTicks();
	Uint32 ticks_delta = cur_ticks - last_frame;
	float fticks = ticks_delta / 1000.0f;
	last_frame = cur_ticks;

	//editor.logic(ctx, fticks);
	//current_cam = editor.mode? &editor.cam : &player_cam;
	current_cam = &player_cam;

	glm::vec3 player_position = phys.objects[player_phys_id].position;
	player_cam.position = player_position - player_cam.direction*5.f;

	auto lit = rend.get_point_light(player_light);
	lit.position = player_position + glm::vec3(0, 2, 0);
	//lit.intensity = ((cur_ticks / 2000) & 1)? 100 : 0;
	rend.set_point_light(player_light, &lit);

	assert(current_cam != nullptr);
	view = glm::lookAt(current_cam->position,
	                   current_cam->position + current_cam->direction,
	                   current_cam->up);

	/*
	for (auto& obj : editor.dynamic_models) {
		float meh = cur_ticks / 1000.0f;

		if (obj.classname == "<default>") {
			obj.rotation = {sin(meh), 0, cos(meh), 0};
		}
	}
	*/

	adjust_draw_resolution();
}
#endif

#if 0
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
#endif
