#include <grend/game_editor.hpp>

#define IMGUI_IMPL_OPENGL_LOADER_GLEW
#include <imgui/imgui.h>
#include <imgui/examples/imgui_impl_sdl.h>
#include <imgui/examples/imgui_impl_opengl3.h>

using namespace grendx;

static void draw_indicator(gameMain *rend, glm::mat4 trans,
                           glm::vec3 pos, glm::quat rot)
{
	// TODO: should load indicator models as part of the editor constructor
	static std::string ind[] = {
		"X-Axis-Pointer",
		"Y-Axis-Pointer",
		"Z-Axis-Pointer",
		"X-Axis-Rotation-Spinner",
		"Y-Axis-Rotation-Spinner",
		"Z-Axis-Rotation-Spinner",
	};

	glm::vec4 temp = trans * glm::vec4(1);
	glm::vec3 tpos = glm::vec3(temp) / temp.z;

	for (unsigned i = 0; i < 6; i++) {
		// TODO: and draw them here
		/*
		rend->draw_model((struct draw_attributes) {
			.name = ind[i],
			.transform =
				glm::translate(pos)
				* glm::mat4_cast(rot)
				* glm::scale(glm::vec3(0.5)),
			.dclass = {DRAWATTR_CLASS_UI, i},
		});
		*/
	}
}

static const ImGuiTreeNodeFlags base_flags
	= ImGuiTreeNodeFlags_OpenOnArrow
	| ImGuiTreeNodeFlags_OpenOnDoubleClick
	| ImGuiTreeNodeFlags_SpanAvailWidth;

void game_editor::addnodes_rec(const std::string& name,
                                          gameObject::ptr obj,
                                          std::set<gameObject::ptr>& selectedPath)
{
	if (obj) {
		ImGuiTreeNodeFlags flags
			= base_flags
			| ((selectedPath.count(obj))? ImGuiTreeNodeFlags_Selected : 0)
			| ((obj->nodes.size() == 0)?  ImGuiTreeNodeFlags_Leaf : 0);


		std::string fullname = name + " : " + obj->typeString();
		if (ImGui::TreeNodeEx(fullname.c_str(), flags)) {
			// avoid setting a new selected node if it's already a parent
			// in the selected path, which avoids overwriting the selected
			// node when trying to traverse to some clicked mesh
			if (ImGui::IsItemClicked() && !selectedPath.count(obj)) {
				selectedNode = obj;
			}

			for (auto& [subname, ptr] : obj->nodes) {
				addnodes_rec(subname, ptr, selectedPath);
			}

			ImGui::TreePop();
		}
	}
}

void game_editor::addnodes(std::string name, gameObject::ptr obj) {
	std::set<gameObject::ptr> path;

	for (gameObject::ptr p = selectedNode; p; p = p->parent) {
		path.insert(p);
	}

	addnodes_rec(name, obj, path);
}

void game_editor::map_window(gameMain *game) {
	ImGui::Begin("Objects", &show_map_window);
	//ImGui::Columns(2);
	
	addnodes("Objects", game->state->rootnode);

	/*
	ImGui::Text("Object properties");
	ImGui::NextColumn();

	for (auto& s : {"X", "Y", "Z"}) {
		ImGui::Text(s);
		ImGui::SameLine();
	}
	ImGui::NextColumn();
	*/

	// TODO: going to be replaced with node tree
#if 0
	for (int i = 0; i < (int)dynamic_models.size(); i++) {
		auto& ent = dynamic_models[i];

		ImGui::Separator();
		std::string name = "Editor object " + std::to_string(i);
		if (ImGui::Selectable(name.c_str(), selected_object == i)) {
			selected_object = i;
		}

		ImGui::NextColumn();
		ImGui::Text(ent.name.c_str());

		if (selected_object == i) {
			char tempname[256];
			strncpy(tempname, ent.classname.c_str(), 256);

			ImGui::InputText("class name", tempname, 256);
			ImGui::InputFloat3("position", glm::value_ptr(ent.position));
			ImGui::InputFloat3("scale", glm::value_ptr(ent.scale));
			ImGui::InputFloat4("rotation (quat)", glm::value_ptr(ent.rotation));

			ent.classname = std::string(tempname);
			rend->draw_model_lines({
				.name = ent.name,
				.transform = glm::translate(ent.position)
					* ent.transform
					* glm::scale(glm::vec3(1.05)),
			});

			draw_indicator(rend, ent.transform, ent.position, ent.rotation);
		}
		ImGui::NextColumn();
	}

	for (auto& [id, obj] : phys->objects) {
		// XXX: use negative numbers to differentiate physics from editor objects
		int transid = -id - 0x8000;
		std::string name = "Physics object " + std::to_string(id);

		ImGui::Separator();
		if (ImGui::Selectable(name.c_str(), selected_object == transid)) {
			selected_object = transid;
		}

		ImGui::NextColumn();
		ImGui::Text(obj.model_name.c_str());

		if (selected_object == transid) {
			rend->draw_model_lines({
				.name = obj.model_name,
				.transform = glm::translate(obj.position)
					* glm::scale(glm::vec3(1.05)),
			});

			ImGui::InputFloat3("position", glm::value_ptr(obj.position));
			ImGui::InputFloat3("velocity", glm::value_ptr(obj.velocity));
			//ImGui::InputFloat3("acceleration", glm::value_ptr(obj.acceleration));
			//ImGui::InputFloat3("scale", glm::value_ptr(obj.scale));
			ImGui::InputFloat4("rotation (quat)", glm::value_ptr(obj.rotation));

			draw_indicator(rend, glm::mat4(1), obj.position, obj.rotation);
		}
		ImGui::NextColumn();
	}

	ImGui::Columns(1);
	ImGui::Separator();

	if (ImGui::Button("Add Object")) {
		set_mode(mode::AddObject);
	}

	ImGui::SameLine();
	if (ImGui::Button("Delete Object")) {
		if (selected_object > 0 && selected_object < (int)dynamic_models.size()) {
			dynamic_models.erase(dynamic_models.begin() + selected_object);
		}
	}
#endif

	ImGui::End();
}
