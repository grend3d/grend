#include <grend/game_editor.hpp>

#define IMGUI_IMPL_OPENGL_LOADER_GLEW
#include <imgui/imgui.h>
#include <imgui/examples/imgui_impl_sdl.h>
#include <imgui/examples/imgui_impl_opengl3.h>

using namespace grendx;

void game_editor::object_select_window(gameMain *game) {
	ImGui::Begin("Loaded Objects", &show_object_select_window);

	// TODO: another way to select from loaded models
	/*
	for (auto it = glman.cooked_models.begin();
	     it != glman.cooked_models.end();
	     it++)
	{
		auto& [name, obj] = *it;

		if (ImGui::Selectable(name.c_str(), it == edit_model)) {
			edit_model = it;
		}

		ImGui::SameLine(300);
		std::string mstr = "(" + std::to_string(obj.meshes.size()) + " meshes)";
		ImGui::Text(mstr.c_str());
	}
	*/

	ImGui::End();
}
