#include <grend/filePane.hpp>
#include <grend/logger.hpp>

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_sdl.h>
#include <imgui/backends/imgui_impl_opengl3.h>

#include <filesystem>

using namespace grendx;
namespace img = ImGui;
namespace fs = std::filesystem;

static const ImGuiTreeNodeFlags base_flags
	= ImGuiTreeNodeFlags_OpenOnArrow
	| ImGuiTreeNodeFlags_OpenOnDoubleClick
	| ImGuiTreeNodeFlags_SpanAvailWidth;

void paneNode::expand(void) {
	if (type != Directory) {
		return;
	}

	auto lastwrite = fs::last_write_time(fullpath);
	if (!expanded || lastwrite != last_modified) {
		subnodes.clear();
		last_modified = lastwrite;
		expanded = true;

		for (auto& ent : fs::directory_iterator(fullpath)) {
			enum paneTypes t;

			     if (ent.is_directory())    t = paneTypes::Directory;
			else if (ent.is_regular_file()) t = paneTypes::File;
			else                            t = paneTypes::Other;

			auto p = paneNode(fullpath / ent.path().filename(), t);
			subnodes.push_back(p);
		}
	}
}

void filePane::renderNodes(paneNode& node) {
	ImGuiTreeNodeFlags flags
		= base_flags
		| ((&node == selected)?                  ImGuiTreeNodeFlags_Selected : 0)
		| (((node.type != paneTypes::Directory)? ImGuiTreeNodeFlags_Leaf     : 0));

	if (img::TreeNodeEx(node.name.c_str(), flags)) {
		node.expand();

		if (img::IsItemClicked()) {
			selected = &node;
		}

		if (node.type == paneTypes::File) {
			if (img::BeginDragDropSource(ImGuiDragDropFlags_None)) {
				std::string temp = node.fullpath;
				// TODO: might be a good idea to encode extensions in
				//       the payload type, so extensions can only be dropped
				//       in places expecting them
				img::SetDragDropPayload("DRAG_FILENAME", temp.c_str(), temp.size() + 1);
				img::Text("File: %s", temp.c_str());
				img::EndDragDropSource();
			}
		}

		for (auto& subnode : node.subnodes) {
			renderNodes(subnode);
		}

		img::TreePop();
	}
}

void filePane::render() {
	img::Begin("Files");
	renderNodes(root);
	img::End();
}
