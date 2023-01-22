#include <grend/gameEditor.hpp>
#include <grend/utility.hpp>
#include <grend/fileDialog.hpp>

#include <grend/ecs/ecs.hpp>
#include <grend/ecs/serializer.hpp>

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_sdl.h>
#include <imgui/backends/imgui_impl_opengl3.h>

#include <fstream>
#include <string.h>

using namespace grendx;
using namespace grendx::engine;

static char searchBuffer[0x1000] = "";
static fileDialog export_entity_dialog("Export entity");

static bool is_name_pair(nlohmann::json& value) {
	return value.is_array()
		&& value.size() == 2
		&& value[0].is_string()
		&& value[1].is_object();
}

static bool is_vector(nlohmann::json& value) {
	if (!value.is_array())
		return false;

	if (value.empty())
		return false;

	if (value.size() > 4)
		return false;

	for (auto& v : value) {
		if (!v.is_number_float()) {
			return false;
		}
	}

	return true;
}

static void drawJson(nlohmann::json& value, const std::string& path = ".") {
	if (is_name_pair(value)) {
		std::string name    = value[0];
		nlohmann::json& obj = value[1];

		ImGui::PushID(1234);
		ImGui::Text("%s", name.c_str());
		ImGui::Indent();
		drawJson(obj, path + ":" + name);
		ImGui::Unindent();
		ImGui::PopID();
	}

	else if (is_vector(value)) {
		float ptrs[4];

		for (int i = 0; i < value.size(); i++) {
			ptrs[i] = value[i];
		}

		ImGui::SameLine();
		switch (value.size()) {
			case 1: ImGui::InputFloat ("", ptrs); break;
			case 2: ImGui::InputFloat2("", ptrs); break;
			case 3: ImGui::InputFloat3("", ptrs); break;
			case 4: ImGui::InputFloat4("", ptrs); break;
			default: break;
		}

		for (int i = 0; i < value.size(); i++) {
			value[i] = ptrs[i];
		}
	}

	else if (value.is_array()) {
		ImGui::Indent();

		for (unsigned i = 0; i < value.size(); i++) {
			ImGui::PushID(i);
			std::string name = "[" + std::to_string(i) + "]";

			// TODO: if entries are all numbers and has <=4 entries,
			//       draw all on the same line
			drawJson(value[i], path + name);

			ImGui::PopID();
		}

		ImGui::Unindent();
	}

	else if (value.is_object()) {
		ImGui::Indent();

		unsigned i = 0;
		for (auto& [name, em] : value.items()) {
			ImGui::PushID(i++);
			std::string temp = path + ":" + name;
			ImGui::Text("%s", name.c_str());
			drawJson(em, path + ":" + name);

			ImGui::PopID();
		}

		ImGui::Unindent();
	}

	else if (value.is_number_float()) {
		auto ptr = value.get_ptr<nlohmann::json::number_float_t*>();
		float p = *ptr;

		ImGui::SameLine();
		ImGui::SliderFloat("float", &p, 0.f, 10.f);

		*ptr = p;
	}

	else if (value.is_string()) {
		#define num 128

		static std::map<std::string, std::array<char, num>> foo;

		const char *asdf = value.get_ptr<std::string*>()->c_str();
		std::string localpath = path + ":" + asdf;

		if (!foo.contains(localpath)) {
			auto& arrbuf = foo[localpath];
			char *data = arrbuf.data();

			strncpy(data, asdf, num);
		}

		auto& arrbuf = foo[localpath];
		char *data = arrbuf.data();

		ImGui::SameLine();
		ImGui::InputText("##edit", data, num);

		if (ImGui::BeginDragDropTarget()) {
			if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("DRAG_FILENAME")) {
				const char *fname = (const char*)payload->Data;
				strncpy(data, fname, num - 1);
			}
		}

		value = data;
	}

	else if (value.is_null()) {
		ImGui::TextColored(ImVec4(0.4f, 0.4f, 0.4f, 1.f), "<null>");
	}
}

static void handle_prompts(gameEditorUI *wrapper) {
	gameEditor::ptr editor = wrapper->editor;

	if (export_entity_dialog.promptFilename()) {
		auto factories = Resolve<ecs::serializer>();
		auto entities  = Resolve<ecs::entityManager>();
		auto* selectedEntity = editor->getSelectedEntity().getPtr();

		const std::string& name = export_entity_dialog.selection;

		if (!selectedEntity) {
			LogErrorFmt("No entity selected, can't export to ", name);
			return;
		}

		LogFmt("Exporting entity to ", name);

		std::ofstream out(name);
		auto curjson = factories->serialize(entities, selectedEntity);

		out << curjson.dump();

		export_entity_dialog.clear();
	}
}

void gameEditorUI::entityEditorWindow() {
	auto entities  = Resolve<ecs::entityManager>();
	auto factories = Resolve<ecs::serializer>();

	ImGui::Begin("Entity Editor", &showEntityEditorWindow);

	ImGui::BeginChild("editComponent");
	ImGui::TextColored(ImVec4(0.4f, 0.4f, 0.4f, 1.f), "Active components");
	ImGui::Separator();

	ImGui::BeginChild("editView");
	static nlohmann::json curjson;
	static ecs::entity* curcomp = nullptr;
	auto* selectedEntity = editor->getSelectedEntity().getPtr();

	if (selectedEntity != curcomp && selectedEntity) {
		curjson = factories->serialize(entities, selectedEntity);
		curcomp = selectedEntity;
	}

	drawJson(curjson);

	if (ImGui::Button("Apply")) {
		ecs::entity *ent = factories->build(entities, curjson);

		entities->remove(selectedEntity);
		entities->clearFreedEntities();
		entities->add(ent);

		editor->setSelectedEntity(ent);
	}

	ImGui::EndChild();
	ImGui::EndChild();

	ImGui::End();
}

void gameEditorUI::entityListWindow() {
	auto entities  = Resolve<ecs::entityManager>();
	auto factories = Resolve<ecs::serializer>();

	ImGui::Begin("Entities", &showEntityListWindow);
	if (ImGui::Button("Clear")) {
		*searchBuffer = '\0';
	}
	ImGui::SameLine();
	ImGui::InputText("Search", searchBuffer, sizeof(searchBuffer));
	auto tags = split_string(searchBuffer);
	std::vector<const char *> tagchars;

	auto* selectedEntity = editor->getSelectedEntity().getPtr();

	for (const auto& v : tags) {
		//tagchars.push_back(v.c_str());
		if (const char *re = remangle(v)) {
			tagchars.push_back(re);
		}
	}

	ImGui::SameLine();
	if (ImGui::Button("New entity")) {
		//showAddEntityWindow = true;
		ImGui::OpenPopup("new_entity_popup");
	}

	if (ImGui::BeginPopup("new_entity_popup")) {
		for (const auto& [name, _] : entities->components) {
			if (ImGui::Selectable(demangle(name).c_str())) {
				nlohmann::json j = {
					{"entity-type", demangle(name)},
					{"entity-properties", {
						{"position", {0,0,0}},
						{"rotation", {1,0,0,0}},
						{"scale",    {1,1,1}}
					}},
					{"components", {}},
				};

				ecs::entity *ent = factories->build(entities, j);
				entities->add(ent);
			}
			//drawSelectableLabel(demangle(name).c_str());
		}
		ImGui::EndPopup();
	}

	ImGui::Separator();

	ImGui::BeginChild("matchingEnts", ImVec2(0, 0), false, 0);
	ImGui::TextColored(ImVec4(0.4f, 0.4f, 0.4f, 1.f), "Matching entities");
	ImGui::Separator();

	ImGui::Button("Testing");

	if (ImGui::BeginDragDropTarget()) {
		if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("DRAG_FILENAME")) {
			LogFmt("Accepting payload");
			try {
				const char *fname = (const char*)payload->Data;
				LogFmt("Have payload: {}", fname);
				std::ifstream in(fname);

				if (in.good()) {
					nlohmann::json j;
					in >> j;

					ecs::entity *ent = factories->build(entities, j);
					entities->add(ent);

					editor->setSelectedEntity(ent);

				} else {
					LogFmt("Invalid file name: {}", fname);
				}

			} catch (std::exception& e) {
				LogFmt("Exception while loading entity: {}", e.what());
			}
		}
		ImGui::EndDragDropTarget();
	}

	ImGui::BeginChild("entityList", ImVec2(0, 0), false, 0);
	for (auto& ent : entities->entities) {
		if (*searchBuffer && !entities->hasComponents(ent, tagchars)) {
			// entity doesn't have the searched tags, filtered out
			// TODO: wait, why am I not using the search interface here?
			continue;
		}

		//std::string entstr = "entity #" + std::to_string((uintptr_t)ent);
		std::string entstr = demangle(ent->mangledType) + " : " + ent->name;
		std::string contextstr = entstr + ":context";
		std::string popupstr = entstr + ":popup";

		if (entities->condemned.count(ent)) {
			entstr = "[deleted] " + entstr;
		}

		if (ImGui::Selectable(entstr.c_str(), ent == selectedEntity)) {
			editor->setSelectedEntity(ent);
		}

		if (ImGui::BeginPopupContextItem(contextstr.c_str())) {
			ImGui::TextColored(ImVec4(0.4f, 0.4f, 0.4f, 1.f), "Action");
			ImGui::Separator();

			if (ImGui::Selectable("Delete")) {
				entities->remove(selectedEntity);
				editor->setSelectedEntity(nullptr);
			}

			if (ImGui::Selectable("Duplicate")) { /* TODO */ }
			ImGui::EndPopup();
		}

		auto& components = entities->getEntityComponents(ent);
		std::set<std::string> seen;
		ImGui::Separator();
		ImGui::Indent(16.f);
		ImGui::TextColored(ImVec4(0.4f, 0.4f, 0.4f, 1.f), "Attached components:");
		//ImGui::Columns(2);

		std::string sectionName = entstr + ":components";
		ImGui::TreePush(sectionName.c_str());

		for (auto& [name, comp] : components) {
			if (!seen.count(name)) {
				if (ImGui::Selectable(demangle(name).c_str())) {
					// TODO: need a way to serialize a specific component,
					//       avoid recreating an entire entity
					editor->setSelectedEntity(ent);
				}

				seen.insert(name);
			}
		}

		ImGui::TreePop();

		std::string asdf = entstr + ":sec";
		ImGui::TreePush(asdf.c_str());
		if (ImGui::Button("Attach")) {
			ImGui::OpenPopup(popupstr.c_str());
		}

		ImGui::SameLine();
		if (ImGui::Button("Save")) {
			editor->setSelectedEntity(ent);
			export_entity_dialog.show();
		}

		if (ImGui::BeginPopup(popupstr.c_str())) {
			for (const auto& [name, _] : factories->factories) {
				if (ImGui::Selectable(name.c_str())) {
					nlohmann::json j = {name, {}};

					factories->build(entities, ent, j);
				}
			}

			ImGui::EndPopup();
		}
		ImGui::TreePop();

		ImGui::Unindent(16.f);
		ImGui::Separator();
	}

	ImGui::EndChild();
	ImGui::EndChild();
	ImGui::End();

	handle_prompts(this);
}

void gameEditorUI::addEntityWindow() {
	// TODO: remove
}
