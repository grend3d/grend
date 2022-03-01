#pragma once
#include <grend/gameObject.hpp>
#include <grend/gameMain.hpp>

#include <string>
#include <thread>
#include <optional>

namespace grendx {

std::pair<gameObject::ptr, modelMap> loadModel(std::string path);
std::pair<gameImport::ptr, modelMap> loadSceneData(std::string path);

/// @file
/**
 * Syncronously load and compile a scene from the specified path.
 *
 * @param path Path to the scene.
 *
 * @return gameImport::ptr representing the scene.
 */
gameImport::ptr loadSceneCompiled(std::string path);

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
			 std::string name="save.map");

std::pair<gameObject::ptr, modelMap> loadMapData(gameMain *game,
                                                 std::string name="save.map");

gameObject::ptr loadMapCompiled(gameMain *game, std::string name="save.map");
gameObject::ptr loadMapAsyncCompiled(gameMain *game, std::string name="save.map");

// namespace grendx
};
