#include "healthbar.hpp"
#include "health.hpp"
#include <grend/interpolation.hpp>

using namespace grendx;
using namespace grendx::ecs;

void worldHealthbar::draw(entityManager *manager, entity *ent,
                          vecGUI& vgui, camera::ptr cam)
{
	health *entHealth = castEntityComponent<health*>(manager, ent, "health");
	// TODO: maybe an error message or something here
	if (!entHealth)
		return;

	float ticks = SDL_GetTicks() / 1000.f;
	glm::vec3 entpos = ent->getNode()->transform.position + glm::vec3(0, 3, 0);
	glm::vec4 screenpos = cam->worldToScreenPosition(entpos);

	if (entHealth->amount < 1.0 && cam->onScreen(screenpos)) {
		// TODO: some sort of grid editor wouldn't be too hard,
		//       probably worthwhile for quick UIs
		float depth = 16*max(0.f, screenpos.w);
		float pad = depth*8.f;

		float width  = 8*pad;
		float height = 3*pad;

		screenpos.y  = 1.0 - screenpos.y;
		screenpos.x *= manager->engine->rend->screen_x;
		screenpos.y *= manager->engine->rend->screen_y;

		glm::vec2 outer    = glm::vec2(screenpos) - glm::vec2(width, height)*0.5f;
		glm::vec2 innermin = outer + pad;
		glm::vec2 innermax = outer + glm::vec2(width, height) - 2*pad;

		nvgFontSize(vgui.nvg, pad);
		nvgFontFace(vgui.nvg, "sans-bold");
		nvgFontBlur(vgui.nvg, 0);
		nvgTextAlign(vgui.nvg, NVG_ALIGN_LEFT);

		nvgBeginPath(vgui.nvg);
		nvgRect(vgui.nvg, outer.x, outer.y, width, height);
		nvgFillColor(vgui.nvg, nvgRGBA(28, 30, 34, 192));
		nvgFill(vgui.nvg);

		//float amount = sin(i*ticks)*0.5 + 0.5;
		//float amount = entHealth->amount;
		lastAmount = interp::average(entHealth->amount, lastAmount, 16.f, 1.f/60);
		nvgBeginPath(vgui.nvg);
		nvgRect(vgui.nvg, innermin.x, innermin.y, lastAmount*(width - 2*pad), pad);
		nvgFillColor(vgui.nvg, nvgRGBA(0, 192, 0, 192));
		nvgFill(vgui.nvg);

		nvgBeginPath(vgui.nvg);
		nvgRect(vgui.nvg, innermin.x + lastAmount*(width - 2*pad),
		        innermin.y, (1.f - lastAmount)*(width - 2*pad), pad);
		nvgFillColor(vgui.nvg, nvgRGBA(192, 0, 0, 192));
		nvgFill(vgui.nvg);

		nvgFillColor(vgui.nvg, nvgRGBA(0xf0, 0x60, 0x60, 160));

		/*
		std::string cur  = std::to_string(int(entHealth->amount * entHealth->hp));
		std::string maxh = std::to_string(int(entHealth->hp));
		std::string curstr = "❎ " + cur + "/" + maxh;

		nvgFillColor(vgui.nvg, nvgRGBA(220, 220, 220, 160));
		nvgText(vgui.nvg, innermin.x, innermin.y + 2*pad, "💚 Enemy", NULL);
		nvgText(vgui.nvg, innermin.x, innermin.y + 3*pad, "❎ Some stats", NULL);
		nvgText(vgui.nvg, innermin.x, innermin.y + 4*pad, curstr.c_str(), NULL);
		*/
	}
}
