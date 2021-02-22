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

struct tabEntry {
	std::shared_ptr<profile::group> group;
	unsigned id;
	bool opened;
};

void gameEditor::profilerWindow(gameMain *game) {
	static std::vector<tabEntry> entries;
	static unsigned counter = 0;

	// if a tab was closed (in the last frame), erase it here
	for (auto it = entries.begin(); it < entries.end();) {
		it = (it->opened)? std::next(it) : entries.erase(it);
	}

	ImGui::Begin("Profiler", &showProfilerWindow);

	if (ImGui::Button("New capture")) {
		auto ptr = profile::getFrame();

		entries.push_back({
			.group  = ptr,
			.id     = counter++,
			.opened = true,
		});
	}

	ImGui::SameLine();
	static ImGuiTabBarFlags flags = ImGuiTabBarFlags_AutoSelectNewTabs;
	if (ImGui::BeginTabBar("Captures"), flags) {
		for (unsigned i = 0; i < entries.size(); i++) {
			auto& ent = entries[i];
			std::string name = "Capture " + std::to_string(ent.id);

			if (ent.opened && ImGui::BeginTabItem(name.c_str(), &ent.opened)) {
				ImGui::Separator();
				drawProfilerGroups(game, ent.group.get());
				ImGui::EndTabItem();
			}
		}
		ImGui::EndTabBar();
	}

	ImGui::End();
}
