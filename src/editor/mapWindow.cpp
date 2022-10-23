#include <grend/gameEditor.hpp>
#include <grend/loadScene.hpp>
#include <grend/utility.hpp>

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_sdl.h>
#include <imgui/backends/imgui_impl_opengl3.h>

using namespace grendx;

static const ImGuiTreeNodeFlags base_flags
	= ImGuiTreeNodeFlags_OpenOnArrow
	| ImGuiTreeNodeFlags_OpenOnDoubleClick
	| ImGuiTreeNodeFlags_SpanAvailWidth;

void gameEditor::addnodesRec(const std::string& name,
                               sceneNode::ptr obj,
                               std::set<sceneNode::ptr>& selectedPath)
{
	if (obj) {
		ImGuiTreeNodeFlags flags
			= base_flags
			| ((selectedPath.count(obj))? ImGuiTreeNodeFlags_Selected : 0)
			| ((obj->nodes.size() == 0)?  ImGuiTreeNodeFlags_Leaf : 0);

		std::string fullname = name + " : " + obj->typeString();
		auto contextMenu = [&] () {
			if (ImGui::BeginPopupContextItem()) {
				static char namebuf[64];

				ImGui::Text("Testing this: ");
				ImGui::InputText("##edit", namebuf, sizeof(namebuf));

				if (ImGui::Button("OK")) {
					ImGui::CloseCurrentPopup();

					if (auto p = obj->parent.lock()) {
						unlink(obj);
						setNode(namebuf, p, obj);
					}
				}

				ImGui::SameLine();
				if (ImGui::Button("Cancel")) {
					ImGui::CloseCurrentPopup();
				}

				ImGui::EndPopup();
			}

			if (ImGui::BeginDragDropTarget()) {
				if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("DRAG_FILENAME")) {
					const char *fname = (const char*)payload->Data;

					if (auto res = loadSceneCompiled(fname)) {
						auto& newobj = *res;
						std::string basename = basenameStr(fname) + ":";

						for (unsigned count = obj->nodes.size();; count++) {
							auto temp = basename + std::to_string(count);

							if (!obj->hasNode(temp)) {
								basename = temp;
								break;
							}
						}

						setNode(basename, obj, newobj);
					}
				}
			}
		};

		if (ImGui::TreeNodeEx(fullname.c_str(), flags)) {
			contextMenu();

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

		} else {
			// XXX: context menu needs to be directly after TreeNodeEx
			contextMenu();
		}

	}
}

void gameEditor::addnodes(std::string name,
                          sceneNode::ptr obj,
                          std::set<sceneNode::ptr>& selectedPath)
{
	addnodesRec(name, obj, selectedPath);
}

// TODO: could be a useful utility function
static sceneNode::ptr findRoot(sceneNode::ptr p) {
	while (p) {
		auto next = p->parent.lock();

		if (!next) {
			return p;

		} else {
			p = next;
		}

	}

	return nullptr;
}

void gameEditor::mapWindow(gameMain *game) {
	static sceneNode::ptr clipboard = nullptr;
	static std::string     clipboardName;
	sceneNode::ptr root = findRoot(selectedNode);

	std::set<sceneNode::ptr> selectedPath;
	for (sceneNode::ptr p = selectedNode; p; p = p->parent.lock()) {
		selectedPath.insert(p);
	}

	ImGui::Begin("Objects", &showMapWindow);

	if (ImGui::Button("Add Node") && selectedNode) {
		setNode("New node", selectedNode, std::make_shared<sceneNode>());
	}

	ImGui::SameLine();
	if (ImGui::Button("Cut Node") && selectedNode) {
		clipboard     = selectedNode;
		clipboardName = getNodeName(selectedNode);
		unlink(selectedNode);
	}

	ImGui::SameLine();
	if (ImGui::Button("Copy Node") && selectedNode) {
		clipboard     = selectedNode;
		clipboardName = getNodeName(selectedNode);
	}

	ImGui::SameLine();
	if (ImGui::Button("Paste Node") && selectedNode) {
		if (clipboardName.empty()) {
			clipboardName = "New node";
		}

		// avoid creating recursive trees
		// TODO: some notification if this check fails
		if (!selectedPath.count(clipboard)) {
			setNode(clipboardName, selectedNode, clipboard);
		}
	}

	ImGui::SameLine();
	ImGui::Text(" | ");

	ImGui::SameLine();
	if (ImGui::Button("Delete Node") && selectedNode) {
		unlink(selectedNode);
		selectedNode = nullptr;
	}

	addnodes("Objects", root, selectedPath);
	addnodes("Clipboard", clipboard, selectedPath);

	ImGui::End();
}
