#include <grend/gameEditor.hpp>
#include <grend/timers.hpp>

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_sdl.h>
#include <imgui/backends/imgui_impl_opengl3.h>

using namespace grendx;

static const ImGuiTreeNodeFlags base_flags
	= ImGuiTreeNodeFlags_OpenOnArrow
	| ImGuiTreeNodeFlags_OpenOnDoubleClick
	| ImGuiTreeNodeFlags_SpanAvailWidth;

static void drawProfilerGroups(gameMain *game,
                               profile::group *root,
                               profile::group *cur = nullptr,
                               std::string name = "frame")
{
	if (!cur) cur = root;

	ImGuiTreeNodeFlags flags
		= base_flags
		| ((cur->subgroups.size() == 0)? ImGuiTreeNodeFlags_Leaf : 0);

	double ms = cur->groupTimer.duration() * 1000;
	double percent = cur->groupTimer.duration() / root->groupTimer.duration() * 100;

	std::string timestr = std::to_string(ms) + "ms";
	std::string percentstr = std::to_string(percent) + "%%";

	std::string txt = name + " : " + timestr + " (" + percentstr + ")";

	if (ImGui::TreeNodeEx(txt.c_str(), flags)) {
		for (auto& [subname, subgroup] : cur->subgroups) {
			drawProfilerGroups(game, root, &subgroup, subname);
		}

		ImGui::TreePop();
	}
}

void gameEditor::profilerWindow(gameMain *game) {
	static std::shared_ptr<profile::group> profptr = nullptr;

	ImGui::Begin("Profiler", &showMetricsWindow);

	if (ImGui::Button("Capture")) {
		profptr = profile::getFrame();
	}

	if (profptr) {
		drawProfilerGroups(game, profptr.get());
	}

	ImGui::End();
}
