#include <grend/gameEditor.hpp>

#include <imgui/imgui.h>
#include <imgui/examples/imgui_impl_sdl.h>
#include <imgui/examples/imgui_impl_opengl3.h>

using namespace grendx;

void game_editor::metricsWindow(gameMain *game) {
	ImGui::Begin("Engine metrics", &show_metrics_window);

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

	ImGui::Text(fpsStr.c_str());
	ImGui::Text(meshes.c_str());
	ImGui::Text(buffered.c_str());
	ImGui::Text(textures.c_str());
	ImGui::Text(total.c_str());

	ImGui::End();
}
