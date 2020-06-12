#pragma once

#include <list>
#include <string>
#include <vector>

#include <string.h>

namespace grendx {

class file_dialog {
	public:
		enum ent_type {
			Directory,
			Model,
			Map,
			Image,
			File,
		};

		struct f_dirent {
			std::string name;
			size_t size;
			enum ent_type type;
		};

		file_dialog(std::string t = "Select File") : title(t) {
			current_dir[0] = '\0';
			chdir(".");
		};

		bool prompt_filename(void);
		void show(void);
		void clear(void);
		void chdir(std::string dir);
		void listdir(void);
		void handle_doubleclicken(struct f_dirent& ent);
		void select(struct f_dirent& ent);

		bool selected = false;
		std::string selection;
		std::string title;

	private:
		bool active = false;
		int cursor_pos = -1;

		enum { FD_PATH_MAX = 256, };
		char current_dir[FD_PATH_MAX];
		std::vector<struct f_dirent> dir_contents;
};

// namespace grendx
}
