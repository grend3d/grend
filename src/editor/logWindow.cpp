#include <grend/gameEditor.hpp>

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_sdl.h>
#include <imgui/backends/imgui_impl_opengl3.h>

using namespace grendx;

void gameEditorUI::logWindow() {
	static bool autoscroll = true;

	ImGui::SetNextWindowSize(ImVec2(480, 600), ImGuiCond_FirstUseEver);
	ImGui::Begin("Engine log", &showLogWindow);
	ImGui::Checkbox("Autoscroll", &autoscroll);

	ImGui::BeginChild("Scroll");
	for (auto& s : editor->logEntries) {
		ImGui::TextUnformatted(s.c_str());
	}

	if (autoscroll) {
		ImGui::SetScrollHereY(1.0f);
	}
	ImGui::EndChild();

	ImGui::End();
}

