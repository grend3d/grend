#pragma once

#include <nanovg/src/nanovg.h>
#include <vector>
#include <utility>
#include <stdint.h>

namespace grendx {

class vecGUI {
	public:
		enum alignment {
			Left,
			Center,
			Right,
		};

		vecGUI();
		~vecGUI();

		void newFrame(int wx, int wy);
		void endFrame(void);
		void drawRect(int x, int y, int w, int h, NVGcolor color, int r=0);
		void drawText(int x, int y, float size, NVGcolor color,
		              const char *str, enum alignment align = alignment::Left);

		bool menuBegin(int x, int y, int w, const char *title);
		bool menuEnd(void);
		bool menuEntry(const char *msg, int *select = nullptr);
		int menuCount(void);
		bool clicked(void);
		bool hovered(void);

		NVGcontext *nvg;

		struct theme {
			NVGcolor background = nvgRGBA(20, 20, 20, 192);
			NVGcolor text = nvgRGBA(0xf0, 0xf0, 0xf0, 192);
			NVGcolor active= nvgRGBA(0xf0, 0xf0, 0xf0, 192);
			NVGcolor inactive = nvgRGBA(0xc0, 0xc0, 0xc0, 192);
			NVGcolor selected = nvgRGBA(28, 30, 24, 192);
		} theme;

	private:
		bool is_clicked = false;
		bool is_hovered = false;
		bool is_updated = false;

		int lastx, lasty;
		int mx = -1, my = -1;
		uint32_t lastbuttons, buttons;

		struct {
			int normal, bold, emoji;
		} fonts;

		struct {
			int x, y, width;
			const char *title;
			std::vector<std::pair<const char *, bool>> entries;
			bool is_active;
		} menu;
};

// namespace grendx
}
