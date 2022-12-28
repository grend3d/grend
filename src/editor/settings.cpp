#include <grend/gameEditor.hpp>
#include <grend/gameState.hpp>

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_sdl.h>
#include <imgui/backends/imgui_impl_opengl3.h>

using namespace grendx;
using namespace grendx::engine;

void gameEditor::settingsWindow() {
	static renderSettings settings;
	static bool showSteps = true;

	auto rend  = Resolve<renderContext>();
	auto state = Resolve<gameState>();

	ImGui::Begin("Render settings", &showSettingsWindow);
	ImGui::Checkbox("Shadows enabled", &settings.shadowsEnabled);
	ImGui::InputScalar("Shadow size", ImGuiDataType_U32, &settings.shadowSize, &showSteps);
	ImGui::InputScalar("Shadow atlas size", ImGuiDataType_U32, &settings.shadowAtlasSize, &showSteps);

	ImGui::Checkbox("Reflections enabled", &settings.reflectionsEnabled);
	ImGui::InputScalar("Reflection size", ImGuiDataType_U32, &settings.reflectionSize, &showSteps);
	ImGui::InputScalar("Reflection atlas size", ImGuiDataType_U32, &settings.reflectionAtlasSize, &showSteps);

	ImGui::Checkbox("Light probes enabled", &settings.lightProbesEnabled);
	ImGui::InputScalar("Light probe size", ImGuiDataType_U32, &settings.lightProbeSize, &showSteps);
	ImGui::InputScalar("Light probe atlas size", ImGuiDataType_U32, &settings.lightProbeAtlasSize, &showSteps);

	ImGui::InputFloat("Scale X", &settings.scaleX);
	ImGui::InputFloat("Scale Y", &settings.scaleY);
	ImGui::InputScalar("Resolution (X)", ImGuiDataType_U32, &settings.targetResX, &showSteps);
	ImGui::InputScalar("Resolution (Y)", ImGuiDataType_U32, &settings.targetResY, &showSteps);
	ImGui::InputScalar("MSAA level", ImGuiDataType_U32, &settings.msaaLevel, &showSteps);
	ImGui::InputScalar("Anisotropic filtering samples", ImGuiDataType_U32, &settings.anisotropicFilterLevel, &showSteps);

	if (ImGui::Button("Apply")) {
		rend->applySettings(settings);
		invalidateLightMaps(state->rootnode);
	}

	ImGui::End();
}

