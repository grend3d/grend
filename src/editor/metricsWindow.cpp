#include <grend/gameEditor.hpp>

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_sdl.h>
#include <imgui/backends/imgui_impl_opengl3.h>

using namespace grendx;

void gameEditor::metricsWindow(gameMain *game) {
	ImGui::Begin("Engine metrics", &showMetricsWindow);

	std::pair<float, float> minmax = {0, 0}; // TODO: implementation
	float fps = game->frame_timer.average();
	std::string fpsStr =
		std::to_string(fps) + " FPS "
		+ "(" + std::to_string(1.f/fps * 1000) + "ms/frame) "
		;

		/*
		+ "(min: " + std::to_string(minmax.first) + ", "
		+ "max: " + std::to_string(minmax.second) + ")"
		*/

	std::string meshes =
		std::to_string(game->metrics.drawnMeshes)
		+ " meshes drawn";

	float bufmb = dbgGlmanBuffered/1048576.f;
	float texmb = dbgGlmanTexturesBuffered/1048576.f;
	float totalmb = bufmb + texmb;

	std::string buffered = "Buffers: " + std::to_string(bufmb) + "MiB";
	std::string textures = "Textures: " + std::to_string(texmb) + "MiB";
	std::string total = "Total: " + std::to_string(totalmb) + "MiB";

#if GREND_ERROR_CHECK
	std::string devbuild = "Debug build";
	if (GL_ERROR_CHECK_ENABLED()) {
		devbuild += " (debugging checks enabled)";
	}

	ImGui::TextColored(ImVec4(0.3f, 0.8f, 0.2f, 1.f), "%s", devbuild.c_str());
#endif

	ImGui::Text("%s", fpsStr.c_str());
	ImGui::Text("%s", meshes.c_str());
	ImGui::Text("%s", buffered.c_str());
	ImGui::Text("%s", textures.c_str());
	ImGui::Text("%s", total.c_str());

	ImGui::End();
}
