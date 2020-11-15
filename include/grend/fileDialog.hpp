#pragma once

#include <list>
#include <string>
#include <vector>

#include <string.h>

namespace grendx {

class fileDialog {
	public:
		enum entType {
			Directory,
			Model,
			Map,
			Image,
			File,
		};

		struct f_dirent {
			std::string name;
			size_t size;
			enum entType type;
		};

		fileDialog(std::string t = "Select File") : title(t) {
			currentDir[0] = '\0';
			chdir(".");
		};

		bool promptFilename(void);
		void show(void);
		void clear(void);
		void chdir(std::string dir);
		void listdir(void);
		void handleDoubleclick(struct f_dirent& ent);
		void select(struct f_dirent& ent);

		bool selected = false;
		std::string selection;
		std::string title;

	private:
		bool active = false;
		int cursorPos = -1;

		enum { FD_PATH_MAX = 256, };
		char currentDir[FD_PATH_MAX];
		std::vector<struct f_dirent> dirContents;
};

// namespace grendx
}
