#include <grend/file_dialog.hpp>

#include <algorithm>
#include <iostream>

#include <imgui/imgui.h>
#include <imgui/examples/imgui_impl_sdl.h>
#include <imgui/examples/imgui_impl_opengl3.h>

// TODO: windows-compatibility
#include <sys/types.h>
#include <dirent.h>

using namespace grendx;

bool file_dialog::prompt_filename(void) {
	if (selected) return true;
	if (!active) return false;

	ImGui::Begin(title.c_str(), &active);
	if (ImGui::Button("Up")) {
		chdir("..");
	}

	ImGui::SameLine();
	if (ImGui::InputText("Path", current_dir, FD_PATH_MAX)) {
		listdir();
	}

	for (int i = 0; i < dir_contents.size(); i++) {
		auto& ent = dir_contents[i];

		ImGui::Separator();
		if (ImGui::Selectable(ent.name.c_str(), cursor_pos == i)) {
			if (cursor_pos == i) {
				handle_doubleclicken(ent);
			} else {
				cursor_pos = i;
			}
		}

		ImGui::SameLine(250);
		std::string sz = std::to_string(ent.size);
		ImGui::Text(sz.c_str());

		ImGui::SameLine(300);
		ImGui::Text((ent.type == ent_type::Directory)? "[DIR]" : "[FILE]");
	}

	//ImGui::Columns(1);
	ImGui::Separator();

	if (ImGui::Button("Cancel")) {
		// TODO
		active = false;
	}

	ImGui::SameLine();
	if (ImGui::Button("OK")) {
		if (cursor_pos > -1) {
			select(dir_contents[cursor_pos]);
		}
	}

	ImGui::End();
	return selected;
}

void file_dialog::chdir(std::string dir) {
	// TODO: safe
	strcat(current_dir, dir.c_str());
	strcat(current_dir, "/");

	listdir();
}

void file_dialog::listdir(void) {
	DIR *dirp;

	if ((dirp = opendir(current_dir))) {
		struct dirent *dent;
		dir_contents.clear();
		cursor_pos = -1;

		while ((dent = readdir(dirp))) {
			dir_contents.push_back({
				.name = std::string(dent->d_name),
				.size = dent->d_reclen,
				.type = (dent->d_type == DT_DIR)
					? ent_type::Directory
					: ent_type::File,
			});
		}

		std::sort(dir_contents.begin(), dir_contents.end(),
			[&] (struct f_dirent& a, struct f_dirent& b) {
				return (a.type != b.type)
					? a.type < b.type
					: a.name < b.name;
			});
	}
}

void file_dialog::handle_doubleclicken(struct f_dirent& ent) {
	switch (ent.type) {
		case ent_type::Directory:
			chdir(ent.name);
			break;

		default:
			select(ent);
			break;
	}
}

void file_dialog::select(struct f_dirent& ent) {
	selected = true;
	selection = std::string(current_dir) + "/" + ent.name;
}

void file_dialog::show(void) {
	active = true;
	selected = false;
}

void file_dialog::clear(void) {
	active = selected = false;
}
