#include "UI.hpp"
#include "inputHandler.hpp"
#include "healthbar.hpp"

void drawPlayerHealthbar(entityManager *manager,
                         vecGUI&vgui,
                         health *playerHealth)
{
	int wx = manager->engine->rend->screen_x;
	int wy = manager->engine->rend->screen_y;

	nvgBeginPath(vgui.nvg);
	nvgRoundedRect(vgui.nvg, 48, 35, 16, 42, 3);
	nvgRoundedRect(vgui.nvg, 35, 48, 42, 16, 3);
	nvgFillColor(vgui.nvg, nvgRGBA(172, 16, 16, 192));
	nvgFill(vgui.nvg);

	//nvgRotate(vgui.nvg, 0.1*cos(ticks));
	nvgBeginPath(vgui.nvg);
	nvgRect(vgui.nvg, 90, 44, 256, 24);
	nvgFillColor(vgui.nvg, nvgRGBA(30, 30, 30, 127));
	nvgFill(vgui.nvg);

	nvgBeginPath(vgui.nvg);
	nvgRect(vgui.nvg, 93, 47, 252*playerHealth->amount, 20);
	nvgFillColor(vgui.nvg, nvgRGBA(192, 32, 32, 127));
	nvgFill(vgui.nvg);
	//nvgRotate(vgui.nvg, -0.1*cos(ticks));

	nvgFontSize(vgui.nvg, 16.f);
	nvgFontFace(vgui.nvg, "sans-bold");
	nvgFontBlur(vgui.nvg, 0);
	nvgTextAlign(vgui.nvg, NVG_ALIGN_LEFT);

	double fps = manager->engine->frame_timer.average();
	std::string fpsstr = std::to_string(fps) + "fps";
	nvgFillColor(vgui.nvg, nvgRGBA(0xf0, 0x60, 0x60, 0xff));
	nvgText(vgui.nvg, wx/2, 80 + 32, fpsstr.c_str(), NULL);
}

void renderObjectives(entityManager *manager,
                      levelController *level,
                      vecGUI& vgui)
{
	int wx = manager->engine->rend->screen_x;
	int wy = manager->engine->rend->screen_y;

	nvgBeginPath(vgui.nvg);
	nvgRoundedRect(vgui.nvg, wx - 250, 50, 200, 100, 10);
	nvgFillColor(vgui.nvg, nvgRGBA(28, 30, 34, 192));
	nvgFill(vgui.nvg);

	nvgFontSize(vgui.nvg, 16.f);
	nvgFontFace(vgui.nvg, "sans-bold");
	nvgFontBlur(vgui.nvg, 0);
	nvgTextAlign(vgui.nvg, NVG_ALIGN_LEFT);
	nvgFillColor(vgui.nvg, nvgRGBA(0xf0, 0x60, 0x60, 160));
	nvgText(vgui.nvg, wx - 82, 80, "❎", NULL);
	nvgFillColor(vgui.nvg, nvgRGBA(220, 220, 220, 160));
	nvgText(vgui.nvg, wx - 235, 80, "💚 Objectives: ", NULL);
	/*
	nvgText(vgui.nvg, wx - 235, 80 + 16, "Go forward ➡", NULL);
	nvgText(vgui.nvg, wx - 235, 80 + 32, "⬅ Go back", NULL);
	*/

	unsigned i = 1;
	for (auto& [desc, completed] : level->objectivesCompleted) {
		std::string foo = (completed? "➡" : "❎") + desc;

		nvgText(vgui.nvg, wx - 235, 80 + (i++ * 16), foo.c_str(), NULL);
	}
}

void renderHealthbars(entityManager *manager,
                      vecGUI& vgui,
                      camera::ptr cam)
{
	std::set<entity*> ents = searchEntities(manager, {"health", "healthbar"});
	std::set<entity*> players = searchEntities(manager, {"player", "health"});

	for (auto& ent : ents) {
		healthbar *bar;
		castEntityComponent(bar, manager, ent, "healthbar");

		if (bar) {
			bar->draw(manager, ent, vgui, cam);
		}
	}

	if (players.size() > 0) {
		entity *ent = *players.begin();
		health *playerHealth;

		castEntityComponent(playerHealth, manager, ent, "health");

		if (playerHealth) {
			drawPlayerHealthbar(manager, vgui, playerHealth);
		}
	}
}

void renderControls(gameMain *game, vecGUI& vgui) {
	// TODO: should have a generic "pad" component
	// assume first instances of these components are the ones we want,
	// since there should only be 1 of each
	auto& movepads   = game->entities->getComponents("touchMovementHandler");
	auto& actionpads = game->entities->getComponents("touchRotationHandler");
	touchMovementHandler *movepad;
	touchRotationHandler *actionpad;

	if (movepads.size() == 0 || actionpads.size() == 0) {
		return;
	}

	movepad   = dynamic_cast<touchMovementHandler*>(*movepads.begin());
	actionpad = dynamic_cast<touchRotationHandler*>(*actionpads.begin());

	// left movement pad
	nvgStrokeWidth(vgui.nvg, 2.0);
	nvgBeginPath(vgui.nvg);
	nvgCircle(vgui.nvg, movepad->center.x + movepad->touchpos.x,
	                    movepad->center.y + movepad->touchpos.y,
	                    movepad->radius / 3.f);
	nvgFillColor(vgui.nvg, nvgRGBA(0x60, 0x60, 0x60, 0x80));
	nvgStrokeColor(vgui.nvg, nvgRGBA(255, 255, 255, 192));
	nvgStroke(vgui.nvg);
	nvgFill(vgui.nvg);

	nvgStrokeWidth(vgui.nvg, 2.0);
	nvgBeginPath(vgui.nvg);
	nvgCircle(vgui.nvg, movepad->center.x, movepad->center.y, movepad->radius);
	nvgStrokeColor(vgui.nvg, nvgRGBA(0x60, 0x60, 0x60, 0x40));
	nvgStroke(vgui.nvg);

	// right action pad
	nvgStrokeWidth(vgui.nvg, 2.0);
	nvgBeginPath(vgui.nvg);
	nvgCircle(vgui.nvg, actionpad->center.x + actionpad->touchpos.x,
	                    actionpad->center.y + actionpad->touchpos.y,
	                    actionpad->radius / 3.f);
	nvgFillColor(vgui.nvg, nvgRGBA(0x60, 0x60, 0x60, 0x80));
	nvgStrokeColor(vgui.nvg, nvgRGBA(255, 255, 255, 192));
	nvgStroke(vgui.nvg);
	nvgFill(vgui.nvg);

	nvgStrokeWidth(vgui.nvg, 2.0);
	nvgBeginPath(vgui.nvg);
	nvgCircle(vgui.nvg, actionpad->center.x, actionpad->center.y, actionpad->radius);
	nvgStrokeColor(vgui.nvg, nvgRGBA(0x60, 0x60, 0x60, 0x40));
	nvgStroke(vgui.nvg);
}
