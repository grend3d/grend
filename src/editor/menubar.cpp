#include <grend/gameEditor.hpp>
#include <grend/loadScene.hpp>
#include <grend/fileDialog.hpp>
#include <grend/ecs/serializer.hpp>

// TODO: debugging, remove
#include <iostream>
#include <fstream>

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_sdl.h>
#include <imgui/backends/imgui_impl_opengl3.h>

using namespace grendx;
using namespace grendx::engine;

static fileDialog open_dialog("Open File");
static fileDialog save_as_dialog("Save As...");

static fileDialog import_model_dialog("Import Model");
static fileDialog import_scene_dialog("Import Scene");
static fileDialog import_map_dialog("Import Map");

// TODO: move this to entity file
static fileDialog load_entity_dialog("Load Entity");

static void handle_prompts(gameEditor *editor) {
	auto state     = Resolve<gameState>();
	auto factories = Resolve<ecs::serializer>();
	auto entities  = Resolve<ecs::entityManager>();

	if (open_dialog.promptFilename()) {
		LogFmt("Opening a file here! at {}", open_dialog.selection);
		open_dialog.clear();

		if (auto node = loadMapCompiled(open_dialog.selection)) {
			editor->clear();
			editor->selectedNode = state->rootnode = *node;
		} else printError(node);
	}

	if (save_as_dialog.promptFilename()) {
		LogFmt("Saving as a file! at {}", save_as_dialog.selection);

		// TODO: some way to save a subnode as it's own map
		//saveMap(game, editor->selectedNode, save_as_dialog.selection);
		saveMap(state->rootnode, save_as_dialog.selection);
		save_as_dialog.clear();
	}

	if (import_model_dialog.promptFilename()) {
		LogFmt("Importing a thing! at {}", import_model_dialog.selection);
		import_model_dialog.clear();

		if (auto res = loadModel(import_model_dialog.selection)) {
			auto [obj, models] = *res;
			std::string name = "model["+std::to_string(obj->id)+"]";
			setNode(name, editor->selectedNode, obj);
			compileModels(models);
		} else printError(res);
	}

	if (import_scene_dialog.promptFilename()) {
		LogFmt("Importing a scene! at {}", import_scene_dialog.selection);
		import_scene_dialog.clear();

		if (auto res = loadSceneCompiled(import_scene_dialog.selection)) {
			auto obj = *res;
			std::string name = "import["+std::to_string(obj->id)+"]";
			setNode(name, editor->selectedNode, obj);
		} else printError(res);
	}

	if (import_map_dialog.promptFilename()) {
		LogFmt("Importing a map! at {}", import_map_dialog.selection);
		import_map_dialog.clear();

		if (auto res = loadMapCompiled(import_map_dialog.selection)) {
			auto obj = *res;
			std::string name = "map["+std::to_string(obj->id)+"]";
			setNode(name, editor->selectedNode, obj);
		} else printError(res);
	}

	if (load_entity_dialog.promptFilename()) {
		const auto& name = load_entity_dialog.selection;
		LogFmt("Importing an entity! at {}", name);

		std::ifstream in(name);

		if (in.good()) {
			nlohmann::json j;
			in >> j;

			ecs::entity *ent = factories->build(entities, j);
			entities->add(ent);

			load_entity_dialog.clear();

		} else {
			// TODO: errors need to be message boxes
			LogErrorFmt("Invalid filename. ({})", name);
		}
	}
}

void gameEditor::menubar() {
	static bool demo_window = false;

	auto state = Resolve<gameState>();
	auto rend  = Resolve<renderContext>();
	auto phys  = Resolve<physics>();

	if (ImGui::BeginMainMenuBar()) {
		if (ImGui::BeginMenu("File")) {
			if (ImGui::MenuItem("New", "CTRL+N")) {
				// TODO: "discard without saving" confirmation
				selectedNode = state->rootnode = std::make_shared<sceneNode>();
			}
			if (ImGui::MenuItem("Open", "CTRL+O")) { open_dialog.show(); }
			if (ImGui::MenuItem("Save", "CTRL+S")) {}
			if (ImGui::MenuItem("Save As", "Shift+CTRL+S")) { save_as_dialog.show(); }

			if (ImGui::MenuItem("Close", "CTRL+O")) {}

			ImGui::Separator();
			//if (ImGui::MenuItem("Import")) { import_dialog.show(); }
			if (ImGui::BeginMenu("Import")) {
				if (ImGui::MenuItem("Model(s)")) { import_model_dialog.show(); }
				if (ImGui::MenuItem("Scene")) { import_scene_dialog.show(); }
				if (ImGui::MenuItem("Map")) { import_map_dialog.show(); }
				ImGui::EndMenu();
			}

			ImGui::Separator();
			if (ImGui::MenuItem("Reload shaders", "CTRL+R")){ reloadShaders(); }

			ImGui::Separator();
			if (ImGui::MenuItem("Exit", "CTRL+Q")) {}

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Edit")) {
			if (ImGui::MenuItem("Undo", "CTRL+Z")) {}
			if (ImGui::MenuItem("Redo", "CTRL+Y")) {}

			ImGui::Separator();
			if (ImGui::MenuItem("Reset+Regenerate physics", "CTRL+P")) {}

			ImGui::Separator();
			if (ImGui::MenuItem("asdf", "CTRL+V")) {}

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("View")) {
			int proj = (int)cam->project();
			float scale = cam->scale();
			bool checkGL = GL_ERROR_CHECK_ENABLED();

			ImGui::Checkbox("Snap to grid", &snapEnabled);
			if (snapEnabled) {
				ImGui::InputFloat("Snap increment", &snapAmount,
								  0.1f, 10.f, "%.1f");
			}

			ImGui::SliderFloat("Movement speed", &movementSpeed,
			                  1.f, 100.f, "%.1f");
			ImGui::SliderFloat("Exposure (tonemapping)", &rend->exposure,
			                   0.1, 10.f);
			ImGui::SliderFloat("Light threshold", &rend->lightThreshold,
			                   0.001, 1.f);

			ImGui::Combo("Projection", &proj, "Perspective\0Orthographic\0");
			ImGui::SliderFloat("Projection scale", &scale, 0.001, 0.2f);
			ImGui::Checkbox("Check GL errors", &checkGL);

			ImGui::Checkbox("Show environment probes", &showProbes);
			ImGui::Checkbox("Show lights", &showLights);

			ImGui::Separator();
			if (ImGui::MenuItem("Render settings")) {
				showSettingsWindow = true;
			}

			if (ImGui::BeginMenu("Dockable windows")) {
				ImGui::MenuItem("File pane",         nullptr, &showFilePane);
				// TODO: rename map window -> scene editor
				ImGui::MenuItem("Scene editor",      nullptr, &showMapWindow);
				ImGui::MenuItem("Scene node editor", nullptr, &showMapWindow);

				ImGui::MenuItem("Entity list",       nullptr, &showEntityListWindow);
				ImGui::MenuItem("Entity editor",     nullptr, &showEntityEditorWindow);
				ImGui::MenuItem("Profiler",          nullptr, &showProfilerWindow);
				ImGui::MenuItem("Metrics",           nullptr, &showProfilerWindow);
				ImGui::MenuItem("Settings",          nullptr, &showSettingsWindow);
				ImGui::MenuItem("Console log",       nullptr, &showLogWindow);
				ImGui::EndMenu();
			}
			ImGui::EndMenu();

			cam->setProjection((enum camera::projection)proj);
			cam->setScale(scale);
			ENABLE_GL_ERROR_CHECK(checkGL);
		}

		if (ImGui::BeginMenu("Scene")) {
			if (ImGui::MenuItem("Scene graph", "o"))
				showMapWindow = true;

			if (ImGui::MenuItem("Node object editor", "r"))
				showObjectEditorWindow = true;

			if (ImGui::MenuItem("Node object selection"))
				showObjectSelectWindow = true;

			if (ImGui::MenuItem("Material editor", "CTRL+M")) {}

			ImGui::Separator();
			if (ImGui::MenuItem("Add object", "lo")) {
				setMode(mode::AddObject);
			}

			if (ImGui::MenuItem("Add point light", "lp")) {
				setMode(mode::AddPointLight);
			}

			if (ImGui::MenuItem("Add spot light", "ls")) {
				setMode(mode::AddSpotLight);
			}

			if (ImGui::MenuItem("Add directional light", "ld")) {
				setMode(mode::AddDirectionalLight);
			}

			if (ImGui::MenuItem("Add reflection probe", "lr")) {
				setMode(mode::AddReflectionProbe);
			}

			if (ImGui::MenuItem("Add irradiance probe", "li")) {
				setMode(mode::AddIrradianceProbe);
			}

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Entities")) {
			if (ImGui::MenuItem("Entity editor"))
				showEntityEditorWindow = true;

			if (ImGui::MenuItem("Entity list"))
				showEntityListWindow = true;

			ImGui::Separator();

			if (ImGui::MenuItem("Load Entity")) {
				load_entity_dialog.show();
				//showAddEntityWindow = true;
			}

			if (ImGui::MenuItem("Edit Entity")) {
				// TODO:
			}

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Lighting")) {
			// TODO: actually have everything set up to implement all of this,
			//       how far we've come...
			if (ImGui::MenuItem("Bake lighting", "CTRL+B")) {}
			if (ImGui::MenuItem("Generate irradiance probes", "CTRL+L")) {}
			if (ImGui::MenuItem("Generate reflectance cubemaps", "Shift-CTRL+L")) {}

			if (ImGui::MenuItem("Re-render selected light maps", "CTRL+p")) {
				if (selectedNode) {
					invalidateLightMaps(selectedNode);
				}
			}

			if (ImGui::MenuItem("Re-render all light maps", "CTRL+P")) {
				if (state->rootnode) {
					invalidateLightMaps(state->rootnode);
				}
			}

			ImGui::Separator();

			static int lightMode = -1;
			int i = 0;
			for (auto& [key, _] : rend->lightingShaders) {
				ImGui::RadioButton(key.c_str(), &lightMode, i);
				if (lightMode == i) {
					rend->setDefaultLightModel(key);
				}

				i++;
			}

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Debug")) {
			if (ImGui::MenuItem("Open profiler")) {
				showProfilerWindow = true;
			}

			if (ImGui::BeginMenu("Physics debug options")) {
				static bool physopts[16];

				// XXX: direct copy of constants from btIDebugDraw.h
				ImGui::Checkbox("Wireframe",              physopts + 0);
				ImGui::Checkbox("Draw AABBs",             physopts + 1);
				ImGui::Checkbox("Features text",          physopts + 2);
				ImGui::Checkbox("Contact points",         physopts + 3);
				ImGui::Checkbox("No deactivation",        physopts + 4);
				ImGui::Checkbox("No help text",           physopts + 5);
				ImGui::Checkbox("Draw text",              physopts + 6);
				ImGui::Checkbox("Profile timings",        physopts + 7);
				ImGui::Checkbox("Sat comparison",         physopts + 8);
				ImGui::Checkbox("Disable bullet lcp",     physopts + 9);
				ImGui::Checkbox("Enable CCD",             physopts + 10);
				ImGui::Checkbox("Draw constraints",       physopts + 11);
				ImGui::Checkbox("Draw constraint limits", physopts + 12);
				ImGui::Checkbox("Fast wireframe",         physopts + 13);
				ImGui::Checkbox("Draw normals",           physopts + 14);
				ImGui::Checkbox("Draw frames",            physopts + 15);

				int mode = 0;
				for (int i = 0; i < 16; i++) {
					mode |= physopts[i] << i;
				}

				// TODO: rename 'phys' should be 'physics', think that involves
				//       renaming the 'physics' class...
				phys->setDebugMode(mode);
				ImGui::EndMenu();
			}

			ImGui::Separator();
			ImGui::MenuItem("Dear ImGui demo window", NULL, &demo_window);
			ImGui::EndMenu();
		}

		ImGui::Combo("[mode]", &mode,
			"Exit editor\0"
			"View\0"
			"Add...\0"
			"Add object\0"
			"Add point light\0"
			"Add spot light\0"
			"Add directional light\0"
			"Add reflection probe\0"
			"Add irradiance probe\0"
			"Select\0"
			"Move...\0"
			"Move along X axis\0"
			"Move along Y axis\0"
			"Move along Z axis\0"
			"Rotate...\0"
			"Rotate along X axis\0"
			"Rotate along Y axis\0"
			"Rotate along Z axis\0"
			"Scale\0"
			"Scale along X axis\0"
			"Scale along Y axis\0"
			"Scale along Z axis\0"
			"Move bounding box upper X bound\0"
			"Move bounding box upper Y bound\0"
			"Move bounding box upper Z bound\0"
			"Move bounding box lower X bound\0"
			"Move bounding box lower Y bound\0"
			"Move bounding box lower Z bound\0"
			"\0");

		setMode((enum mode)mode);
		ImGui::EndMainMenuBar();
	}

	if (demo_window) {
		ImGui::ShowDemoWindow(&demo_window);
	}

	handle_prompts(this);
}
