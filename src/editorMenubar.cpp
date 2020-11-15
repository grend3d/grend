#include <grend/gameEditor.hpp>
#include <grend/fileDialog.hpp>

// TODO: debugging, remove
#include <iostream>

#include <imgui/imgui.h>
#include <imgui/examples/imgui_impl_sdl.h>
#include <imgui/examples/imgui_impl_opengl3.h>

using namespace grendx;

static fileDialog open_dialog("Open File");
static fileDialog save_as_dialog("Save As...");

static fileDialog import_model_dialog("Import Model");
static fileDialog import_scene_dialog("Import Scene");
static fileDialog import_map_dialog("Import Map");

static void handle_prompts(gameEditor *editor, gameMain *game) {
	if (open_dialog.promptFilename()) {
		std::cout << "Opening a file here! at " << open_dialog.selection <<  std::endl;
		open_dialog.clear();

		// TODO: need function to clear state + set new root node
		editor->clear(game);
		editor->selectedNode = game->state->rootnode
			= loadMap(game, open_dialog.selection);
	}

	if (save_as_dialog.promptFilename()) {
		std::cout << "Saving as a file! at " << save_as_dialog.selection << std::endl;

		// TODO: some way to save a subnode as it's own map
		//saveMap(game, editor->selectedNode, save_as_dialog.selection);
		saveMap(game, game->state->rootnode, save_as_dialog.selection);
		save_as_dialog.clear();
	}

	if (import_model_dialog.promptFilename()) {
		std::cout << "Importing a thing! at "
		          << import_model_dialog.selection << std::endl;
		import_model_dialog.clear();

		// TODO: XXX: no need for const if it's just being casted away anyway...
		auto obj = loadModel(import_model_dialog.selection);
		std::string name = "model["+std::to_string(obj->id)+"]";
		setNode(name, editor->selectedNode, obj);
		bindCookedMeshes();
	}

	if (import_scene_dialog.promptFilename()) {
		std::cout << "Importing a scene! at "
		          << import_scene_dialog.selection << std::endl;
		import_scene_dialog.clear();

		auto obj = loadScene(import_scene_dialog.selection);
		std::string name = "import["+std::to_string(obj->id)+"]";
		setNode(name, editor->selectedNode, obj);
		bindCookedMeshes();
	}

	if (import_map_dialog.promptFilename()) {
		std::cout << "Importing a map! at "
		          << import_map_dialog.selection << std::endl;
		import_scene_dialog.clear();
	}
}

void gameEditor::menubar(gameMain *game) {
	static bool demo_window = false;

	if (ImGui::BeginMainMenuBar()) {
		if (ImGui::BeginMenu("File")) {
			if (ImGui::MenuItem("New", "CTRL+N")) {
				// TODO: "discard without saving" confirmation
				// TODO: need function to clear state + set new root node
				selectedNode = game->state->rootnode
					= std::make_shared<gameObject>();
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
			if (ImGui::MenuItem("Reload shaders", "CTRL+R")){ reloadShaders(game); }

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

		if (ImGui::BeginMenu("Options")) {
			int proj = (int)cam->project();
			float scale = cam->scale();

			ImGui::SliderFloat("Snap increment", &fidelity,
			                  0.5f, 50.f, "%.1f");
			ImGui::SliderFloat("Movement speed", &movementSpeed,
			                  1.f, 100.f, "%.1f");
			ImGui::SliderFloat("Exposure (tonemapping)", &exposure, 0.1, 10.f);
			ImGui::SliderFloat("Light threshold", &lightThreshold,
			                   0.001, 1.f);

			ImGui::Combo("Projection", &proj, "Perspective\0Orthographic\0");
			ImGui::SliderFloat("Projection scale", &scale, 0.001, 0.2f);

			ImGui::Checkbox("Show environment probes", &showProbes);
			ImGui::Checkbox("Show lights", &showLights);

			ImGui::EndMenu();

			cam->setProjection((enum camera::projection)proj);
			cam->setScale(scale);
		}

		if (ImGui::BeginMenu("Objects")) {
			if (ImGui::MenuItem("Map editor", "o"))
				showMapWindow = true;
			if (ImGui::MenuItem("Object editor", "r"))
				showObjectEditorWindow = true;
			if (ImGui::MenuItem("Object selection"))
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
				if (game->state->rootnode) {
					invalidateLightMaps(game->state->rootnode);
				}
			}

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Debug")) {
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

	handle_prompts(this, game);
}
