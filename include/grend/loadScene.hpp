#pragma once
#include <grend/sceneNode.hpp>
#include <grend/sceneModel.hpp>

#include <string>
#include <thread>
#include <optional>
#include <future>

namespace grendx {

enum errorFlag {
	resultError,
};

template <typename T>
class result : public std::optional<T> {
	public:
		result(T&& value)
			: std::optional<T>(value) {};

		result(T& value)
			: std::optional<T>(value) {};

		result(const std::pair<enum errorFlag, const std::string&>& msg)
			: std::optional<T>(),
			error(msg.second) {};

		result(enum errorFlag, const std::string& msg)
			: std::optional<T>(),
			error(msg) {};

		const std::string& what() {
			return error;
		}

	private:
		std::string error;
};

static inline
std::pair<enum errorFlag, const std::string&>
invalidResult(const std::string& msg) noexcept {
	return {resultError, msg};
}

template <typename T>
void printError(result<T>& res) {
	if (!res) {
		// TODO: proper logger
		SDL_Log("ERROR: %s\n", res.what().c_str());
	}
}

using objectPair = std::pair<sceneNode::ptr, modelMap>;

result<objectPair> loadModel(std::string path) noexcept;
result<objectPair> loadSceneData(std::string path) noexcept;

/// @file
/**
 * Syncronously load and compile a scene from the specified path.
 *
 * @param path Path to the scene.
 *
 * @return sceneNode::ptr representing the scene.
 */
result<sceneNode::ptr> loadSceneCompiled(std::string path) noexcept;

/**
 * Load a scene asyncronously.
 *
 * @param game Game context pointer.
 * @param path The file path of the scene to be loaded.
 *
 * @return A pair with a node that will contain the loaded scene, and a future to
 *         wait on, if needed.
 *         If the scene couldn't be loaded, the result will be a sceneNode::ptr
 *         with no subnodes. (Should probably return an error of some sort...)
 */
std::pair<sceneNode::ptr, std::future<bool>>
loadSceneAsyncCompiled(std::string path);

void saveMap(sceneNode::ptr root,
			 std::string name="save.map") noexcept;

result<objectPair>
loadMapData(std::string name="save.map") noexcept;

result<sceneNode::ptr>
loadMapCompiled(std::string name="save.map") noexcept;

result<sceneNode::ptr>
loadMapAsyncCompiled(std::string name="save.map") noexcept;

// namespace grendx
};
