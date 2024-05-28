#include <grend/filePane.hpp>
#include <grend/fileBookmarks.hpp>
#include <grend/logger.hpp>

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_sdl.h>
#include <imgui/backends/imgui_impl_opengl3.h>

#include <filesystem>

using namespace grendx;
namespace img = ImGui;
namespace fs = std::filesystem;

// XXX: TODO: move
#include <grend/gameMain.hpp>
using namespace grendx::engine;
template <typename T>
T* ResolveOrBind(void) {
	if (T* lookup = Services().tryResolve<T>()) {
		return lookup;

	} else {
		Services().bind<T, T>();
		return Resolve<T>();
	}
}

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

			fs::path entPath = fullpath / ent.path().filename();
			auto p = paneNode(entPath, t);
			subnodes.push_back(p);
		}
	}
}

void filePane::renderNodesRec(paneNode& node) {
	ImGuiTreeNodeFlags flags
		= base_flags
		| ((&node == selected)?                  ImGuiTreeNodeFlags_Selected    : 0)
		| ((&node == &root)?                     ImGuiTreeNodeFlags_DefaultOpen : 0)
		| (((node.type != paneTypes::Directory)? ImGuiTreeNodeFlags_Leaf        : 0))
		;

	// XXX: need to pass a non-empty string to TreeNodeEx
	std::string displayName = " " + node.name;
	bool opened = img::TreeNodeEx(displayName.c_str(), flags);

	std::string popupContext = node.name + ":context";
	if (ImGui::BeginPopupContextItem(popupContext.c_str())) {
		ImGui::TextColored(ImVec4(0.4f, 0.4f, 0.4f, 1.f), "Action");
		ImGui::Separator();

		if (node.type == paneTypes::Directory) {
			if (ImGui::Selectable("Add bookmark")) {
				auto bookmarks = ResolveOrBind<fileBookmarksState>();
				bookmarks->addBookmark(std::string(node.fullpath));
			}

			if (ImGui::Selectable("Change Directory")) {
				newPathBuf = node.fullpath;
				rootUpdated = true;
			}
		}

		else if (node.type == paneTypes::File) {
			if (ImGui::Selectable("Import")) {
				// TODO:
			}
		}

		if (ImGui::Selectable("Other stuff maybe")) { /* TODO */ }
		ImGui::EndPopup();
	}

	if (opened) {
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
			renderNodesRec(subnode);
		}

		img::TreePop();
	}
}

void filePane::renderNodes(void) {
	renderNodesRec(root);

	if (rootUpdated) {
		chdir(newPathBuf);
		rootUpdated = false;
	}
}

void filePane::updatePathBuf(void) {
	std::string pathStr = root.fullpath;
	strncpy(pathBuf, pathStr.c_str(), sizeof(pathBuf));
	pathBuf[sizeof(pathBuf) - 1] = 0;
}

void filePane::chdir(std::string_view newroot) {
	LogFmt("Changing directory to {}", newroot);
	if (newroot == "..") {
		if (root.fullpath.has_parent_path()) {
			std::string pathStr = root.fullpath.parent_path();
			root = paneNode(pathStr, paneTypes::Directory);
		}

	} else {
		root = paneNode(newroot, paneTypes::Directory);
	}

	updatePathBuf();
}

void filePane::render() {
	img::Begin("Files");

	if (editPath == false) {
		ImGui::Text("%s", pathBuf);

		ImGui::SameLine();
		if (ImGui::Button("Up")) {
			chdir("..");
		}

		ImGui::SameLine();
		if (ImGui::Button(">")) {
			editPath = true;
		}

	} else {
		ImGui::InputText("Path", pathBuf, sizeof(pathBuf));
		ImGui::SameLine();

		if (ImGui::Button("Ok")) {
			chdir(pathBuf);
			editPath = false;
		}
	}

	ImGui::BeginChild("Main View");
	ImGui::BeginChild("File list", ImVec2(0, -300), false, 0);
	renderNodes();
	ImGui::EndChild();

	ImGui::Separator();
	ImGui::Text("Bookmarks");

	ImGui::BeginChild("BookmarksList", ImVec2(0, 0), false, 0);
	if (auto path = fileBookmarks()) {
		LogFmt("Setting new path {}", *path);
		chdir(*path);
	}
	ImGui::EndChild();

	ImGui::EndChild();
	img::End();
}
