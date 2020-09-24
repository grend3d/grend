#include <grend/playerView.hpp>
#include <grend/gameView.hpp>
#include <grend/audioMixer.hpp>
#include <grend/controllers.hpp>
#include <glm/gtx/rotate_vector.hpp>

#if GLSL_VERSION < 300
// GLES2
#define NANOVG_GLES2_IMPLEMENTATION
#elif GLSL_VERSION == 300
// GLES3
#define NANOVG_GLES3_IMPLEMENTATION
#else
// GL3+ core
#define NANOVG_GL3_IMPLEMENTATION
#endif

#include <nanovg/src/nanovg.h>
#include <nanovg/src/nanovg_gl.h>
#include <nanovg/src/nanovg_gl_utils.h>

using namespace grendx;

static renderPostStage<rOutput>::ptr testpost = nullptr;
static gameObject::ptr testweapon = nullptr;

static channelBuffers_ptr weaponSound =
	openAudio(GR_PREFIX "assets/sfx/impact.ogg");

struct nvg_data {
	int fontNormal, fontBold, fontIcons, fontEmoji;
};

playerView::playerView(gameMain *game) : gameView() {
	static const float speed = 15.f;
#if GLSL_VERSION < 300
	//vg = nvgCreateGLES2(NVG_ANTIALIAS | NVG_STENCIL_STROKES | NVG_DEBUG);
#else
	//vg = nvgCreateGLES3(NVG_ANTIALIAS | NVG_STENCIL_STROKES | NVG_DEBUG);
	vg = nvgCreateGL3(NVG_ANTIALIAS);
#endif
	assert(vg != nullptr);
	struct nvg_data temp;
	temp.fontNormal = nvgCreateFont(vg, "sans", "libs/nanovg/example/Roboto-Regular.ttf");
	temp.fontBold = nvgCreateFont(vg, "sans-bold", "libs/nanovg/example/Roboto-Bold.ttf");
	temp.fontEmoji = nvgCreateFont(vg, "emoji", "libs/nanovg/example/NotoEmoji-Regular.ttf");
	nvgAddFallbackFontId(vg, temp.fontNormal, temp.fontEmoji);
	nvgAddFallbackFontId(vg, temp.fontBold, temp.fontEmoji);

	cameraPhysID = game->phys->add_sphere(cameraObj, glm::vec3(0, 10, 0), 1.0, 1.0);
	setNode("player camera", game->state->physObjects, cameraObj);

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

void playerView::logic(gameMain *game, float delta) {
	// TODO: cam->update(delta);
	/*
	cam->position += cam->velocity.z*cam->direction*delta;
	cam->position += cam->velocity.y*cam->up*delta;
	cam->position += cam->velocity.x*cam->right*delta;
	*/
	game->phys->set_acceleration(cameraPhysID, cam->velocity);
	cam->position = cameraObj->transform.position + glm::vec3(0, 1.5, 0);
	cameraObj->transform.rotation = glm::quat(glm::vec3(
		// pitch, yaw, roll
		cam->direction.y*-0.5f,
		atan2(cam->direction.x, cam->direction.z),
		0));
}

static void drawUIStuff(NVGcontext *vg, int wx, int wy) {
	Framebuffer().bind();
	glEnable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_SCISSOR_TEST);
	glEnable(GL_CULL_FACE);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
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
	nvgFillColor(vg, nvgRGBA(220, 220, 220, 160));
	nvgTextAlign(vg, NVG_ALIGN_LEFT);
	nvgText(vg, wx - 235, 80, "💚 Testing this", NULL);
	nvgText(vg, wx - 235, 80 + 16, "Go forward ➡", NULL);
	nvgText(vg, wx - 235, 80 + 32, "⬅ Go back", NULL);

	nvgRestore(vg);
	nvgEndFrame(vg);
}

void playerView::drawMainMenu(int wx, int wy) {
	static int lastx, lasty;
	static int mx = -1, my = -1;
	static Uint32 lastbuttons, buttons;

	lastx = mx;
	lasty = my;
	lastbuttons = buttons;
	buttons = SDL_GetMouseState(&mx, &my);

	Framebuffer().bind();

	glEnable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_SCISSOR_TEST);
	glEnable(GL_CULL_FACE);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	nvgBeginFrame(vg, wx, wy, 1.0);
	nvgSave(vg);
	int center = wx/2;

	nvgBeginPath(vg);
	nvgRoundedRect(vg, center - 160, 64, 320, 192 + 32, 10);
	nvgFillColor(vg, nvgRGBA(20, 20, 20, 192));
	nvgFill(vg);

	nvgTextAlign(vg, NVG_ALIGN_CENTER);
	nvgFontSize(vg, 32.f);
	nvgFontFace(vg, "sans-bold");
	nvgFontBlur(vg, 0);
	nvgFillColor(vg, nvgRGBA(0xf0, 0xf0, 0xf0, 192));
	nvgText(vg, center, 100, "💚 Main Menu lmao", NULL);

	int count = 0;
	auto drawEnt = [&](const char *msg) {
		bool clicked = false;
		int lowx = center - 150;
		int hix  = center + 150;
		int lowy = 100 + count*32 + 2;
		int hiy  = lowy + 36;

		if (menuSelect == count) {
			nvgBeginPath(vg);
			nvgRoundedRect(vg, lowx, lowy, 300, 36, 10);
			nvgFillColor(vg, nvgRGBA(28, 30, 34, 192));
			nvgFill(vg);

			nvgFillColor(vg, nvgRGBA(0xf0, 0xf0, 0xf0, 192));

		} else {
			nvgFillColor(vg, nvgRGBA(0xd0, 0xd0, 0xd0, 192));
		}

		bool updated = mx != lastx || my != lasty || buttons != lastbuttons;
		if (updated && mx >= lowx && mx < hix && my >= lowy && my < hiy) {
			menuSelect = count;
			clicked = !!(buttons & SDL_BUTTON(SDL_BUTTON_LEFT));
		}

		nvgText(vg, center, 100 + (count+1)*32, msg, NULL);
		count++;

		return clicked;
	};

	if (drawEnt("New game")) {
		input.mode = modes::Move;
	}

	if (drawEnt("Continue")) {
		// TODO:
	}

	if (drawEnt("Settings")) {
		// TODO:
	}

	if (drawEnt("Quit")) {
		// TODO:
	}

	nvgRestore(vg);
	nvgEndFrame(vg);
}

void playerView::render(gameMain *game) {
	int winsize_x, winsize_y;
	SDL_GetWindowSize(game->ctx.window, &winsize_x, &winsize_y);

	if (!testpost) {
		// XXX: keep postprocessing chain in renderer class
		testpost = makePostprocessor<rOutput>(game->rend->shaders["post"],
				SCREEN_SIZE_X, SCREEN_SIZE_Y);
	}

	if (input.mode == modes::MainMenu) {
		renderWorld(game, cam);

		// TODO: function to do this
		testpost->setSize(winsize_x, winsize_y);
		testpost->draw(game->rend->framebuffer);
		drawMainMenu(winsize_x, winsize_y);

	} else {
		renderWorld(game, cam);

		// TODO: function to do this
		testpost->setSize(winsize_x, winsize_y);
		testpost->draw(game->rend->framebuffer);
		drawUIStuff(vg, winsize_x, winsize_y);
	}
}
