#include <grend/game_editor.hpp>

#define IMGUI_IMPL_OPENGL_LOADER_GLEW
#include <imgui/imgui.h>
#include <imgui/examples/imgui_impl_sdl.h>

using namespace grendx;

void game_editor::handleSelectObject(gameMain *game) {
	// TODO: functions for this
	int x, y;
	int win_x, win_y;
	Uint32 buttons = SDL_GetMouseState(&x, &y); (void)buttons;
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
}

static bool imguiWantsKeyboard(void) {
	ImGuiIO& io = ImGui::GetIO();

	return io.WantCaptureKeyboard;
}

static bool imguiWantsMouse(void) {
	ImGuiIO& io = ImGui::GetIO();

	return io.WantCaptureMouse;
}


static void handleAddNode(game_editor *editor,
                          std::string name,
                          gameObject::ptr obj)
{
	assert(editor->selectedNode != nullptr);
	obj->position = editor->entbuf.position;
	obj->rotation = editor->entbuf.rotation;
	setNode(name, editor->selectedNode, obj);
	editor->selectedNode = obj;
};

template <class T>
static bindFunc makeClicker(game_editor *editor,
                            gameMain *game,
                            std::string name)
{
	return [=] (SDL_Event& ev, unsigned flags) {
		if (editor->selectedNode
		    && ev.type == SDL_MOUSEBUTTONDOWN
			&& ev.button.button == SDL_BUTTON_LEFT)
		{
			auto ptr = std::make_shared<T>();
			std::string nodename = name + std::to_string(ptr->id);
			handleAddNode(editor, nodename, ptr);
			return (int)game_editor::mode::View;
		}

		return MODAL_NO_CHANGE;
	};
}

void game_editor::loadInputBindings(gameMain *game) {
	// camera movement (on key press)
	inputBinds.bind(MODAL_ALL_MODES,
		[&, game] (SDL_Event& ev, unsigned flags) {
			if (ev.type == SDL_KEYDOWN) {
				switch (ev.key.keysym.sym) {
					case SDLK_w: cam->velocity.z =  movement_speed; break;
					case SDLK_s: cam->velocity.z = -movement_speed; break;
					case SDLK_a: cam->velocity.x =  movement_speed; break;
					case SDLK_d: cam->velocity.x = -movement_speed; break;
					case SDLK_q: cam->velocity.y =  movement_speed; break;
					case SDLK_e: cam->velocity.y = -movement_speed; break;
					default: break;
				}
			}
			return MODAL_NO_CHANGE;
		},
		imguiWantsKeyboard);

	// Return back to View mode from any other mode
	inputBinds.bind(MODAL_ALL_MODES,
		[&, game] (SDL_Event& ev, unsigned flags) {
			if (ev.type == SDL_KEYDOWN) {
				if (ev.key.keysym.sym == SDLK_ESCAPE) {
					return (int)mode::View;
				}
			}
			return MODAL_NO_CHANGE;
		},
		imguiWantsKeyboard);

	// reload shaders
	inputBinds.bind(mode::View,
		[&, game] (SDL_Event& ev, unsigned flags) {
			if (ev.type == SDL_KEYDOWN
			    && ev.key.keysym.sym == SDLK_r
			    && flags & bindFlags::Control)
			{
					reload_shaders(game);
			}
			return MODAL_NO_CHANGE;
		},
		imguiWantsKeyboard);

	// camera movement (on key up)
	inputBinds.bind(MODAL_ALL_MODES,
		[&, game] (SDL_Event& ev, unsigned flags) {
			if (ev.type == SDL_KEYUP) {
				switch (ev.key.keysym.sym) {
					case SDLK_w:
					case SDLK_s: cam->velocity.z = 0; break;
					case SDLK_a:
					case SDLK_d: cam->velocity.x = 0; break;
					case SDLK_q:
					case SDLK_e: cam->velocity.y = 0; break;
				}
			}
			return MODAL_NO_CHANGE;
		},
		imguiWantsKeyboard);

	// map editing
	inputBinds.bind(mode::View,
		[&, game] (SDL_Event& ev, unsigned flags) {
			if (ev.type == SDL_KEYDOWN) {
				switch (ev.key.keysym.sym) {
					case SDLK_i:
						// TODO: need function to set clear state + set new root
						clear(game);
						selectedNode = game->state->rootnode = loadMap(game);
						break;
					case SDLK_o: saveMap(game, game->state->rootnode); break;
					case SDLK_DELETE: selectedNode = unlink(selectedNode); break;
				}
			}
			return MODAL_NO_CHANGE;
		},
		imguiWantsKeyboard);

	// camera movement (set direction)
	inputBinds.bind(MODAL_ALL_MODES,
		[&, game] (SDL_Event& ev, unsigned flags) {
			int x, y;
			Uint32 buttons = SDL_GetMouseState(&x, &y); (void)buttons;

			if (buttons & SDL_BUTTON(SDL_BUTTON_MIDDLE)) {
				int win_x, win_y;
				SDL_GetWindowSize(game->ctx.window, &win_x, &win_y);

				x = (x > 0)? x : win_x/2;
				y = (x > 0)? y : win_y/2;

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
			return MODAL_NO_CHANGE;
		},
		imguiWantsMouse);

	// scroll wheel for cursor placement
	inputBinds.bind(MODAL_ALL_MODES,
		[&, game] (SDL_Event& ev, unsigned flags) {
			if (ev.type == SDL_MOUSEWHEEL) {
				edit_distance -= ev.wheel.y/1.f /* scroll sensitivity */;
			}

			return MODAL_NO_CHANGE;
		},
		imguiWantsMouse);

	// camera movement (set direction)
	inputBinds.bind(mode::View,
		[&, game] (SDL_Event& ev, unsigned flags) {
			if (ev.type == SDL_MOUSEBUTTONDOWN
			    && ev.button.button == SDL_BUTTON_LEFT)
			{
				if (flags & bindFlags::Control) {
					// TODO: need like a keymapping system
					selectedNode = game->state->rootnode;

				} else {
					handleSelectObject(game);
				}
			}
			return MODAL_NO_CHANGE;
		},
		imguiWantsMouse);

	// handle menubar keybinds
	inputBinds.bind(mode::View,
		[&, game] (SDL_Event& ev, unsigned flags) {
			if (ev.type == SDL_KEYDOWN) {
				switch (ev.key.keysym.sym) {
					case SDLK_l: return (int)mode::AddSomething;
					case SDLK_g: return (int)mode::MoveSomething;
					case SDLK_r: return (int)mode::RotateSomething;
				}
			}
			return MODAL_NO_CHANGE;
		},
		imguiWantsKeyboard);

	// handle add keybinds
	inputBinds.bind(mode::AddSomething,
		[&, game] (SDL_Event& ev, unsigned flags) {
			if (ev.type == SDL_KEYDOWN) {
				switch (ev.key.keysym.sym) {
					case SDLK_o: return (int)mode::AddObject;
					case SDLK_p: return (int)mode::AddPointLight;
					case SDLK_s: return (int)mode::AddSpotLight;
					case SDLK_d: return (int)mode::AddDirectionalLight;
					case SDLK_r: return (int)mode::AddReflectionProbe;
					default:     return (int)mode::View;
				}
			}
			return MODAL_NO_CHANGE;
		},
		imguiWantsKeyboard);

	// handle move keybinds
	inputBinds.bind(mode::MoveSomething,
		[&, game] (SDL_Event& ev, unsigned flags) {
			if (ev.type == SDL_KEYDOWN) {
				entbuf.position = selectedNode->position;
				entbuf.scale    = selectedNode->scale;
				entbuf.rotation = selectedNode->rotation;

				switch (ev.key.keysym.sym) {
					case SDLK_x: return (int)mode::MoveX;
					case SDLK_y: return (int)mode::MoveY;
					case SDLK_z: return (int)mode::MoveZ;
					default:     return (int)mode::View;
				}
			}
			return MODAL_NO_CHANGE;
		},
		imguiWantsKeyboard);

	// rotate keybinds
	inputBinds.bind(mode::RotateSomething,
		[&, game] (SDL_Event& ev, unsigned flags) {
			if (ev.type == SDL_KEYDOWN) {
				entbuf.position = selectedNode->position;
				entbuf.scale    = selectedNode->scale;
				entbuf.rotation = selectedNode->rotation;

				switch (ev.key.keysym.sym) {
					case SDLK_x: return (int)mode::RotateX;
					case SDLK_y: return (int)mode::RotateY;
					case SDLK_z: return (int)mode::RotateZ;
					default:     return (int)mode::View;
				}
			}
			return MODAL_NO_CHANGE;
		},
		imguiWantsKeyboard);


	inputBinds.bind(mode::AddObject,
		makeClicker<gameObject>(this, game, "object E"),
		imguiWantsMouse);

	inputBinds.bind(mode::AddPointLight,
		makeClicker<gameLightPoint>(this, game, "point light E"),
		imguiWantsMouse);

	inputBinds.bind(mode::AddSpotLight,
		makeClicker<gameLightSpot>(this, game, "spot light E"),
		imguiWantsMouse);

	inputBinds.bind(mode::AddDirectionalLight,
		makeClicker<gameLightDirectional>(this, game, "directional light E"),
		imguiWantsMouse);

	inputBinds.bind(mode::AddReflectionProbe,
		makeClicker<gameReflectionProbe>(this, game, "reflection probe E"),
		imguiWantsMouse);

	auto releaseMove = [&, game] (SDL_Event& ev, unsigned flags) {
		if (ev.type == SDL_MOUSEBUTTONUP
		    && ev.button.button == SDL_BUTTON_LEFT)
		{
			return (int)mode::View;
		} else {
			return MODAL_NO_CHANGE;
		}
	};
	
	inputBinds.bind(mode::MoveX, releaseMove, imguiWantsMouse);
	inputBinds.bind(mode::MoveY, releaseMove, imguiWantsMouse);
	inputBinds.bind(mode::MoveZ, releaseMove, imguiWantsMouse);
	inputBinds.bind(mode::RotateX, releaseMove, imguiWantsMouse);
	inputBinds.bind(mode::RotateY, releaseMove, imguiWantsMouse);
	inputBinds.bind(mode::RotateZ, releaseMove, imguiWantsMouse);
}

void game_editor::handleInput(gameMain *game, SDL_Event& ev)
{
	ImGui_ImplSDL2_ProcessEvent(&ev);
	// TODO: camelCase
	set_mode((enum mode)inputBinds.dispatch(ev));
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

void game_editor::handleMoveRotate(gameMain *game) {
	int x, y;
	int win_x, win_y;
	Uint32 buttons = SDL_GetMouseState(&x, &y); (void)buttons;
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
