#pragma once

#include <list>
#include <string>
#include <vector>

namespace grendx {

class file_dialog {
	public:
		file_dialog(std::string t = "Select File") : title(t) {};
		bool prompt_filename(void);
		void show(void);
		void clear(void);

		bool selected = false;
		std::string selection;
		std::string title;

		std::vector<std::string> current_dir_contents = {
			"Foo",
			"Bar",
			"Bizz",
			"Quix"
		};

	private:
		bool active = false;
		int cursor_pos = -1;

		enum { FD_PATH_MAX = 256, };
		char current_dir[FD_PATH_MAX];
};

// namespace grendx
}
