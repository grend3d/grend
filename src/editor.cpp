#include <grend/gameState.hpp>
#include <grend/game_editor.hpp>
#include <grend/engine.hpp>
#include <grend/gl_manager.hpp>

#define IMGUI_IMPL_OPENGL_LOADER_GLEW
#include <imgui/imgui.h>
#include <imgui/examples/imgui_impl_sdl.h>
#include <imgui/examples/imgui_impl_opengl3.h>

using namespace grendx;

game_editor::game_editor(gameMain *game) : gameView() {
	objects = gameObject::ptr(new gameObject());
	testpost = makePostprocessor<rOutput>(game->rend->shaders["post"],
	                                      SCREEN_SIZE_X, SCREEN_SIZE_Y);

	loading_thing = makePostprocessor<rOutput>(
		load_program("shaders/out/postprocess.vert",
		             "shaders/out/texpresent.frag"),
		SCREEN_SIZE_X,
		SCREEN_SIZE_Y
	);

	loading_img = gen_texture();
	loading_img->buffer(materialTexture("assets/tex/loading-splash.png"));

	clear(game);
	initImgui(game);
	load_map(game);
	loadUIModels();
	bind_cooked_meshes();

	// XXX
	show_object_editor_window = true;
};

class clicker : public gameObject {
	public:
		clicker(game_editor *ptr, enum game_editor::mode click)
			: gameObject(), editor(ptr), clickmode(click) {}; 

		virtual void onLeftClick() {
			std::cerr << "have mode: " << clickmode << std::endl;
			editor->set_mode(clickmode);

			if (parent) {
				parent->onLeftClick();
			}
		}

		game_editor *editor;
		enum game_editor::mode clickmode;
};

void game_editor::loadUIModels(void) {
	// TODO: Need to swap Z/Y pointer and spinner models
	//       blender coordinate system isn't the same as opengl's (duh)
	std::string dir = "assets/obj/UI/";
	UI_models["X-Axis-Pointer"] = load_object(dir + "X-Axis-Pointer.obj");
	UI_models["Y-Axis-Pointer"] = load_object(dir + "Y-Axis-Pointer.obj");
	UI_models["Z-Axis-Pointer"] = load_object(dir + "Z-Axis-Pointer.obj");
	UI_models["X-Axis-Rotation-Spinner"]
		= load_object(dir + "X-Axis-Rotation-Spinner.obj");
	UI_models["Y-Axis-Rotation-Spinner"]
		= load_object(dir + "Y-Axis-Rotation-Spinner.obj");
	UI_models["Z-Axis-Rotation-Spinner"]
		= load_object(dir + "Z-Axis-Rotation-Spinner.obj");

	UI_objects = gameObject::ptr(new gameObject());
	gameObject::ptr xptr = gameObject::ptr(new clicker(this, mode::MoveX));
	gameObject::ptr yptr = gameObject::ptr(new clicker(this, mode::MoveZ));
	gameObject::ptr zptr = gameObject::ptr(new clicker(this, mode::MoveY));
	gameObject::ptr xrot = gameObject::ptr(new clicker(this, mode::RotateX));
	gameObject::ptr yrot = gameObject::ptr(new clicker(this, mode::RotateZ));
	gameObject::ptr zrot = gameObject::ptr(new clicker(this, mode::RotateY));

	setNode("X-Axis",     xptr, UI_models["X-Axis-Pointer"]);
	setNode("X-Rotation", xrot, UI_models["X-Axis-Rotation-Spinner"]);
	setNode("Y-Axis",     yptr, UI_models["Y-Axis-Pointer"]);
	setNode("Y-Rotation", yrot, UI_models["Y-Axis-Rotation-Spinner"]);
	setNode("Z-Axis",     zptr, UI_models["Z-Axis-Pointer"]);
	setNode("Z-Rotation", zrot, UI_models["Z-Axis-Rotation-Spinner"]);

	setNode("X-Axis", UI_objects, xptr);
	setNode("Y-Axis", UI_objects, yptr);
	setNode("Z-Axis", UI_objects, zptr);
	setNode("X-Rotation", UI_objects, xrot);
	setNode("Y-Rotation", UI_objects, yrot);
	setNode("Z-Rotation", UI_objects, zrot);

	compile_models(UI_models);
	bind_cooked_meshes();
}

void game_editor::render(gameMain *game) {
	// TODO: handle this in gameMain or handleInput here
	int winsize_x, winsize_y;
	SDL_GetWindowSize(game->ctx.window, &winsize_x, &winsize_y);

	game->rend->framebuffer->clear();

	if (game->state->rootnode) {
		renderQueue que(cam);
		que.add(game->state->rootnode);
		que.updateLights(game->rend->shaders["shadow"], game->rend->atlases);
		que.updateReflections(game->rend->shaders["refprobe"],
		                      game->rend->atlases,
		                      game->rend->defaultSkybox);
		DO_ERROR_CHECK();

		//Framebuffer().bind();
		game->rend->framebuffer->framebuffer->bind();
		DO_ERROR_CHECK();

		game->rend->shaders["main"]->bind();
		glActiveTexture(GL_TEXTURE6);
		game->rend->atlases.reflections->color_tex->bind();
		game->rend->shaders["main"]->set("reflection_atlas", 6);
		glActiveTexture(GL_TEXTURE7);
		game->rend->atlases.shadows->depth_tex->bind();
		game->rend->shaders["main"]->set("shadowmap_atlas", 7);
		DO_ERROR_CHECK();

		//game->rend->shaders["main"]->set("skytexture", 4);
		game->rend->shaders["main"]->set("time_ms", SDL_GetTicks() * 1.f);
		DO_ERROR_CHECK();
		
		que.flush(game->rend->framebuffer, game->rend->shaders["main"],
		          game->rend->atlases);

		// add UI objects after rendering main scene, using different shader
		// TODO: maybe attach shaders to gameObjects
		que.add(UI_objects);
		que.flush(game->rend->framebuffer, game->rend->shaders["unshaded"],
		          game->rend->atlases);

		game->rend->defaultSkybox.draw(cam, game->rend->framebuffer);
	}

	testpost->draw(game->rend->framebuffer);

	render_editor(game);
	render_imgui(game);

	glDepthMask(GL_TRUE);
	enable(GL_DEPTH_TEST);
}

void game_editor::initImgui(gameMain *game) {
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGui::StyleColorsDark();
	ImGui_ImplSDL2_InitForOpenGL(game->ctx.window, game->ctx.glcontext);
	// TODO: make the glsl version here depend on GL version/the string in
	//       shaders/version.glsl
	//ImGui_ImplOpenGL3_Init("#version 130");
	//ImGui_ImplOpenGL3_Init("#version 300 es");
	ImGui_ImplOpenGL3_Init("#version " GLSL_STRING);
}

void game_editor::load_model(gameMain *game, std::string path) {
	std::string ext = filename_extension(path);
	if (ext == ".obj") {
		//model m(path);
		gameModel::ptr m = load_object(path);
		compile_model(path, m);

		// add to editor model selection
		models[path] = m;
	}

	else if (ext == ".gltf") {
		model_map mods = load_gltf_models(path);
		compile_models(mods);
		models.insert(mods.begin(), mods.end());
	}

	// TODO: keep track of source files in model
	editor_model_files.push_back(path);
}

void game_editor::load_scene(gameMain *game, std::string path) {
	std::string ext = filename_extension(path);
	if (ext == ".gltf") {
		auto [scene, mods] = load_gltf_scene(path);
		std::cerr << "load_scene(): loading scene" << std::endl;

		/*
		for (auto& node : scene.nodes) {
			dynamic_mods.push_back((struct editor_entry) {
				.name = node.name,
				.classname = "static",
				.transform = node.transform,
				.inverted = node.inverted,
			});
		}
		*/

		/*
		if (selectedNode == nullptr) {
			selectedNode = game->state->rootnode;
		}
		*/

		for (auto& node : scene.nodes) {
			std::cerr << "load_scene(): loading node" << std::endl;
			// TODO: need to be able to have the same model with different
			//       position/rotation/scale
			//       hmm... why not just use a wrapper gameObject?
			gameObject::ptr obj = gameObject::ptr(new gameObject());

			obj->position = node.position;
			obj->scale    = node.scale;
			obj->rotation = node.rotation;
			setNode(node.name, obj, mods[node.name]);
			setNode(node.name, selectedNode, obj);
		}

		compile_models(mods);
		models.insert(mods.begin(), mods.end());
	}

	editor_scene_files.push_back(path);
}

void game_editor::update_models(gameMain *game) {
	//const gl_manager& glman = game->rend->get_glman();
	//edit_model = glman.cooked_models.begin();
}

void game_editor::reload_shaders(gameMain *game) {
	for (auto& [name, shader] : game->rend->shaders) {
		if (shader->reload()) {
			link_program(shader);

		} else {
			std::cerr << ">> couldn't reload shader: " << name << std::endl;
		}
	}
}

void game_editor::handleInput(gameMain *game, SDL_Event& ev)
{
	static bool control = false;
	ImGuiIO& io = ImGui::GetIO();
	ImGui_ImplSDL2_ProcessEvent(&ev);

	if (!io.WantCaptureKeyboard) {
		if (ev.type == SDL_KEYDOWN) {
			switch (ev.key.keysym.sym) {
				case SDLK_RCTRL:
				case SDLK_LCTRL:
					control = true;
					break;

				case SDLK_w: cam->velocity.z =  movement_speed; break;
				case SDLK_s: cam->velocity.z = -movement_speed; break;
				case SDLK_a: cam->velocity.x =  movement_speed; break;
				case SDLK_d: cam->velocity.x = -movement_speed; break;
				case SDLK_q: cam->velocity.y =  movement_speed; break;
				case SDLK_e: cam->velocity.y = -movement_speed; break;

				//case SDLK_m: in_edit_mode = !in_edit_mode; break;
				case SDLK_m: mode = mode::Inactive; break;
				case SDLK_i: load_map(game); break;
				case SDLK_o: save_map(game); break;

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
					entbuf.scale *= 0.9f;
					break;

				case SDLK_k:
					// scale up
					entbuf.transform *= glm::scale(glm::vec3(1/0.9));
					entbuf.scale *= 1/0.9f;
					break;

					/*
				// TODO: another way to explore loaded models
				case SDLK_r:
					if (control) {
						reload_shaders(game);

					} else {
						// scroll forward through models
						if (edit_model == cooked_models.begin()) {
							edit_model = glman.cooked_models.end();
						}
						edit_model--;
					}
					break;

				case SDLK_f:
					edit_model++;
					if (edit_model == glman.cooked_models.end()) {
					    edit_model = glman.cooked_models.begin();
					}
					break;
					*/

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
				case SDLK_RCTRL:
				case SDLK_LCTRL:
					control = false;
					break;

				case SDLK_w:
				case SDLK_s:
					cam->velocity.z = 0;
					break;

				case SDLK_a:
				case SDLK_d:
					cam->velocity.x = 0;
					break;

				case SDLK_q:
				case SDLK_e:
					cam->velocity.y = 0;
					break;
			}
		}
	}

	if (!io.WantCaptureMouse) {
		int x, y;
		Uint32 buttons = SDL_GetMouseState(&x, &y); (void)buttons;

		if (buttons & SDL_BUTTON(SDL_BUTTON_MIDDLE)) {
			int win_x, win_y;
			SDL_GetWindowSize(game->ctx.window, &win_x, &win_y);

			x = (x > 0)? x : win_x/2;
			y = (x > 0)? y : win_y/2;

			// TODO: get window resolution
			float center_x = (float)win_x / 2;
			float center_y = (float)win_y / 2;

			float rel_x = ((float)x - center_x) / center_x;
			float rel_y = ((float)y - center_y) / center_y;

			cam->set_direction(glm::vec3(
				sin(rel_x*2*M_PI),
				sin(-rel_y*M_PI/2.f),
				-cos(rel_x*2*M_PI)
			));
		}

		if (ev.type == SDL_MOUSEWHEEL) {
			edit_distance -= ev.wheel.y/10.f /* scroll sensitivity */;
		}

		else if (ev.type == SDL_MOUSEBUTTONDOWN) {
			if (ev.button.button == SDL_BUTTON_LEFT) {
				// TODO: functions for this
				int x, y;
				int win_x, win_y;
				Uint32 buttons = SDL_GetMouseState(&x, &y);
				SDL_GetWindowSize(game->ctx.window, &win_x, &win_y);

				float fx = x/(1.f*win_x);
				float fy = y/(1.f*win_y);

				gameObject::ptr clicked = game->rend->framebuffer->index(fx, fy);
				std::cerr << "clicked object: " << clicked << std::endl;

				if (clicked) {
					clicked->onLeftClick();
					clicked_x = (x*1.f / win_x);
					clicked_y = ((win_y - y)*1.f / win_y);
					click_depth = glm::distance(clicked->position, cam->position);

					if (isUIObject(clicked)) {
						entbuf.position = selectedNode->position;
						entbuf.scale    = selectedNode->scale;
						entbuf.rotation = selectedNode->rotation;
						std::cerr << "It's a UI model" << std::endl;

					} else {
						selectedNode = getNonModel(clicked);
						assert(selectedNode != nullptr);
						std::cerr << "Not a UI model" << std::endl;
					}

				} else {
					selectedNode = nullptr;
				}

				switch (mode) {
// TODO:
#if 0
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
							rend->add_light((struct point_light) {
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
							rend->add_light((struct spot_light) {
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
							rend->add_light((struct directional_light) {
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
						rend->add_reflection_probe((struct reflection_probe) {
							.position = entbuf.position,
							.changed = true,
						});
						set_mode(mode::View);
						break;
#endif

					default:
						break;
				}
			}

		} else if (ev.type == SDL_MOUSEBUTTONUP) {
			if (ev.button.button == SDL_BUTTON_LEFT) {
				switch (mode) {
					// switch back to view mode after releasing the left button
					case mode::MoveX:
					case mode::MoveY:
					case mode::MoveZ:
					case mode::RotateX:
					case mode::RotateY:
					case mode::RotateZ:
						mode = mode::View;
						break;

					default:
						break;
				}
			}
		}

		/*
		// TODO: reuse this for cursor code
		auto align = [&] (float x) { return floor(x * fidelity)/fidelity; };
		entbuf.position = glm::vec3(
			align(cam->direction.x*edit_distance + cam->position.x),
			align(cam->direction.y*edit_distance + cam->position.y),
			align(cam->direction.z*edit_distance + cam->position.z));
			*/
	}
}

// TODO: move to utility
#define TAU  (6.2831853)
#define TAUF (6.2831853f)

template <typename T>
static T sign(T x) {
	if (x >= 0) {
		return 1;
	} else {
		return -1;
	}
}

void game_editor::logic(gameMain *game, float delta) {
	// TODO: should time-dependent position updates be part of the camera
	//       class? (probably)
	cam->position += cam->velocity.z*cam->direction*delta;
	cam->position += cam->velocity.y*cam->up*delta;
	cam->position += cam->velocity.x*cam->right*delta;

	gameObject::ptr target = selectedNode;

	// models and meshes shouldn't be the main node info containers since they
	// might not be unique, so traverse upwards until something that's not
	// a model/mesh is reached
	for (; target &&
			(target->type == gameObject::objType::Mesh
			 || target->type == gameObject::objType::Model);
		 target = target->parent);

	if (target) {
		for (auto& str : {"X-Axis", "Y-Axis", "Z-Axis",
		                  "X-Rotation", "Y-Rotation", "Z-Rotation"})
		{
			auto ptr = UI_objects->getNode(str);

			ptr->position = target->position;
			ptr->scale    = target->scale;
			ptr->rotation = target->rotation;
		}
	}

	handleMoveRotate(game);
}

void game_editor::handleMoveRotate(gameMain *game) {
	int x, y;
	int win_x, win_y;
	Uint32 buttons = SDL_GetMouseState(&x, &y);
	SDL_GetWindowSize(game->ctx.window, &win_x, &win_y);

	float adj_x = x / (1.f*win_x);
	float adj_y = (win_y - y) / (1.f * win_y);
	float amount_x = adj_x - clicked_x;
	float amount_y = adj_y - clicked_y;
	float amount = amount_x + amount_y;
	float reversed_x;
	float reversed_y;

	glm::mat3 rot;
	glm::vec3 dir;

	switch (mode) {
		case mode::MoveX:
			rot = glm::mat3_cast(entbuf.rotation);
			dir = rot * glm::vec3(1, 0, 0); 
			reversed_x = sign(glm::dot(dir, -cam->right));
			reversed_y = sign(glm::dot(dir, cam->up));

			selectedNode->position =
				entbuf.position
				+ dir*click_depth*amount_x*reversed_x
				+ dir*click_depth*amount_y*reversed_y
				;
			break;

		case mode::MoveY:
			rot = glm::mat3_cast(entbuf.rotation);
			dir = rot * glm::vec3(0, 1, 0); 
			reversed_x = sign(glm::dot(dir, -cam->right));
			reversed_y = sign(glm::dot(dir, cam->up));

			selectedNode->position =
				entbuf.position
				+ dir*click_depth*amount_x*reversed_x
				+ dir*click_depth*amount_y*reversed_y
				;
			break;

		case mode::MoveZ:
			rot = glm::mat3_cast(entbuf.rotation);
			dir = rot * glm::vec3(0, 0, 1); 
			reversed_x = sign(glm::dot(dir, -cam->right));
			reversed_y = sign(glm::dot(dir, cam->up));

			selectedNode->position =
				entbuf.position
				+ dir*click_depth*amount_x*reversed_x
				+ dir*click_depth*amount_y*reversed_y
				;
			break;

		// TODO: need to split rotation spinner in seperate quadrant meshes
		//       so that this can pick the right movement direction
		//       for the spinner
		case mode::RotateX:
			selectedNode->rotation
				= glm::quat(glm::rotate(entbuf.rotation, TAUF*amount, glm::vec3(1, 0, 0)));
			break;

		case mode::RotateY:
			selectedNode->rotation
				= glm::quat(glm::rotate(entbuf.rotation, TAUF*amount, glm::vec3(0, 1, 0)));
			break;

		case mode::RotateZ:
			selectedNode->rotation
				= glm::quat(glm::rotate(entbuf.rotation, TAUF*amount, glm::vec3(0, 0, 1)));
			break;

		default:
			break;
	}
}

void game_editor::showLoadingScreen(gameMain *game) {
	// TODO: maybe not this
	loading_thing->draw(loading_img, nullptr);
	SDL_GL_SwapWindow(game->ctx.window);
}

bool game_editor::isUIObject(gameObject::ptr obj) {
	for (gameObject::ptr temp = obj; temp; temp = temp->parent) {
		if (temp == UI_objects) {
			return true;
		}
	}

	return false;
}

gameObject::ptr game_editor::getNonModel(gameObject::ptr obj) {
	for (gameObject::ptr temp = obj; temp; temp = temp->parent) {
		if (temp->type != gameObject::objType::Mesh
		    && temp->type != gameObject::objType::Model)
		{
			return temp;
		}
	}

	return nullptr;
}

void game_editor::clear(gameMain *game) {
	showLoadingScreen(game);

	cam->position = {0, 0, 0};
	editor_model_files.clear();

	// TODO: clear() for state
	selectedNode = game->state->rootnode = gameObject::ptr(new gameObject());
}

// TODO: rename 'renderer' to 'rend' or something
void game_editor::render_imgui(gameMain *game) {
	menubar(game);
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

// don't think this is needed anymore...
/*
void game_editor::render_map_models(renderer *rend, context& ctx) {
	for (unsigned i = 0; i < dynamic_models.size(); i++) {
		auto& v = dynamic_models[i];
		glm::mat4 trans = glm::translate(v.position)
			* glm::mat4_cast(v.rotation)
			* glm::scale(v.scale)
			* v.transform;

		rend->dqueue_draw_model({
			.name = v.name,
			.transform = trans,
			.face_order = v.inverted? GL_CW : GL_CCW,
			.cull_faces = true,
			.dclass = {DRAWATTR_CLASS_MAP, i},
		});

		DO_ERROR_CHECK();
	}
}
*/

void game_editor::render_editor(gameMain *game) {
	if (true /*mode != mode::Inactive*/) {
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplSDL2_NewFrame(game->ctx.window);
		ImGui::NewFrame();

		if (show_map_window) {
			map_window(game);
		}

		if (show_object_select_window) {
			object_select_window(game);
		}

		if (selectedNode && show_object_editor_window) {
			objectEditorWindow(game);
		}

#if 0

		if (mode == mode::AddObject) {
			struct draw_attributes attrs = {
				.name = edit_model->first,
				.transform = glm::translate(entbuf.position) * entbuf.transform,
			};

			glFrontFace(entbuf.inverted? GL_CW : GL_CCW);
			rend->draw_model_lines(&attrs);
			rend->draw_model(&attrs);
			DO_ERROR_CHECK();
		}

		if (mode == mode::AddPointLight) {
			rend->dqueue_draw_model({
				.name = "smoothsphere",
				.transform = 
					glm::translate(entbuf.position)
					// TODO: keep scale state
					* glm::scale(glm::vec3(1)),
			});

		// TODO: cone here
		} else if (mode == mode::AddSpotLight) {
			rend->dqueue_draw_model({
				.name = "smoothsphere",
				.transform = glm::translate(entbuf.position)
					// TODO: keep scale state
					* glm::scale(glm::vec3(1)),
			});

		} else if (mode == mode::AddDirectionalLight) {
			rend->dqueue_draw_model({
				.name = "smoothsphere",
				.transform = glm::translate(entbuf.position)
					// TODO: keep scale state
					* glm::scale(glm::vec3(1)),
			});

		} else if (mode == mode::AddReflectionProbe) {
			rend->dqueue_draw_model({
				.name = "smoothsphere",
				.transform = glm::translate(entbuf.position),
			});
		}
#endif
	}
}
