#include <grend/gameEditor.hpp>

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_sdl.h>
#include <imgui/backends/imgui_impl_opengl3.h>

using namespace grendx;

void gameEditorUI::objectSelectWindow() {
	ImGui::Begin("Loaded Objects", &showObjectSelectWindow);

	// TODO: not actually used anymore, remove
#if 0
	// TODO: another way to select from loaded models
	for (auto it = models.begin(); it != models.end(); it++) {
		auto& [name, obj] = *it;

		if (ImGui::Selectable(name.c_str(), obj == editor->selectedNode)) {
			editor->selectedNode = obj;
		}

		ImGui::SameLine(300);
		//std::string mstr = "(" + std::to_string(obj->nodes.size()) + " meshes)";
		std::string mstr = "(TODO: reimplement?)";
		ImGui::Text("%s", mstr.c_str());
	}
#endif

	ImGui::End();
}
