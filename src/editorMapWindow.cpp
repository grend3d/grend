#include <grend/gameEditor.hpp>

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

void gameEditor::addnodesRec(const std::string& name,
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
				addnodesRec(subname, ptr, selectedPath);
			}

			ImGui::TreePop();
		}
	}
}

void gameEditor::addnodes(std::string name, gameObject::ptr obj) {
	std::set<gameObject::ptr> path;

	for (gameObject::ptr p = selectedNode; p; p = p->parent) {
		path.insert(p);
	}

	addnodesRec(name, obj, path);
}

void gameEditor::mapWindow(gameMain *game) {
	ImGui::Begin("Objects", &showMapWindow);
	addnodes("Objects", game->state->rootnode);
	ImGui::End();
}
