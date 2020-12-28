#include <grend/playerView.hpp>
#include <grend/gameView.hpp>
#include <grend/audioMixer.hpp>
#include <grend/controllers.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <grend/geometryGeneration.hpp>

using namespace grendx;

static gameObject::ptr testweapon = nullptr;

static channelBuffers_ptr weaponSound =
	openAudio(GR_PREFIX "assets/sfx/impact.ogg");

static gameModel::ptr cuboid = generate_cuboid(1.0, 1.0, 1.0);

struct nvg_data {
	int fontNormal, fontBold, fontIcons, fontEmoji;
};

playerView::playerView(gameMain *game) : gameView() {
	/*
	static const float speed = 15.f;
	compileModel("physcuboid", cuboid);
	bindCookedMeshes();

	post = renderPostChain::ptr(new renderPostChain(
		{game->rend->postShaders["tonemap"], game->rend->postShaders["psaa"]},
		SCREEN_SIZE_X, SCREEN_SIZE_Y));

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

	input.setMode(modes::MainMenu);
		*/
}

static glm::vec3 lastvel = glm::vec3(0);

void playerView::logic(gameMain *game, float delta) {
	game->phys->stepSimulation(1.f/game->frame_timer.last());
	cam->updatePosition(delta);
}

/*
static void drawUIStuff(NVGcontext *vg, int wx, int wy) {
	Framebuffer().bind();
	setDefaultGlFlags();

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
*/

void playerView::render(gameMain *game) {
	/*
	int winsize_x, winsize_y;
	SDL_GetWindowSize(game->ctx.window, &winsize_x, &winsize_y);
	renderFlags flags = game->rend->getLightingFlags();

	renderWorld(game, cam, flags);
	post->draw(game->rend->framebuffer);

	if (input.mode == modes::MainMenu) {
		renderWorld(game, cam, flags);

		post->draw(game->rend->framebuffer);
		//drawMainMenu(winsize_x, winsize_y);

	} else {
		renderWorld(game, cam, flags);
		post->draw(game->rend->framebuffer);
		drawUIStuff(vgui.nvg, winsize_x, winsize_y);
	}
	*/
}
