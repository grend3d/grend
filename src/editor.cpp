#include <grend/main_logic.hpp>

#define IMGUI_IMPL_OPENGL_LOADER_GLEW
#include <imgui/imgui.h>
#include <imgui/examples/imgui_impl_sdl.h>
#include <imgui/examples/imgui_impl_opengl3.h>

using namespace grendx;

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

void game_editor::menubar(void) {
	static bool demo_window = false;

	if (ImGui::BeginMainMenuBar()) {
		if (ImGui::BeginMenu("File")) {
			if (ImGui::MenuItem("Open", "CTRL+O")) {}
			if (ImGui::MenuItem("Save", "CTRL+S")) {}
			if (ImGui::MenuItem("Save As", "Shift+CTRL+S")) {}
			if (ImGui::MenuItem("Close", "CTRL+O")) {}

			ImGui::Separator();
			if (ImGui::BeginMenu("Import")) {
				if (ImGui::MenuItem("Import .obj model")) {}
				if (ImGui::MenuItem("Import .gltf models")) {}
				if (ImGui::MenuItem("Import .gltf scene")) {}
				if (ImGui::MenuItem("Import .map scene")) {}
				if (ImGui::MenuItem("Load .gltf scene as current")) {}
				ImGui::EndMenu();
			}

			ImGui::Separator();
			if (ImGui::MenuItem("Reload shaders")) {}

			ImGui::Separator();
			if (ImGui::MenuItem("Exit", "CTRL+Q")) {}

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Edit")) {
			if (ImGui::MenuItem("Undo", "CTRL+Z")) {}
			if (ImGui::MenuItem("Redo", "CTRL+Y")) {}

			ImGui::Separator();
			if (ImGui::MenuItem("Reset+Regenerate physics", "CTRL+P")) {}

			ImGui::Separator();
			if (ImGui::MenuItem("asdf", "CTRL+V")) {}

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Options")) {
			ImGui::SliderFloat("Snap increment", &fidelity,
			                  0.5f, 50.f, "%.1f");
			ImGui::SliderFloat("Movement speed", &movement_speed,
			                  1.f, 100.f, "%.1f");
			ImGui::SliderFloat("Exposure (tonemapping)", &exposure, 0.1, 10.f);
			ImGui::SliderFloat("Light threshold", &light_threshold,
			                   0.001, 1.f);

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Tools")) {
			if (ImGui::MenuItem("Objects editor", "o"))
				show_map_window = true;
			if (ImGui::MenuItem("Lights editor", "l"))
				show_lights_window = true;

			if (ImGui::MenuItem("Material editor", "CTRL+M")) {}

			ImGui::Separator();
			if (ImGui::MenuItem("Add object", "r")) {}
			if (ImGui::MenuItem("Scale object", "s")) {}

			ImGui::Separator();
			if (ImGui::MenuItem("Bake lighting", "CTRL+B")) {}
			if (ImGui::MenuItem("Generate environment light probes", "CTRL+L")) {}
			if (ImGui::MenuItem("Generate IBL cubemaps", "Shift-CTRL+L")) {}

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Debug")) {
			ImGui::MenuItem("Dear ImGui demo window", NULL, &demo_window);
			ImGui::EndMenu();
		}

		ImGui::Combo("[mode]", &mode,
			"Exit editor\0" "View\0" "Add object\0"
			"Add point light\0" "Add spot light\0" "Add directional light\0"
			"Add reflection probe\0"
			"Select\0"
			"\0");

		ImGui::EndMainMenuBar();
	}

	if (demo_window) {
		ImGui::ShowDemoWindow(&demo_window);
	}
}

// TODO: rename 'engine' to 'renderer' or something
void game_editor::render_imgui(engine *renderer, context& ctx) {
	if (mode != mode::Inactive) {
		menubar();
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	}
}

void game_editor::map_window(engine *renderer, imp_physics *phys, context& ctx) {
	ImGui::Begin("Objects", &show_map_window);
	ImGui::Columns(2);

	/*
	ImGui::Text("Object properties");
	ImGui::NextColumn();

	for (auto& s : {"X", "Y", "Z"}) {
		ImGui::Text(s);
		ImGui::SameLine();
	}
	ImGui::NextColumn();
	*/

	for (int i = 0; i < (int)dynamic_models.size(); i++) {
		auto& ent = dynamic_models[i];

		ImGui::Separator();
		std::string name = "Editor object " + std::to_string(i);
		if (ImGui::Selectable(name.c_str(), selected_object == i)) {
			selected_object = i;
		}

		ImGui::NextColumn();
		ImGui::Text(ent.name.c_str());

		if (selected_object == i) {
			renderer->draw_model_lines({
				.name = ent.name,
				.transform = glm::translate(ent.position)
					* ent.transform
					* glm::scale(glm::vec3(1.05)),
			});

			ImGui::InputFloat3("position", glm::value_ptr(ent.position));
			ImGui::InputFloat3("scale", glm::value_ptr(ent.scale));
			ImGui::InputFloat4("rotation (quat)", glm::value_ptr(ent.rotation));
		}
		ImGui::NextColumn();
	}

	for (auto& [id, obj] : phys->objects) {
		// XXX: use negative numbers to differentiate physics from editor objects
		int transid = -id - 0x8000;
		std::string name = "Physics object " + std::to_string(id);

		ImGui::Separator();
		if (ImGui::Selectable(name.c_str(), selected_object == transid)) {
			selected_object = transid;
		}

		ImGui::NextColumn();
		ImGui::Text(obj.model_name.c_str());

		if (selected_object == transid) {
			renderer->draw_model_lines({
				.name = obj.model_name,
				.transform = glm::translate(obj.position)
					* glm::scale(glm::vec3(1.05)),
			});

			ImGui::InputFloat3("position", glm::value_ptr(obj.position));
			ImGui::InputFloat3("velocity", glm::value_ptr(obj.velocity));
			//ImGui::InputFloat3("acceleration", glm::value_ptr(obj.acceleration));
			//ImGui::InputFloat3("scale", glm::value_ptr(obj.scale));
			ImGui::InputFloat4("rotation (quat)", glm::value_ptr(obj.rotation));
		}
		ImGui::NextColumn();
	}

	ImGui::Columns(1);
	ImGui::Separator();

	if (ImGui::Button("Add Object")) {
		set_mode(mode::AddObject);
	}

	ImGui::SameLine();
	if (ImGui::Button("Delete Object")) {
		if (selected_object > 0 && selected_object < (int)dynamic_models.size()) {
			dynamic_models.erase(dynamic_models.begin() + selected_object);
		}
	}

	ImGui::End();
}

void game_editor::lights_window(engine *renderer, context& ctx) {
	ImGui::Begin("Lights", &show_lights_window);
	ImGui::Columns(2);

	for (auto& [id, light] : renderer->point_lights) {
		std::string name = "Point light " + std::to_string(id);

		ImGui::Separator();
		if (ImGui::Selectable(name.c_str(), selected_light == (int)id)) {
			selected_light = id;
		}

		ImGui::NextColumn();
		ImGui::InputFloat3("position", glm::value_ptr(light.position));

		if (selected_light == (int)id) {
			ImGui::ColorEdit4("color", glm::value_ptr(light.diffuse));
			ImGui::SliderFloat("intensity", &light.intensity, 0.f, 1000.f);
			ImGui::SliderFloat("radius", &light.radius, 0.01f, 3.f);
			ImGui::Checkbox("casts shadows", &light.casts_shadows);
			ImGui::Checkbox("static shadows", &light.static_shadows);

			renderer->draw_model_lines({
				.name = "smoothsphere",
				.transform = glm::translate(glm::vec3(light.position))
					* glm::scale(glm::vec3(light_extent(&light, light_threshold))),
			});
		}

		ImGui::NextColumn();
	}

	for (auto& [id, light] : renderer->spot_lights) {
		std::string name = "Spot light " + std::to_string(id);

		ImGui::Separator();
		if (ImGui::Selectable(name.c_str(), selected_light == (int)id)) {
			selected_light = id;
		}

		ImGui::NextColumn();
		ImGui::InputFloat3("position", glm::value_ptr(light.position));

		if (selected_light == (int)id) {
			ImGui::ColorEdit4("color", glm::value_ptr(light.diffuse));
			// TODO: normalize, better representation
			ImGui::SliderFloat3("direction", glm::value_ptr(light.direction),
				-1.f, 1.f);
			ImGui::SliderFloat("intensity", &light.intensity, 0.f, 1000.f);
			ImGui::SliderFloat("radius", &light.radius, 0.01f, 3.f);
			ImGui::SliderFloat("angle", &light.angle, 0.0f, 1.f);
			ImGui::Checkbox("casts shadows", &light.casts_shadows);
			ImGui::Checkbox("static shadows", &light.static_shadows);

			/*
			renderer->draw_model_lines("smoothsphere",
				glm::translate(glm::vec3(light.position))
				* glm::scale(glm::vec3(light_extent(&light, light_threshold))));
				*/
		}

		ImGui::NextColumn();
	}

	for (auto& [id, light] : renderer->directional_lights) {
		std::string name = "directional light " + std::to_string(id);

		ImGui::Separator();
		if (ImGui::Selectable(name.c_str(), selected_light == (int)id)) {
			selected_light = id;
		}

		ImGui::NextColumn();
		ImGui::InputFloat3("position", glm::value_ptr(light.position));

		if (selected_light == (int)id) {
			ImGui::ColorEdit4("color", glm::value_ptr(light.diffuse));
			// TODO: normalize, better representation
			ImGui::SliderFloat3("direction", glm::value_ptr(light.direction),
				-1.f, 1.f);
			ImGui::SliderFloat("intensity", &light.intensity, 0.f, 1000.f);
			ImGui::Checkbox("casts shadows", &light.casts_shadows);
			ImGui::Checkbox("static shadows", &light.static_shadows);

			renderer->draw_model_lines({
				.name = "smoothsphere",
				.transform = glm::translate(glm::vec3(light.position))
			});
		}

		ImGui::NextColumn();
	}


	ImGui::Columns(1);
	ImGui::Separator();

	if (ImGui::Button("Add Point Light")) {
		set_mode(mode::AddPointLight);
	}

	ImGui::SameLine();
	if (ImGui::Button("Add Spot Light")) {
		set_mode(mode::AddSpotLight);
	}

	ImGui::SameLine();
	if (ImGui::Button("Add Directional Light")) {
		set_mode(mode::AddDirectionalLight);
	}

	ImGui::SameLine();
	if (ImGui::Button("Delete Light")) {
		renderer->free_light(selected_light);
	}

	ImGui::End();
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

static inline std::vector<float> strvec_to_float(std::vector<std::string> strs) {
	std::vector<float> ret;

	for (auto& s : strs) {
		if (!s.empty()) {
			ret.push_back(std::stof(s));
		}
	}

	return ret;
}

template <typename T>
static inline T parse_vec(std::string& str) {
	auto temp = strvec_to_float(split_string(str, ','));
	// TODO: throw an exception here?
	assert(temp.size() >= T::length());

	T ret;
	for (unsigned i = 0; i < T::length(); i++) {
		ret[i] = temp[i];
	}

	return ret;
}

void game_editor::load_map(engine *renderer, std::string name) {
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
			// TODO: size check
			auto matvec = strvec_to_float(split_string(statement[3], ','));

			editor_entry v;
			v.name = statement[1];
			v.position = parse_vec<glm::vec3>(statement[2]);
			v.inverted = std::stoi(statement[4]);

			for (unsigned i = 0; i < 16; i++) {
				v.transform[i/4][i%4] = matvec[i];
			}

			dynamic_models.push_back(v);
			std::cerr << "# loaded a " << v.name << std::endl;
		}

		if (statement[0] == "reflection_probe" && statement.size() == 2) {
			// TODO: size check
			glm::vec3 pos = parse_vec<glm::vec3>(statement[1]);

			renderer->add_reflection_probe((struct engine::reflection_probe) {
				.position = pos,
				.changed = true,
			});
		}

		if (statement[0] == "point_light" && statement.size() == 7) {
			glm::vec3 pos = parse_vec<glm::vec3>(statement[1]);
			glm::vec4 diffuse = parse_vec<glm::vec4>(statement[2]);
			float radius = std::stof(statement[3]);
			float intensity = std::stof(statement[4]);
			bool casts_shadows = std::stoi(statement[5]);
			bool static_shadows = std::stoi(statement[6]);

			uint32_t nlit = renderer->add_light((struct engine::point_light){
				.position = pos,
				.diffuse = diffuse,
				.radius = radius,
				.intensity = intensity,
				.casts_shadows = casts_shadows,
				.static_shadows = static_shadows,
			});

			edit_lights.point.push_back(nlit);
		}

		if (statement[0] == "spot_light" && statement.size() == 9) {
			glm::vec3 pos = parse_vec<glm::vec3>(statement[1]);
			glm::vec4 diffuse = parse_vec<glm::vec4>(statement[2]);
			glm::vec3 direction = parse_vec<glm::vec3>(statement[3]);
			float radius = std::stof(statement[4]);
			float intensity = std::stof(statement[5]);
			float angle = std::stof(statement[6]);
			bool casts_shadows = std::stoi(statement[7]);
			bool static_shadows = std::stoi(statement[8]);

			uint32_t nlit = renderer->add_light((struct engine::spot_light){
				.position = pos,
				.diffuse = diffuse,
				.direction = direction,
				.radius = radius,
				.intensity = intensity,
				.angle = angle,
				.casts_shadows = casts_shadows,
				.static_shadows = static_shadows,
			});

			edit_lights.spot.push_back(nlit);
		}

		if (statement[0] == "directional_light" && statement.size() == 7) {
			glm::vec3 pos = parse_vec<glm::vec3>(statement[1]);
			glm::vec4 diffuse = parse_vec<glm::vec4>(statement[2]);
			glm::vec3 direction = parse_vec<glm::vec3>(statement[3]);
			float intensity = std::stof(statement[4]);
			bool casts_shadows = std::stoi(statement[5]);
			bool static_shadows = std::stoi(statement[6]);

			uint32_t nlit = renderer->add_light((struct engine::directional_light){
				.position = pos,
				.diffuse = diffuse,
				.direction = direction,
				.intensity = intensity,
				.casts_shadows = casts_shadows,
				.static_shadows = static_shadows,
			});

			edit_lights.spot.push_back(nlit);
		}
	}
}

template <typename T>
static inline std::string format_vec(T& vec) {
	std::string ret = "";

	for (unsigned i = 0; i < T::length(); i++) {
		ret += std::to_string(vec[i]) + ",";
	}

	return ret;
}

template <typename T>
static inline std::string format_mat(T& mtx) {
	std::string ret = "";

	for (unsigned y = 0; y < T::col_type::length(); y++) {
		for (unsigned x = 0; x < T::row_type::length(); x++) {
			ret += std::to_string(mtx[y][x]) + ",";
		}
	}

	return ret;
}

void game_editor::save_map(engine *renderer, std::string name) {
	std::ofstream foo(name);
	std::cerr << "saving map " << name << std::endl;

	if (!foo.good()) {
		std::cerr << "couldn't open save file" << name << std::endl;
		return;
	}

	foo << "### test scene save file" << std::endl;

	for (auto& v : dynamic_models) {
		foo << "entity\t" << v.name << "\t"
			<< format_vec(v.position) << "\t";

		foo << format_mat(v.transform) << "\t";
		foo << v.inverted;
		foo << std::endl;
	}

	for (auto& p : renderer->ref_probes) {
		foo << "reflection_probe\t"
		    << format_vec(p.position)
		    << std::endl;
	}

	for (auto& id : edit_lights.point) {
		if (renderer->point_lights.find(id) != renderer->point_lights.end()) {
			auto lit = renderer->get_point_light(id);

			foo << "point_light\t"
			    << format_vec(lit.position) << "\t"
			    << format_vec(lit.diffuse) << "\t"
			    << lit.radius << "\t"
			    << lit.intensity << "\t"
			    << lit.casts_shadows << "\t"
			    << lit.static_shadows
			    << std::endl;
		}
	}

	for (auto& id : edit_lights.spot) {
		if (renderer->spot_lights.find(id) != renderer->spot_lights.end()) {
			auto lit = renderer->get_spot_light(id);

			foo << "spot_light\t"
			    << format_vec(lit.position) << "\t"
			    << format_vec(lit.diffuse) << "\t"
			    << format_vec(lit.direction) << "\t"
			    << lit.radius << "\t"
				<< lit.intensity << "\t"
				<< lit.angle << "\t"
			    << lit.casts_shadows << "\t"
			    << lit.static_shadows
			    << std::endl;
		}
	}

	for (auto& id : edit_lights.directional) {
		if (renderer->directional_lights.find(id)
		    != renderer->directional_lights.end())
		{
			auto lit = renderer->get_directional_light(id);

			foo << "directional_light\t"
			    << format_vec(lit.position) << "\t"
			    << format_vec(lit.diffuse) << "\t"
			    << format_vec(lit.direction) << "\t"
			    << lit.intensity << "\t"
			    << lit.casts_shadows << "\t"
			    << lit.static_shadows
			    << std::endl;
		}
	}
}
