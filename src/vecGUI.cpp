#include <grend/vecGUI.hpp>
#include <grend/openglIncludes.hpp>
#include <grend/glManager.hpp>
#include <grend/sdlContext.hpp>
#include <assert.h>

using namespace grendx;

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

vecGUI::vecGUI() {
#if GLSL_VERSION < 300
	nvg = nvgCreateGLES2(NVG_ANTIALIAS);
#elif GLSL_VERSION == 300
	nvg = nvgCreateGLES3(NVG_ANTIALIAS);
#else
	// TODO: flag to enable/disable stencil strokes
	//nvg = nvgCreateGL3(NVG_ANTIALIAS | NVG_STENCIL_STROKES);
	nvg = nvgCreateGL3(NVG_ANTIALIAS);
#endif

	assert(nvg != nullptr);
	fonts.normal = nvgCreateFont(nvg, "sans",
		GR_PREFIX "assets/fonts/Roboto-Regular.ttf");
	fonts.bold = nvgCreateFont(nvg, "sans-bold",
		GR_PREFIX "assets/fonts/Roboto-Bold.ttf");
	fonts.emoji = nvgCreateFont(nvg, "emoji",
		GR_PREFIX "assets/fonts/NotoEmoji-Regular.ttf");

	nvgAddFallbackFontId(nvg, fonts.normal, fonts.emoji);
	nvgAddFallbackFontId(nvg, fonts.bold, fonts.emoji);
}

vecGUI::~vecGUI() {
	// TODO: clean up
}

void vecGUI::newFrame(int wx, int wy) {
	// XXX: should this be a seperate function? Or should the caller
	//      be responsible for setting this up
	Framebuffer().bind();
	setDefaultGlFlags();

	disable(GL_DEPTH_TEST);
	disable(GL_SCISSOR_TEST);
	glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	// update mouse state
	lastx = mx;
	lasty = my;
	lastbuttons = buttons;
	buttons = SDL_GetMouseState(&mx, &my);
	is_updated = mx != lastx || my != lasty || buttons != lastbuttons;

	nvgBeginFrame(nvg, wx, wy, 1.0);
	nvgSave(nvg);
}

void vecGUI::endFrame(void) {
	nvgRestore(nvg);
	nvgEndFrame(nvg);
}

void vecGUI::drawRect(int x, int y, int w, int h, NVGcolor color, int r) {
	nvgBeginPath(nvg);
	if (r) {
		nvgRoundedRect(nvg, x, y, w, h, r);

	} else {
		nvgRect(nvg, x, y, w, h);
	}

	nvgFillColor(nvg, color);
	nvgFill(nvg);
}

void vecGUI::drawText(int x, int y, float size, NVGcolor color,
                      const char *str, enum alignment align)
{
	switch (align) {
		case alignment::Left:   nvgTextAlign(nvg, NVG_ALIGN_LEFT); break;
		case alignment::Center: nvgTextAlign(nvg, NVG_ALIGN_CENTER); break;
		case alignment::Right:  nvgTextAlign(nvg, NVG_ALIGN_RIGHT); break;
	}

	nvgFontSize(nvg, size);
	nvgFontFace(nvg, "sans-bold");
	nvgFontBlur(nvg, 0);
	nvgFillColor(nvg, color);
	nvgText(nvg, x, y, str, NULL);
}


bool vecGUI::menuBegin(int x, int y, int w, const char *title) {
	menu.title = title;
	menu.x = x;
	menu.y = y;
	menu.width = w;
	menu.entries.clear();
	menu.is_active = true;

	return menu.is_active;
}

bool vecGUI::menuEntry(const char *msg, int *select) {
	int lowx = menu.x + 5;
	int lowy = menu.y + (menu.entries.size()+2)*32;
	int hix  = menu.x + menu.width - 10;
	int hiy  = lowy + 32;

	bool selected = select && *select == menu.entries.size();
	menu.entries.push_back({msg, selected});

	is_hovered = is_updated && mx >= lowx && mx < hix && my >= lowy && my < hiy; 
	is_clicked = is_hovered && !!(buttons & SDL_BUTTON(SDL_BUTTON_LEFT));

	return menu.is_active;
}

bool vecGUI::menuEnd(void) {
	drawRect(menu.x, menu.y, menu.width, (menu.entries.size()+3)*32,
	         theme.background, 10);
	drawText(menu.x, menu.y + 32, 32.f, theme.text, menu.title);

	for (size_t i = 0; i < menu.entries.size(); i++) {
		auto& [title, selected] = menu.entries[i];
		if (selected) {
			drawRect(menu.x + 5, menu.y + 32.f*(i+2), menu.width - 10, 36.f,
			         theme.selected, 10);
		}

		drawText(menu.x + 10, menu.y + 32.f*(i+3), 32.f, theme.text, title);
	}

	return menu.is_active;
}

bool vecGUI::clicked(void) {
	return is_clicked;
}

bool vecGUI::hovered(void) {
	return is_hovered;
}

int vecGUI::menuCount(void) {
	return menu.entries.size() - 1;
}
