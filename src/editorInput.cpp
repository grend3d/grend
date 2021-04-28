#include <grend/gameEditor.hpp>
#include <grend/controllers.hpp>

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_sdl.h>

using namespace grendx;

void gameEditor::handleSelectObject(gameMain *game) {
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
		clickedX = (x*1.f / win_x);
		clickedY = ((win_y - y)*1.f / win_y);
		clickDepth = glm::distance(clicked->getTransformTRS().position, cam->position());

		if (isUIObject(clicked)) {
			transformBuf = selectedNode->getTransformTRS();
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


static void handleAddNode(gameEditor *editor,
                          std::string name,
                          gameObject::ptr obj)
{
	assert(editor->selectedNode != nullptr);

	obj->setTransform(editor->cursorBuf);
	setNode(name, editor->selectedNode, obj);
	editor->selectedNode = obj;
};

template <class T>
static bindFunc makeClicker(gameEditor *editor,
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
			return (int)gameEditor::mode::View;
		}

		return MODAL_NO_CHANGE;
	};
}

void gameEditor::loadInputBindings(gameMain *game) {
	inputBinds.bind(MODAL_ALL_MODES, resizeInputHandler(game, post));

	// camera movement (on key press)
	inputBinds.bind(MODAL_ALL_MODES,
		[&, game] (SDL_Event& ev, unsigned flags) {
			if (ev.type == SDL_KEYDOWN) {
				auto v = cam->velocity();

				switch (ev.key.keysym.sym) {
					case SDLK_w: v.z =  movementSpeed; break;
					case SDLK_s: v.z = -movementSpeed; break;
					case SDLK_a: v.x =  movementSpeed; break;
					case SDLK_d: v.x = -movementSpeed; break;
					case SDLK_q: v.y =  movementSpeed; break;
					case SDLK_e: v.y = -movementSpeed; break;
					default: break;
				}

				cam->setVelocity(v);
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
					reloadShaders(game);
			}
			return MODAL_NO_CHANGE;
		},
		imguiWantsKeyboard);

	// camera movement (on key up)
	inputBinds.bind(MODAL_ALL_MODES,
		[&, game] (SDL_Event& ev, unsigned flags) {
			if (ev.type == SDL_KEYUP) {
				auto v = cam->velocity();

				switch (ev.key.keysym.sym) {
					case SDLK_w:
					case SDLK_s: v.z = 0; break;
					case SDLK_a:
					case SDLK_d: v.x = 0; break;
					case SDLK_q:
					case SDLK_e: v.y = 0; break;
				}

				cam->setVelocity(v);
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
						selectedNode = game->state->rootnode = loadMapCompiled(game);
						break;
					case SDLK_o: saveMap(game, game->state->rootnode); break;
					case SDLK_DELETE: selectedNode = unlink(selectedNode); break;
				}
			}
			return MODAL_NO_CHANGE;
		},
		imguiWantsKeyboard);

	// clone keybind
	inputBinds.bind(mode::View,
		[&, game] (SDL_Event& ev, unsigned flags) {
			if (ev.type == SDL_KEYDOWN
			    && ev.key.keysym.sym == SDLK_d
			    && flags & bindFlags::Shift
			    && selectedNode && !selectedNode->parent.expired())
			{
				gameObject::ptr temp = clone(selectedNode);
				// TODO: name based on selectedNode
				std::string name = "cloned " + std::to_string(temp->id);
				setNode(name, selectedNode->parent.lock(), temp);
				return (int)mode::MoveSomething;
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

				cam->setDirection(glm::vec3(
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
				editDistance -= ev.wheel.y/1.f /* scroll sensitivity */;
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
					case SDLK_f: return (int)mode::ScaleSomething;
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
				transformBuf = selectedNode->getTransformTRS();

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
				transformBuf = selectedNode->getTransformTRS();

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

	// scale keybinds
	inputBinds.bind(mode::ScaleSomething,
		[&, game] (SDL_Event& ev, unsigned flags) {
			if (ev.type == SDL_KEYDOWN) {
				transformBuf = selectedNode->getTransformTRS();

				switch (ev.key.keysym.sym) {
					case SDLK_x: return (int)mode::ScaleX;
					case SDLK_y: return (int)mode::ScaleY;
					case SDLK_z: return (int)mode::ScaleZ;
					default:     return (int)mode::View;
				}
			}
			return MODAL_NO_CHANGE;
		},
		imguiWantsKeyboard);

	inputBinds.bind(mode::View,
		[&, game] (SDL_Event& ev, unsigned flags) {
			int ret = MODAL_NO_CHANGE;

			if (selectedNode == nullptr
			    || selectedNode->type != gameObject::objType::ReflectionProbe)
			{
				// only reflection probes have editable bounding boxes for the
				// time being
				return ret;
			}

			if (ev.type == SDL_KEYDOWN) {
				if (flags & bindFlags::Shift) {
					switch (ev.key.keysym.sym) {
						case SDLK_x: ret = mode::MoveAABBPosX; break;
						case SDLK_y: ret = mode::MoveAABBPosY; break;
						case SDLK_z: ret = mode::MoveAABBPosZ; break;
						default: break;
					}

				} else {
					switch (ev.key.keysym.sym) {
						case SDLK_x: ret = mode::MoveAABBNegX; break;
						case SDLK_y: ret = mode::MoveAABBNegY; break;
						case SDLK_z: ret = mode::MoveAABBNegZ; break;
						default: break;
					}
				}
			}

			if (ret != MODAL_NO_CHANGE) {
				gameReflectionProbe::ptr probe =
					std::dynamic_pointer_cast<gameReflectionProbe>(selectedNode);

				// XXX: put bounding box in position/scale transform...
				// TODO: don't do that
				transformBuf.position = probe->boundingBox.min;
				transformBuf.scale    = probe->boundingBox.max;

				// XXX: no mouse click, pretend there was a click at the center
				//      for movement purposes
				clickedX = clickedY = 0.5;
			}

			return ret;
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

	inputBinds.bind(mode::AddIrradianceProbe,
		makeClicker<gameIrradianceProbe>(this, game, "irradiance probe E"),
		imguiWantsMouse);

	auto releaseMove = [&, game] (SDL_Event& ev, unsigned flags) {
		if (ev.type == SDL_MOUSEBUTTONUP
		    && ev.button.button == SDL_BUTTON_LEFT)
		{
			invalidateLightMaps(selectedNode);
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
	inputBinds.bind(mode::ScaleSomething, releaseMove, imguiWantsMouse);
	inputBinds.bind(mode::ScaleX, releaseMove, imguiWantsMouse);
	inputBinds.bind(mode::ScaleY, releaseMove, imguiWantsMouse);
	inputBinds.bind(mode::ScaleZ, releaseMove, imguiWantsMouse);
	inputBinds.bind(mode::MoveAABBPosX, releaseMove, imguiWantsMouse);
	inputBinds.bind(mode::MoveAABBPosY, releaseMove, imguiWantsMouse);
	inputBinds.bind(mode::MoveAABBPosZ, releaseMove, imguiWantsMouse);
	inputBinds.bind(mode::MoveAABBNegX, releaseMove, imguiWantsMouse);
	inputBinds.bind(mode::MoveAABBNegY, releaseMove, imguiWantsMouse);
	inputBinds.bind(mode::MoveAABBNegZ, releaseMove, imguiWantsMouse);
}

void gameEditor::handleInput(gameMain *game, SDL_Event& ev)
{
	ImGui_ImplSDL2_ProcessEvent(&ev);
	// TODO: camelCase
	setMode((enum mode)inputBinds.dispatch(ev));
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

void gameEditor::handleMoveRotate(gameMain *game) {
	int x, y;
	int win_x, win_y;
	Uint32 buttons = SDL_GetMouseState(&x, &y); (void)buttons;
	SDL_GetWindowSize(game->ctx.window, &win_x, &win_y);

	float adj_x = x / (1.f*win_x);
	float adj_y = (win_y - y) / (1.f * win_y);
	float amount_x = adj_x - clickedX;
	float amount_y = adj_y - clickedY;
	float amount = amount_x + amount_y;
	float reversed_x;
	float reversed_y;

	glm::mat3 rot;
	glm::vec3 dir;

	TRS selectedTransform = selectedNode->getTransformTRS();

	switch (mode) {
		case mode::MoveX:
			rot = glm::mat3_cast(transformBuf.rotation);
			dir = rot * glm::vec3(1, 0, 0); 
			reversed_x = sign(glm::dot(dir, -cam->right()));
			reversed_y = sign(glm::dot(dir, cam->up()));

			selectedTransform.position = 
				transformBuf.position
				+ dir*clickDepth*amount_x*reversed_x
				+ dir*clickDepth*amount_y*reversed_y
				;
			selectedNode->setTransform(selectedTransform);
			break;

		case mode::MoveY:
			rot = glm::mat3_cast(transformBuf.rotation);
			dir = rot * glm::vec3(0, 1, 0); 
			reversed_x = sign(glm::dot(dir, -cam->right()));
			reversed_y = sign(glm::dot(dir, cam->up()));

			selectedTransform.position =
				transformBuf.position
				+ dir*clickDepth*amount_x*reversed_x
				+ dir*clickDepth*amount_y*reversed_y
				;
			selectedNode->setTransform(selectedTransform);
			break;

		case mode::MoveZ:
			rot = glm::mat3_cast(transformBuf.rotation);
			dir = rot * glm::vec3(0, 0, 1); 
			reversed_x = sign(glm::dot(dir, -cam->right()));
			reversed_y = sign(glm::dot(dir, cam->up()));

			selectedTransform.position =
				transformBuf.position
				+ dir*clickDepth*amount_x*reversed_x
				+ dir*clickDepth*amount_y*reversed_y
				;
			selectedNode->setTransform(selectedTransform);
			break;

		// TODO: need to split rotation spinner in seperate quadrant meshes
		//       so that this can pick the right movement direction
		//       for the spinner
		case mode::RotateX:
			selectedTransform.rotation =
				glm::quat(glm::rotate(transformBuf.rotation, TAUF*amount, glm::vec3(1, 0, 0)));
			selectedNode->setTransform(selectedTransform);
			break;

		case mode::RotateY:
			selectedTransform.rotation =
				glm::quat(glm::rotate(transformBuf.rotation, TAUF*amount, glm::vec3(0, 1, 0)));
			selectedNode->setTransform(selectedTransform);
			break;

		case mode::RotateZ:
			selectedTransform.rotation =
				glm::quat(glm::rotate(transformBuf.rotation, TAUF*amount, glm::vec3(0, 0, 1)));
			selectedNode->setTransform(selectedTransform);
			break;

		// scale, unlike the others, has a mouse handler for the select mode,
		// similar to blender
		case mode::ScaleSomething:
			selectedTransform.scale =
				transformBuf.scale + glm::vec3(TAUF*amount);
			selectedNode->setTransform(selectedTransform);
			break;

		case mode::ScaleX:
			selectedTransform.scale =
				transformBuf.scale + glm::vec3(TAUF*amount, 0, 0);
			selectedNode->setTransform(selectedTransform);
			break;

		case mode::ScaleY:
			selectedTransform.scale =
				transformBuf.scale + glm::vec3(0, TAUF*amount, 0);
			selectedNode->setTransform(selectedTransform);
			break;

		case mode::ScaleZ:
			selectedTransform.scale =
				transformBuf.scale + glm::vec3(0, 0, TAUF*amount);
			selectedNode->setTransform(selectedTransform);
			break;

		default:
			break;
	}

	if (selectedNode->type == gameObject::objType::ReflectionProbe) {
		gameReflectionProbe::ptr probe =
			std::dynamic_pointer_cast<gameReflectionProbe>(selectedNode);

		float reversed_x = sign(glm::dot(glm::vec3(1, 0, 0), -cam->right()));
		float reversed_y = sign(glm::dot(glm::vec3(0, 1, 0),  cam->up()));
		float reversed_z = sign(glm::dot(glm::vec3(0, 0, 1), -cam->right()));
		float depth = glm::distance(probe->getTransformTRS().position, cam->position());

		switch (mode) {
			case mode::MoveAABBPosX:
				probe->boundingBox.max =
					transformBuf.scale 
						+ glm::vec3(1, 0, 0)*depth*amount_x*reversed_x
						+ glm::vec3(1, 0, 0)*depth*amount_y*reversed_y;
				break;

			case mode::MoveAABBPosY:
				probe->boundingBox.max =
					transformBuf.scale
						+ glm::vec3(0, 1, 0)*depth*amount_x*reversed_x
						+ glm::vec3(0, 1, 0)*depth*amount_y*reversed_y;
				break;

			case mode::MoveAABBPosZ:
				probe->boundingBox.max =
					transformBuf.scale
						+ glm::vec3(0, 0, 1)*depth*amount_x*reversed_z
						+ glm::vec3(0, 0, 1)*depth*amount_y*reversed_y*reversed_z;
				break;

			case mode::MoveAABBNegX:
				probe->boundingBox.min =
					transformBuf.position
						+ glm::vec3(1, 0, 0)*depth*amount_x*reversed_x
						+ glm::vec3(1, 0, 0)*depth*amount_y*reversed_y;
				break;

			case mode::MoveAABBNegY:
				probe->boundingBox.min =
					transformBuf.position
						+ glm::vec3(0, 1, 0)*depth*amount_x*reversed_x
						+ glm::vec3(0, 1, 0)*depth*amount_y*reversed_y;
				break;

			case mode::MoveAABBNegZ:
				probe->boundingBox.min =
					transformBuf.position
						+ glm::vec3(0, 0, 1)*depth*amount_x*reversed_z
						+ glm::vec3(0, 0, 1)*depth*amount_y*reversed_y*reversed_z;
				break;

			default:
				break;
		}
	}
}
