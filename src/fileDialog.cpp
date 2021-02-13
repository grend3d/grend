#include <grend/fileDialog.hpp>

#include <algorithm>
#include <iostream>

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_sdl.h>
#include <imgui/backends/imgui_impl_opengl3.h>

// TODO: windows-compatibility
#include <sys/types.h>
#include <dirent.h>

#include <filesystem>
namespace fs = std::filesystem;

using namespace grendx;

bool fileDialog::promptFilename(void) {
	if (selected) return true;
	if (!active) return false;

	ImGui::Begin(title.c_str(), &active);
	if (ImGui::Button("Up")) {
		chdir("..");
	}

	ImGui::SameLine();
	if (ImGui::InputText("Path", currentDir, FD_PATH_MAX)) {
		listdir();
	}

	for (int i = 0; i < dirContents.size(); i++) {
		auto& ent = dirContents[i];

		ImGui::Separator();
		if (ImGui::Selectable(ent.name.c_str(), cursorPos == i)) {
			if (cursorPos == i) {
				handleDoubleclick(ent);
			} else {
				cursorPos = i;
			}
		}

		ImGui::SameLine(250);
		std::string sz = std::to_string(ent.size);
		ImGui::Text("%s", sz.c_str());

		ImGui::SameLine(300);
		ImGui::Text("%s", (ent.type == entType::Directory)? "[DIR]" : "[FILE]");
	}

	//ImGui::Columns(1);
	ImGui::Separator();

	if (ImGui::Button("Cancel")) {
		// TODO
		active = false;
	}

	ImGui::SameLine();
	if (ImGui::Button("OK")) {
		if (cursorPos > -1) {
			select(dirContents[cursorPos]);
		}
	}

	ImGui::End();
	return selected;
}

void fileDialog::chdir(std::string dir) {
	// TODO: safe
	strcat(currentDir, dir.c_str());
	strcat(currentDir, "/");

	listdir();
}

void fileDialog::listdir(void) {
	if (fs::exists(currentDir) && fs::is_directory(currentDir)) {
		std::cerr << "Testing this" << std::endl;
		dirContents.clear();
		cursorPos = -1;

		for (auto& p : fs::directory_iterator(currentDir)) {
			dirContents.push_back({
				.name = p.path().filename(),
				//.size = fs::file_size(p.path()),
				.size = 0,
				.type = (fs::is_directory(p.path()))
						? entType::Directory
						: entType::File,
			});
		}

		std::sort(dirContents.begin(), dirContents.end(),
			[&] (struct f_dirent& a, struct f_dirent& b) {
				return (a.type != b.type)
					? a.type < b.type
					: a.name < b.name;
			});
	} else {
		std::cerr << "Invalid directory " << currentDir << std::endl;
	}
#if 0
	DIR *dirp;

	if ((dirp = opendir(currentDir))) {
		struct dirent *dent;
		dirContents.clear();
		cursorPos = -1;

		while ((dent = readdir(dirp))) {
			dirContents.push_back({
				.name = std::string(dent->d_name),
				.size = dent->d_reclen,
				.type = (dent->d_type == DT_DIR)
					? entType::Directory
					: entType::File,
			});
		}

		std::sort(dirContents.begin(), dirContents.end(),
			[&] (struct f_dirent& a, struct f_dirent& b) {
				return (a.type != b.type)
					? a.type < b.type
					: a.name < b.name;
			});
	}
#endif
}

void fileDialog::handleDoubleclick(struct f_dirent& ent) {
	switch (ent.type) {
		case entType::Directory:
			chdir(ent.name);
			break;

		default:
			select(ent);
			break;
	}
}

void fileDialog::select(struct f_dirent& ent) {
	selected = true;
	selection = std::string(currentDir) + "/" + ent.name;
}

void fileDialog::show(void) {
	active = true;
	selected = false;
}

void fileDialog::clear(void) {
	active = selected = false;
}
