#include <grend/gameEditor.hpp>
#include <grend/utility.hpp>

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_sdl.h>
#include <imgui/backends/imgui_impl_opengl3.h>

#include <string.h>

using namespace grendx;

static char searchBuffer[0x1000] = "";

static void drawSelectableLabel(const char *txt) {
	if (ImGui::Selectable(txt)) {
		if (*searchBuffer) {
			// add a space if the search buffer already has contents
			strncat(searchBuffer, " ", sizeof(searchBuffer) - 1);
		}

		strncat(searchBuffer, txt, sizeof(searchBuffer) - 1);
	}
}

static void drawJson(nlohmann::json& value) {
	static ImGuiTreeNodeFlags flags
		= ImGuiTreeNodeFlags_Bullet
		| ImGuiTreeNodeFlags_DefaultOpen
		/*
		| ImGuiTreeNodeFlags_OpenOnArrow
		| ImGuiTreeNodeFlags_OpenOnDoubleClick
		| ImGuiTreeNodeFlags_SpanAvailWidth
		*/
		;

	if (value.is_array()) {
		for (unsigned i = 0; i < value.size(); i++) {
			std::string name = "[" + std::to_string(i) + "]";

			if (ImGui::TreeNodeEx(name.c_str(), flags)) {
				drawJson(value[i]);
				ImGui::TreePop();
			}
		}
	}

	else if (value.is_object()) {
		for (auto& [name, em] : value.items()) {
			if (ImGui::TreeNodeEx(name.c_str(), flags)) {
				drawJson(em);
				ImGui::TreePop();
			}
		}
	}

	else if (value.is_number_float()) {
		auto ptr = value.get_ptr<nlohmann::json::number_float_t*>();
		float p = *ptr;

		ImGui::SameLine();
		ImGui::SliderFloat("float", &p, 0.f, 10.f);

		*ptr = p;
	}

	else if (value.is_string()) {
		ImGui::SameLine();
		ImGui::Text("%s", value.get_ptr<std::string*>()->c_str());
	}

	else if (value.is_null()) {
		ImGui::SameLine();
		ImGui::TextColored(ImVec4(0.4f, 0.4f, 0.4f, 1.f), "<null>");
	}
}

void gameEditor::addEntityWindow(gameMain *game) {
	static char comboBuf[0x1000];
	static const size_t bufsize = sizeof(comboBuf) - 1;

	*comboBuf = '\0'; // reset every frame

	ImGui::Begin("Add entity", &showAddEntityWindow, ImGuiWindowFlags_MenuBar);
	std::vector<std::string> names;

	size_t n = 0;
	for (auto& [name, _] : game->factories->factories) {
		names.push_back(name);

		if (n + name.length() + 2 < bufsize) {
			strncpy(comboBuf + n, name.c_str(), bufsize - n);
			n += name.length() + 1;
			// add second null terminator in case this is the last entry
			comboBuf[n + 1] = '\0';
		}
	}

	// make sure the combo string is null-terminated (man I don't miss C strings)
	// this is one big thing I'm not a fan of in imgui -
	// std::initializer_list would still be pretty cheap for the cases where you
	// want to specify a list of constants in code, std::vector& would be _much_
	// cheaper for cases like this where you're dynamically generating some list
	// of options...
	// why not use a more c++-ish style, it's basically a c library written in c++
	comboBuf[bufsize] = '\0';
	comboBuf[bufsize - 1] = '\0';

	static int baseIndex = -1;
	static std::vector<int> componentIndexes;
	static nlohmann::json curjson;
	static bool syncToCursor = true;

	// TODO
	if (ImGui::BeginMenuBar()) {
		if (ImGui::BeginMenu("Templates")) {
			if (ImGui::MenuItem("Save as new template")) {

				static char templateSave[256] = "Template";
				ImGui::InputText("## template save", templateSave, sizeof(templateSave));
				ImGui::SameLine();
				if (ImGui::Button("Save as template")) {
					// TODO: Save as template implementation
				}
			}

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Entity properties")) {
			ImGui::Checkbox("Sync node properties to cursor", &syncToCursor);
			ImGui::EndMenu();
		}

		ImGui::EndMenuBar();
	}

	static int curidx = 0;
	ImGui::Combo("Base entity", &curidx, comboBuf);
	ImGui::Separator();

	if (curidx != baseIndex) {
		curjson = game->factories->properties(names[curidx]);
		baseIndex = curidx;
		// XXX
		curjson["entity-type"] = names[curidx];
	}

	float footer =
		2.f * (ImGui::GetStyle().ItemSpacing.y
		       + ImGui::GetFrameHeightWithSpacing());
	float sidebuttons = ImGui::GetStyle().ItemSpacing.x
		+ ImGui::GetFrameHeightWithSpacing();

	ImGui::BeginChild("components", ImVec2(0, -footer));
	ImGui::TextColored(ImVec4(0.4f, 0.4f, 0.4f, 1.f), "Properties");
	ImGui::Separator();
	drawJson(curjson);
	ImGui::EndChild();

	ImGui::Separator();
	static int addidx = 0;
	ImGui::Combo("##Component Type", &addidx, comboBuf);
	ImGui::SameLine();
	if (ImGui::Button("Add component")) {
		auto props = game->factories->properties(names[addidx]);

		curjson["components"].push_back({
			names[addidx],
			props
		});
	}

	if (syncToCursor) {
		// TODO: could have function to serialize any TRS
		curjson["node"] = {
			{"position",
				{
					cursorBuf.position.x,
					cursorBuf.position.y,
					cursorBuf.position.z,
				}},
			{"rotation",
				{
					cursorBuf.rotation.w,
					cursorBuf.rotation.x,
					cursorBuf.rotation.y,
					cursorBuf.rotation.z,
				}},
			{"scale",
				{
					cursorBuf.scale.x,
					cursorBuf.scale.y,
					cursorBuf.scale.z,
				}},
		};
	}

	ImGui::Separator();

	static bool attach = false;
	ImGui::Checkbox("Attach to selected node", &attach);

	ImGui::SameLine();
	if (ImGui::Button("Create")) {
		ecs::entity *ent = game->factories->build(game->entities.get(), curjson);

		if (ent) {
			if (attach && selectedNode) {
				setNode("entity-original-node", selectedNode, ent->node);
				ent->node = selectedNode;
			}

			game->entities->add(ent);
			selectedNode = ent->node;
		}
	}

	ImGui::SameLine();
	if (ImGui::Button("Cancel")) {
		componentIndexes.clear();
		baseIndex = 0;
		showAddEntityWindow = false;
	}

	ImGui::End();
}

void gameEditor::entitySelectWindow(gameMain *game) {
	ImGui::Begin("Entities", &showEntitySelectWindow);
	if (ImGui::Button("Clear")) {
		*searchBuffer = '\0';
	}
	ImGui::SameLine();
	ImGui::InputText("Search", searchBuffer, sizeof(searchBuffer));
	auto tags = split_string(searchBuffer);
	std::vector<const char *> tagchars;

	for (const auto& v : tags) {
		//tagchars.push_back(v.c_str());
		if (const char *re = remangle(v)) {
			tagchars.push_back(re);
		}
	}

	ImGui::SameLine();
	if (ImGui::Button("New entity")) {
		showAddEntityWindow = true;
	}
	ImGui::Separator();

	ImGui::Columns(2);
	ImGui::BeginChild("components");
	ImGui::TextColored(ImVec4(0.4f, 0.4f, 0.4f, 1.f), "Active components");
	ImGui::Separator();

	ImGui::BeginChild("componentList");
	for (const auto& [name, _] : game->entities->components) {
		drawSelectableLabel(demangle(name).c_str());
	}

	ImGui::EndChild();
	ImGui::EndChild();
	ImGui::NextColumn();

	ImGui::BeginChild("matchingEnts", ImVec2(0, 0), false, 0);
	ImGui::TextColored(ImVec4(0.4f, 0.4f, 0.4f, 1.f), "Matching entities");
	ImGui::Separator();

	ImGui::BeginChild("entityList", ImVec2(0, 0), false, 0);
	for (auto& ent : game->entities->entities) {
		if (*searchBuffer && !game->entities->hasComponents(ent, tagchars)) {
			// entity doesn't have the searched tags, filtered out
			// TODO: wait, why am I not using the search interface here?
			continue;
		}

		std::string entstr = "entity #" + std::to_string((uintptr_t)ent);
		std::string contextstr = entstr + ":context";

		if (game->entities->condemned.count(ent)) {
			entstr = "[deleted] " + entstr;
		}

		if (ImGui::Selectable(entstr.c_str(), ent == selectedEntity)) {
			selectedEntity = ent;
			selectedNode = ent->node;
		}

		if (ImGui::BeginPopupContextItem(contextstr.c_str())) {
			ImGui::TextColored(ImVec4(0.4f, 0.4f, 0.4f, 1.f), "Action");
			ImGui::Separator();

			if (ImGui::Selectable("Delete")) {
				game->entities->remove(selectedEntity);
				selectedEntity = nullptr;
			}

			if (ImGui::Selectable("Duplicate")) { /* TODO */ }
			ImGui::EndPopup();
		}

		auto& components = game->entities->getEntityComponents(ent);
		std::set<std::string> seen;
		ImGui::Separator();
		ImGui::Indent(16.f);
		ImGui::TextColored(ImVec4(0.4f, 0.4f, 0.4f, 1.f), "Attached components:");
		ImGui::Columns(2);

		std::string sectionName = entstr + ":components";
		ImGui::TreePush(sectionName.c_str());

		for (auto& [name, comp] : components) {
			if (!seen.count(name)) {
				drawSelectableLabel(demangle(name).c_str());
				ImGui::NextColumn();
				seen.insert(name);
			}
		}

		ImGui::TreePop();
		ImGui::Columns(1);
		ImGui::Unindent(16.f);
		ImGui::Separator();
	}

	ImGui::EndChild();
	ImGui::EndChild();
	ImGui::Columns(1);
	ImGui::End();
}
