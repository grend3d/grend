#include <nlohmann/json.hpp>

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_sdl.h>
#include <imgui/backends/imgui_impl_opengl3.h>

#include <grend/fileBookmarks.hpp>
#include <grend/gameMain.hpp>
#include <grend/logger.hpp>

#include <iostream>

using namespace grendx;
using namespace grendx::engine;
using namespace nlohmann;

// XXX: TODO: move
template <typename T>
T* ResolveOrBind(void) {
	using namespace engine;

	if (T* lookup = Services().tryResolve<T>()) {
		return lookup;

	} else {
		Services().bind<T, T>();
		return Resolve<T>();
	}
}

fileBookmarksState::fileBookmarksState() : IoC::Service() {
	load();
}

const std::list<std::string>& fileBookmarksState::getBookmarks() {
	return this->bookmarks;
}

void fileBookmarksState::removeBookmark(size_t index) {

}

void fileBookmarksState::addBookmark(std::string_view path) {
	bookmarks.push_back(std::string(path));
	save();
	LogFmt("Adding bookmark path {}", path);
}

void fileBookmarksState::save(void) {
	std::ofstream saveFile("./bookmarks.json");

	json data = {};
	for (auto& place : bookmarks) {
		data.push_back(place);
	}

	saveFile << data.dump(4) << std::endl;
}

void fileBookmarksState::load(void) {
	std::ifstream saveFile("./bookmarks.json");

	if (!saveFile.good())
		return;

	json data;
	saveFile >> data;

	try {
		for (const std::string place : data) {
			bookmarks.push_back(place);
		}

	} catch (std::exception& e) {
		LogErrorFmt("Exception while loading: {}", e.what());
	}
}

std::optional<std::string> grendx::fileBookmarks() {
	auto state = ResolveOrBind<fileBookmarksState>();
	std::string ret;

	for (const auto& path : state->getBookmarks()) {
		if (ImGui::Selectable(path.c_str())) {
			ret = path;
		}
	}

	if (ret.empty()) {
		return {};
	} else {
		return ret;
	}
}
