#include <grend/playerView.hpp>
#include <grend/gameView.hpp>
#include <grend/audioMixer.hpp>
#include <grend/controllers.hpp>
#include <glm/gtx/rotate_vector.hpp>

using namespace grendx;

static renderPostChain::ptr post = nullptr;
static gameObject::ptr testweapon = nullptr;

static channelBuffers_ptr weaponSound =
	openAudio(GR_PREFIX "assets/sfx/impact.ogg");

struct nvg_data {
	int fontNormal, fontBold, fontIcons, fontEmoji;
};

playerView::playerView(gameMain *game) : gameView() {
	static const float speed = 15.f;

	post = renderPostChain::ptr(new renderPostChain(
		{game->rend->shaders["tonemap"]},
		SCREEN_SIZE_X, SCREEN_SIZE_Y));

	cameraPhys = game->phys->addSphere(cameraObj, glm::vec3(0, 10, 0), 1.0, 1.0);
	setNode("player camera", game->state->physObjects, cameraObj);

	input.bind(MODAL_ALL_MODES,
		[&, game] (SDL_Event& ev, unsigned flags) {
			if (ev.type == SDL_WINDOWEVENT
			    && ev.window.event == SDL_WINDOWEVENT_RESIZED)
			{
				auto width = ev.window.data1;
				auto height = ev.window.data2;

				game->rend->framebuffer->setSize(width, height);
				post->setSize(width, height);
			}
			return MODAL_NO_CHANGE;
		});


	input.bind(modes::MainMenu,
		[&] (SDL_Event& ev, unsigned flags) {
			if (ev.type == SDL_KEYDOWN) {
				switch (ev.key.keysym.sym) {
					case SDLK_DOWN:
					case SDLK_j:
						menuSelect++;
						break;

					case SDLK_UP:
					case SDLK_k:
						menuSelect = max(0, menuSelect - 1);
						break;

					case SDLK_RETURN:
					case SDLK_SPACE:
						if (menuSelect == 0) {
							return (int)modes::Move;
						}
						break;

					default:
						break;
				}
			}

			return MODAL_NO_CHANGE;
		});

	input.bind(modes::Move, controller::camMovement(cam, 10.f));
	input.bind(modes::Move, controller::camFPS(cam, game));

	// switch modes (TODO: add pause)
	input.bind(modes::Move,
		[&] (SDL_Event& ev, unsigned flags) {
			if (ev.type == SDL_KEYDOWN) {
				switch (ev.key.keysym.sym) {
					// TODO: should pause
					case SDLK_ESCAPE: return (int)modes::MainMenu;
					default: break;
				};
			}
			return MODAL_NO_CHANGE;
		});

	// TODO: this could be a function generator
	input.bind(modes::Move,
		[&, game] (SDL_Event& ev, unsigned flags) {
			if (ev.type == SDL_MOUSEBUTTONDOWN
			    && ev.button.button == SDL_BUTTON_LEFT)
			{
				auto sound = openAudio(GR_PREFIX "assets/sfx/impact.ogg");
				auto ptr = std::make_shared<stereoAudioChannel>(sound);
				game->audio->add(ptr);
			}

			return MODAL_NO_CHANGE;
		});

	input.setMode(modes::MainMenu);
}

void playerView::handleInput(gameMain *game, SDL_Event& ev) {
	input.dispatch(ev);
}

static glm::vec3 lastvel = glm::vec3(0);

void playerView::logic(gameMain *game, float delta) {
	if (cam->velocity() != lastvel) {
		cameraPhys->setAcceleration(cam->velocity());
		lastvel = cam->velocity();
	}

	game->phys->stepSimulation(1.f/game->frame_timer.last());
	cam->setPosition(cameraObj->transform.position - 5.f*cam->direction());

	glm::vec3 vel = cameraPhys->getVelocity();
	cameraObj->transform.rotation =
		glm::quat(glm::vec3(0, atan2(vel.x, vel.z), 0));

	/*
	//cam->setPosition(cameraObj->transform.position + glm::vec3(0, 1.5, 0));

	cameraObj->transform.rotation = glm::quat(glm::vec3(
		// pitch, yaw, roll
		cam->direction().y*-0.5f,
		atan2(cam->direction().x, cam->direction().z),
		0));
		*/

}

static void drawUIStuff(NVGcontext *vg, int wx, int wy) {
	Framebuffer().bind();
	set_default_gl_flags();

	disable(GL_DEPTH_TEST);
	disable(GL_SCISSOR_TEST);
	glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	nvgBeginFrame(vg, wx, wy, 1.0);
	nvgSave(vg);

	float ticks = SDL_GetTicks() / 1000.f;

	nvgBeginPath(vg);
	nvgRoundedRect(vg, 48, 35, 16, 42, 3);
	nvgRoundedRect(vg, 35, 48, 42, 16, 3);
	nvgFillColor(vg, nvgRGBA(172, 16, 16, 192));
	nvgFill(vg);

	nvgRotate(vg, 0.1*cos(ticks));
	nvgBeginPath(vg);
	nvgRect(vg, 90, 44, 256, 24);
	nvgFillColor(vg, nvgRGBA(30, 30, 30, 127));
	nvgFill(vg);

	nvgBeginPath(vg);
	nvgRect(vg, 93, 47, 128 + 120*sin(ticks), 20);
	nvgFillColor(vg, nvgRGBA(192, 32, 32, 127));
	nvgFill(vg);

	nvgRotate(vg, -0.1*cos(ticks));
	nvgBeginPath(vg);
	nvgRoundedRect(vg, wx - 250, 50, 200, 100, 10);
	nvgFillColor(vg, nvgRGBA(28, 30, 34, 192));
	nvgFill(vg);

	nvgFontSize(vg, 16.f);
	nvgFontFace(vg, "sans-bold");
	nvgFontBlur(vg, 0);
	nvgTextAlign(vg, NVG_ALIGN_LEFT);
	nvgFillColor(vg, nvgRGBA(0xf0, 0x60, 0x60, 160));
	nvgText(vg, wx - 82, 80, "❎", NULL);
	nvgFillColor(vg, nvgRGBA(220, 220, 220, 160));
	nvgText(vg, wx - 235, 80, "💚 Testing this", NULL);
	nvgText(vg, wx - 235, 80 + 16, "Go forward ➡", NULL);
	nvgText(vg, wx - 235, 80 + 32, "⬅ Go back", NULL);

	nvgRestore(vg);
	nvgEndFrame(vg);
}

void playerView::drawMainMenu(int wx, int wy) {
	int center = wx/2;
	vgui.newFrame(wx, wy);
	vgui.menuBegin(center - 150, 100, 300, "💚 Main Menu lmao");
	if (vgui.menuEntry("New game", &menuSelect)) {
		if (vgui.clicked()) {
			input.mode = modes::Move;

		} else if (vgui.hovered()) {
			menuSelect = vgui.menuCount();
		}
	}

	if (vgui.menuEntry("Continue", &menuSelect)) {
		if (vgui.clicked()) {
			// TODO:

		} else if (vgui.hovered()) {
			menuSelect = vgui.menuCount();
		}
	}

	if (vgui.menuEntry("Settings", &menuSelect)) {
		if (vgui.clicked()) {
			// TODO:

		} else if (vgui.hovered()) {
			menuSelect = vgui.menuCount();
		}
	}

	if (vgui.menuEntry("Quit", &menuSelect)) {
		if (vgui.clicked()) {
			// TODO:

		} else if (vgui.hovered()) {
			menuSelect = vgui.menuCount();
		}
	}

	vgui.menuEnd();
	vgui.endFrame();
}

void playerView::render(gameMain *game) {
	int winsize_x, winsize_y;
	SDL_GetWindowSize(game->ctx.window, &winsize_x, &winsize_y);

	if (input.mode == modes::MainMenu) {
		renderWorld(game, cam);

		// TODO: need to set post size on resize event..
		//post->setSize(winsize_x, winsize_y);
		post->draw(game->rend->framebuffer);

		// TODO: function to do this
		drawMainMenu(winsize_x, winsize_y);

	} else {
		renderWorld(game, cam);
		post->draw(game->rend->framebuffer);

		// TODO: function to do this
		drawUIStuff(vgui.nvg, winsize_x, winsize_y);
	}
}
