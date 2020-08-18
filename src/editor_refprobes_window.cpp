#include <grend/game_editor.hpp>

#define IMGUI_IMPL_OPENGL_LOADER_GLEW
#include <imgui/imgui.h>
#include <imgui/examples/imgui_impl_sdl.h>
#include <imgui/examples/imgui_impl_opengl3.h>

using namespace grendx;

void game_editor::refprobes_window(gameMain *game) {
	ImGui::Begin("Reflection probes", &show_refprobe_window);
	ImGui::Columns(2);

	// TODO: node tree selection
#if 0
	for (auto& [id, probe] : rend->ref_probes) {
		std::string name = "reflection probe " + std::to_string(id);

		ImGui::Separator();
		if (ImGui::Selectable(name.c_str(), selected_refprobe == (int)id)) {
			selected_refprobe = id;
		}

		ImGui::NextColumn();
		ImGui::InputFloat3("position", glm::value_ptr(probe.position));

		if (selected_refprobe == (int)id) {
			ImGui::Checkbox("static", &probe.is_static);
		}

		ImGui::NextColumn();
	}

	ImGui::Columns(1);
	ImGui::Separator();

	if (ImGui::Button("Add reflection probe")) {
		set_mode(mode::AddReflectionProbe);
	}

	ImGui::SameLine();
	if (ImGui::Button("Delete reflection probe")) {
		rend->free_reflection_probe(selected_refprobe);
	}
#endif

	ImGui::End();
}
