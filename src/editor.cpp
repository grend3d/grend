#include <grend/main_logic.hpp>

#define IMGUI_IMPL_OPENGL_LOADER_GLEW
#include <imgui/imgui.h>
#include <imgui/examples/imgui_impl_sdl.h>
#include <imgui/examples/imgui_impl_opengl3.h>

using namespace grendx;

void game_editor::load_model(engine *renderer, std::string path) {
	std::string ext = filename_extension(path);
	// TODO: XXX: no need for const
	auto& glman = (gl_manager&)renderer->get_glman();

	if (ext == ".obj") {
		model m(path);
		glman.compile_model(path, m);
	}

	else if (ext == ".gltf") {
		model_map models = load_gltf_models(path);
		glman.compile_models(models);
	}

	editor_model_files.push_back(path);
}

void game_editor::update_models(engine *renderer) {
	const gl_manager& glman = renderer->get_glman();
	edit_model = glman.cooked_models.begin();
}

void game_editor::handle_editor_input(engine *renderer,
                                      context& ctx,
                                      SDL_Event& ev)
{
	const gl_manager& glman = renderer->get_glman();
	ImGuiIO& io = ImGui::GetIO();
	ImGui_ImplSDL2_ProcessEvent(&ev);

	if (!io.WantCaptureKeyboard) {
		if (ev.type == SDL_KEYDOWN) {
			switch (ev.key.keysym.sym) {
				case SDLK_w: cam.velocity.z =  movement_speed; break;
				case SDLK_s: cam.velocity.z = -movement_speed; break;
				case SDLK_a: cam.velocity.x =  movement_speed; break;
				case SDLK_d: cam.velocity.x = -movement_speed; break;
				case SDLK_q: cam.velocity.y =  movement_speed; break;
				case SDLK_e: cam.velocity.y = -movement_speed; break;

				//case SDLK_m: in_edit_mode = !in_edit_mode; break;
				case SDLK_m: mode = mode::Inactive; break;
				case SDLK_i: load_map(renderer); break;
				case SDLK_o: save_map(renderer); break;

				case SDLK_g:
					//edit_rotation -= M_PI/4;
					entbuf.transform *= glm::rotate((float)-M_PI/4.f, glm::vec3(1, 0, 0));
					break;

				case SDLK_h:
					//edit_rotation -= M_PI/4;
					entbuf.transform *= glm::rotate((float)M_PI/4.f, glm::vec3(1, 0, 0));
					break;

				case SDLK_z:
					//edit_rotation -= M_PI/4;
					entbuf.transform *= glm::rotate((float)-M_PI/4.f, glm::vec3(0, 1, 0));
					break;

				case SDLK_c:
					//edit_rotation += M_PI/4;
					entbuf.transform *= glm::rotate((float)M_PI/4.f, glm::vec3(0, 1, 0));
					break;

				case SDLK_x:
					// flip horizontally (along the X axis)
					entbuf.transform *= glm::scale(glm::vec3(-1, 1, 1));
					entbuf.inverted = !entbuf.inverted;
					break;

				case SDLK_b:
					// flip horizontally (along the Z axis)
					entbuf.transform *= glm::scale(glm::vec3( 1, 1, -1));
					entbuf.inverted = !entbuf.inverted;
					break;

				case SDLK_v:
					// flip vertically
					entbuf.transform *= glm::scale(glm::vec3( 1, -1, 1));
					entbuf.inverted = !entbuf.inverted;
					break;

				case SDLK_j:
					// scale down
					entbuf.transform *= glm::scale(glm::vec3(0.9));
					break;

				case SDLK_k:
					// scale up
					entbuf.transform *= glm::scale(glm::vec3(1/0.9));
					break;

				case SDLK_r:
					if (edit_model == glman.cooked_models.begin()) {
					    edit_model = glman.cooked_models.end();
					}
					edit_model--;
					break;

				case SDLK_f:
					edit_model++;
					if (edit_model == glman.cooked_models.end()) {
					    edit_model = glman.cooked_models.begin();
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

		else if (ev.type == SDL_KEYUP) {
			switch (ev.key.keysym.sym) {
				case SDLK_w:
				case SDLK_s:
					cam.velocity.z = 0;
					break;

				case SDLK_a:
				case SDLK_d:
					cam.velocity.x = 0;
					break;

				case SDLK_q:
				case SDLK_e:
					cam.velocity.y = 0;
					break;
			}
		}
	}

	if (!io.WantCaptureMouse) {
		int x, y;
		Uint32 buttons = SDL_GetMouseState(&x, &y); (void)buttons;

		if (buttons & SDL_BUTTON(SDL_BUTTON_MIDDLE)) {
			// TODO: generic functions to do SO(3) rotation (does glm do it?
			//       probably, also maybe should be quarternions...)
			int win_x, win_y;
			SDL_GetWindowSize(ctx.window, &win_x, &win_y);

			x = (x > 0)? x : win_x/2;
			y = (x > 0)? y : win_y/2;

			// TODO: get window resolution
			float center_x = (float)win_x / 2;
			float center_y = (float)win_y / 2;

			float rel_x = ((float)x - center_x) / center_x;
			float rel_y = ((float)y - center_y) / center_y;

			cam.set_direction(glm::vec3(
				movement_speed * sin(rel_x*2*M_PI),
				movement_speed * sin(-rel_y*M_PI/2.f),
				movement_speed * -cos(rel_x*2*M_PI)
			));
		}

		if (ev.type == SDL_MOUSEWHEEL) {
			edit_distance -= ev.wheel.y/10.f /* scroll sensitivity */;
		}

		else if (ev.type == SDL_MOUSEBUTTONDOWN) {
			if (ev.button.button == SDL_BUTTON_LEFT) {
				switch (mode) {
					case mode::AddObject:
						/*
						dynamic_models.push_back({
							edit_model->first,
							edit_position,
							edit_transform,
							edit_inverted,
						});
						*/

						selected_object = dynamic_models.size();
						entbuf.name = edit_model->first;
						dynamic_models.push_back(entbuf);
						set_mode(mode::View);
						break;

					case mode::AddPointLight:
						selected_light =
							renderer->add_light((struct engine::point_light) {
								.position = glm::vec4(entbuf.position, 1.0),
								// TODO: set before placing
								.diffuse = glm::vec4(1),
								.radius = 1.f,
								.intensity = 50.f,
							});
						edit_lights.point.push_back(selected_light);
						set_mode(mode::View);
						break;

					case mode::AddSpotLight:
						selected_light =
							renderer->add_light((struct engine::spot_light) {
								.position = glm::vec4(entbuf.position, 1.0),
								// TODO: set before placing
								.diffuse = glm::vec4(1),
								.direction = {0, 1, 0},
								.radius = 1.f,
								.intensity = 50.f,
								.angle = 0.5f,
							});
						edit_lights.spot.push_back(selected_light);
						set_mode(mode::View);
						break;

					case mode::AddDirectionalLight:
						selected_light =
							renderer->add_light((struct engine::directional_light) {
								.position = glm::vec4(entbuf.position, 1.0),
								// TODO: set before placing
								.diffuse = glm::vec4(1),
								.direction = {0, 1, 0},
								.intensity = 50.f,
							});
						edit_lights.directional.push_back(selected_light);
						set_mode(mode::View);
						break;

					case mode::AddReflectionProbe:
						renderer->add_reflection_probe((struct engine::reflection_probe) {
							.position = entbuf.position,
							.changed = true,
						});
						set_mode(mode::View);
						break;

					default:
						break;
				}
			}
		}

		// TODO: snap to increments
		auto align = [&] (float x) { return floor(x * fidelity)/fidelity; };
		entbuf.position = glm::vec3(
			align(cam.direction.x*edit_distance + cam.position.x),
			align(cam.direction.y*edit_distance + cam.position.y),
			align(cam.direction.z*edit_distance + cam.position.z));
	}
}

void game_editor::logic(context& ctx, float delta) {
	// TODO: should time-dependent position updates be part of the camera
	//       class? (probably)
	cam.position += cam.velocity.z*cam.direction*delta;
	cam.position += cam.velocity.y*cam.up*delta;
	cam.position += cam.velocity.x*cam.right*delta;
}

void game_editor::clear(engine *renderer) {
	cam.position = {0, 0, 0};
	dynamic_models.clear();
	editor_model_files.clear();

	for (auto& vec : {edit_lights.point, edit_lights.spot, edit_lights.directional}) {
		for (uint32_t id : vec) {
			renderer->free_light(id);
		}
	}

	edit_lights.point.clear();
	edit_lights.spot.clear();
	edit_lights.directional.clear();

	renderer->ref_probes.clear();
}

// TODO: rename 'engine' to 'renderer' or something
void game_editor::render_imgui(engine *renderer, context& ctx) {
	if (mode != mode::Inactive) {
		menubar(renderer);
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	}
}


void game_editor::render_map_models(engine *renderer, context& ctx) {
	for (auto& v : dynamic_models) {
		glm::mat4 trans = glm::translate(v.position) * v.transform;

		//glFrontFace(v.inverted? GL_CW : GL_CCW);
		renderer->dqueue_draw_model({
			.name = v.name,
			.transform = trans,
			.face_order = v.inverted? GL_CW : GL_CCW,
			.cull_faces = true,
		});

		DO_ERROR_CHECK();
	}
}

void game_editor::render_editor(engine *renderer,
                                imp_physics *phys,
                                context& ctx)
{
	if (mode != mode::Inactive) {
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplSDL2_NewFrame(ctx.window);
		ImGui::NewFrame();

		if (show_map_window) {
			map_window(renderer, phys, ctx);
		}

		if (show_lights_window) {
			for (const auto& [id, plit] : renderer->point_lights) {
				renderer->dqueue_draw_model({
					.name = "smoothsphere",
					.transform = glm::translate(glm::vec3(plit.position))
						* glm::scale(glm::vec3(plit.radius)),
				});
			}

			lights_window(renderer, ctx);
		}

		if (show_refprobe_window) {
			refprobes_window(renderer, ctx);
		}

		if (mode == mode::AddObject) {
			struct engine::draw_attributes attrs = {
				.name = edit_model->first,
				.transform = glm::translate(entbuf.position) * entbuf.transform,
			};

			glFrontFace(entbuf.inverted? GL_CW : GL_CCW);
			renderer->draw_model_lines(&attrs);
			renderer->draw_model(&attrs);
			DO_ERROR_CHECK();
		}

		if (mode == mode::AddPointLight) {
			renderer->dqueue_draw_model({
				.name = "smoothsphere",
				.transform = 
					glm::translate(entbuf.position)
					// TODO: keep scale state
					* glm::scale(glm::vec3(1)),
			});

		// TODO: cone here
		} else if (mode == mode::AddSpotLight) {
			renderer->dqueue_draw_model({
				.name = "smoothsphere",
				.transform = glm::translate(entbuf.position)
					// TODO: keep scale state
					* glm::scale(glm::vec3(1)),
			});

		} else if (mode == mode::AddDirectionalLight) {
			renderer->dqueue_draw_model({
				.name = "smoothsphere",
				.transform = glm::translate(entbuf.position)
					// TODO: keep scale state
					* glm::scale(glm::vec3(1)),
			});

		} else if (mode == mode::AddReflectionProbe) {
			renderer->dqueue_draw_model({
				.name = "smoothsphere",
				.transform = glm::translate(entbuf.position),
			});
		}
	}
}
