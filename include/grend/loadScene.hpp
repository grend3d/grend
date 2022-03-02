#pragma once
#include <grend/gameObject.hpp>
#include <grend/gameMain.hpp>

#include <string>
#include <thread>
#include <optional>

namespace grendx {

enum errorFlag {
	resultError,
};

template <typename T>
class result : public std::optional<T> {
	public:
		result(T&& value)
			: std::optional<T>(value) {};

		result(enum errorFlag, const std::string& msg)
			: std::optional<T>(),
			error(msg) {};

		const std::string& what() {
			return error;
		}

	private:
		std::string error;
};


template <typename T>
void printError(result<T>& res) {
	if (!res) {
		// TODO: proper logger
		SDL_Log("ERROR: %s\n", res.what().c_str());
	}
}

using importPair = std::pair<gameImport::ptr, modelMap>;
using objectPair = std::pair<gameObject::ptr, modelMap>;

result<objectPair> loadModel(std::string path) noexcept;
result<importPair> loadSceneData(std::string path) noexcept;

/// @file
/**
 * Syncronously load and compile a scene from the specified path.
 *
 * @param path Path to the scene.
 *
 * @return gameImport::ptr representing the scene.
 */
result<gameImport::ptr> loadSceneCompiled(std::string path) noexcept;

/**
 * Load a scene asyncronously.
 *
 * @param game Game context pointer.
 * @param path The file path of the scene to be loaded.
 *
 * @return A pair with a node that will contain the loaded scene, and a future to
 *         wait on, if needed.
 *         If the scene couldn't be loaded, the result will be a gameImport::ptr
 *         with no subnodes. (Should probably return an error of some sort...)
 */
std::pair<gameImport::ptr, std::future<bool>>
loadSceneAsyncCompiled(gameMain *game, std::string path);

void saveMap(gameMain *game,
			 gameObject::ptr root,
			 std::string name="save.map") noexcept;

result<objectPair>
loadMapData(gameMain *game, std::string name="save.map") noexcept;

result<gameObject::ptr>
loadMapCompiled(gameMain *game, std::string name="save.map") noexcept;

result<gameObject::ptr>
loadMapAsyncCompiled(gameMain *game, std::string name="save.map") noexcept;

// namespace grendx
};
