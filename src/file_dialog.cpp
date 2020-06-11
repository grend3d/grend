#include <grend/file_dialog.hpp>

#include <iostream>

#define IMGUI_IMPL_OPENGL_LOADER_GLEW
#include <imgui/imgui.h>
#include <imgui/examples/imgui_impl_sdl.h>
#include <imgui/examples/imgui_impl_opengl3.h>

using namespace grendx;

bool file_dialog::prompt_filename(void) {
	if (selected) return true;
	if (!active) return false;

	ImGui::Begin(title.c_str(), &active);
	if (ImGui::InputText("Path", current_dir, FD_PATH_MAX)) {
		std::cout << "Input path changed" << std::endl;
	}
	ImGui::Columns(2);

	for (unsigned i = 0; i < current_dir_contents.size(); i++) {
		ImGui::Separator();
		if (ImGui::Selectable(current_dir_contents[i].c_str(), cursor_pos == i)) {
			cursor_pos = i;
		}
		ImGui::NextColumn();

		ImGui::Text("1234PiB");
		ImGui::NextColumn();
	}

	ImGui::Columns(1);

	if (ImGui::Button("Cancel")) {
		// TODO
		active = false;
	}

	ImGui::SameLine();
	if (ImGui::Button("OK")) {
		selected = true;
		active = false;
	}

	ImGui::End();
	return selected;
}

void file_dialog::show(void) {
	active = true;
	selected = false;
}

void file_dialog::clear(void) {
	active = selected = false;
}
