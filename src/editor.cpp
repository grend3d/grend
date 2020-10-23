#include <grend/game_editor.hpp>
#include <grend/gameState.hpp>
#include <grend/engine.hpp>
#include <grend/gl_manager.hpp>

#include <imgui/imgui.h>
#include <imgui/examples/imgui_impl_sdl.h>
#include <imgui/examples/imgui_impl_opengl3.h>

using namespace grendx;

static gameModel::ptr  physmodel;

game_editor::game_editor(gameMain *game) : gameView() {
	objects = gameObject::ptr(new gameObject());
	/*
	testpost = makePostprocessor<rOutput>(game->rend->shaders["tonemap"],
	                                      SCREEN_SIZE_X, SCREEN_SIZE_Y);
										  */
	post = renderPostChain::ptr(new renderPostChain(
		{game->rend->shaders["tonemap"]},
		SCREEN_SIZE_X, SCREEN_SIZE_Y));

	loading_thing = makePostprocessor<rOutput>(
		load_program(GR_PREFIX "shaders/out/postprocess.vert",
		             GR_PREFIX "shaders/out/texpresent.frag"),
		SCREEN_SIZE_X,
		SCREEN_SIZE_Y
	);

	loading_img = gen_texture();
	loading_img->buffer(materialTexture(GR_PREFIX "assets/tex/loading-splash.png"));

	clear(game);
	initImgui(game);
	loadUIModels();

	auto moda = std::make_shared<gameObject>();
	auto modb = std::make_shared<gameObject>();
	physmodel = load_object(GR_PREFIX "assets/obj/smoothsphere.obj");
	compile_model("testphys", physmodel);

	setNode("lmao", moda, physmodel);
	setNode("lmao", modb, physmodel);
	setNode("fooa", game->state->physObjects, moda);
	setNode("foob", game->state->physObjects, modb);
	game->phys->add_sphere(moda, glm::vec3(0, 10, 0), 1.0, 1.0);
	game->phys->add_sphere(modb, glm::vec3(-10, 10, 0), 1.0, 1.0);

	bind_cooked_meshes();
	loadInputBindings(game);
	set_mode(mode::View);

	// XXX
	show_object_editor_window = true;
};

class clicker : public gameObject {
	public:
		clicker(game_editor *ptr, enum game_editor::mode click)
			: gameObject(), editor(ptr), clickmode(click) {}; 

		virtual void onLeftClick() {
			std::cerr << "have mode: " << clickmode << std::endl;
			editor->set_mode(clickmode);

			if (parent) {
				parent->onLeftClick();
			}
		}

		game_editor *editor;
		enum game_editor::mode clickmode;
};

void game_editor::loadUIModels(void) {
	// TODO: Need to swap Z/Y pointer and spinner models
	//       blender coordinate system isn't the same as opengl's (duh)
	std::string dir = GR_PREFIX "assets/obj/UI/";
	UI_models["X-Axis-Pointer"] = load_object(dir + "X-Axis-Pointer.obj");
	UI_models["Y-Axis-Pointer"] = load_object(dir + "Y-Axis-Pointer.obj");
	UI_models["Z-Axis-Pointer"] = load_object(dir + "Z-Axis-Pointer.obj");
	UI_models["X-Axis-Rotation-Spinner"]
		= load_object(dir + "X-Axis-Rotation-Spinner.obj");
	UI_models["Y-Axis-Rotation-Spinner"]
		= load_object(dir + "Y-Axis-Rotation-Spinner.obj");
	UI_models["Z-Axis-Rotation-Spinner"]
		= load_object(dir + "Z-Axis-Rotation-Spinner.obj");
	UI_models["Cursor-Placement"]
		= load_object(dir + "Cursor-Placement.obj");
	UI_models["Bounding-Box"] = generate_cuboid(1.f, 1.f, 1.f);

	UI_objects = gameObject::ptr(new gameObject());
	gameObject::ptr xptr = gameObject::ptr(new clicker(this, mode::MoveX));
	gameObject::ptr yptr = gameObject::ptr(new clicker(this, mode::MoveZ));
	gameObject::ptr zptr = gameObject::ptr(new clicker(this, mode::MoveY));
	gameObject::ptr xrot = gameObject::ptr(new clicker(this, mode::RotateX));
	gameObject::ptr yrot = gameObject::ptr(new clicker(this, mode::RotateZ));
	gameObject::ptr zrot = gameObject::ptr(new clicker(this, mode::RotateY));
	gameObject::ptr cursor = gameObject::ptr(new gameObject());
	gameObject::ptr bbox   = gameObject::ptr(new gameObject());

	setNode("X-Axis",           xptr,   UI_models["X-Axis-Pointer"]);
	setNode("X-Rotation",       xrot,   UI_models["X-Axis-Rotation-Spinner"]);
	setNode("Y-Axis",           yptr,   UI_models["Y-Axis-Pointer"]);
	setNode("Y-Rotation",       yrot,   UI_models["Y-Axis-Rotation-Spinner"]);
	setNode("Z-Axis",           zptr,   UI_models["Z-Axis-Pointer"]);
	setNode("Z-Rotation",       zrot,   UI_models["Z-Axis-Rotation-Spinner"]);
	setNode("Cursor-Placement", cursor, UI_models["Cursor-Placement"]);
	setNode("Bounding-Box",     bbox,   UI_models["Bounding-Box"]);

	setNode("X-Axis", UI_objects, xptr);
	setNode("Y-Axis", UI_objects, yptr);
	setNode("Z-Axis", UI_objects, zptr);
	setNode("X-Rotation", UI_objects, xrot);
	setNode("Y-Rotation", UI_objects, yrot);
	setNode("Z-Rotation", UI_objects, zrot);
	setNode("Cursor-Placement", UI_objects, cursor);
	setNode("Bounding-Box", UI_objects, bbox);

	bbox->visible = false;
	compile_models(UI_models);
	bind_cooked_meshes();
}

void game_editor::render(gameMain *game) {
	renderQueue que(cam);
	auto flags = game->rend->getFlags();

	// draw UI objects before others, to avoid UI objects not being in the
	// stencil buffer if the stencil counter overflows (and so not being
	// clickable)
	// TODO: maybe attach shaders to gameObjects
	// TODO: skinned unshaded shader
	auto unshadedFlags = flags;
	unshadedFlags.mainShader = unshadedFlags.skinnedShader
		= game->rend->shaders["unshaded"];
	game->rend->setFlags(unshadedFlags);
	que.add(UI_objects);
	que.flush(game->rend->framebuffer, game->rend);
	game->rend->setFlags(game->rend->getDefaultFlags());
	renderWorld(game, cam);

	// TODO: function to do this
	int winsize_x, winsize_y;
	SDL_GetWindowSize(game->ctx.window, &winsize_x, &winsize_y);
	//post->setSize(winsize_x, winsize_y);
	post->draw(game->rend->framebuffer);

// XXX: FIXME: imgui on es2 results in a blank screen, for whatever reason
//             the postprocessing shader doesn't see anything from the
//             render framebuffer, although the depth/stencil buffer seems
//             to be there...
#if GLSL_VERSION > 100
	render_editor(game);
	render_imgui(game);
#endif
}

void game_editor::initImgui(gameMain *game) {
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGui::StyleColorsDark();
	ImGui_ImplSDL2_InitForOpenGL(game->ctx.window, game->ctx.glcontext);
	// TODO: make the glsl version here depend on GL version/the string in
	//       shaders/version.glsl
	//ImGui_ImplOpenGL3_Init("#version 100");
	//ImGui_ImplOpenGL3_Init("#version 300 es");
	ImGui_ImplOpenGL3_Init("#version " GLSL_STRING);
}

gameObject::ptr grendx::loadModel(std::string path) {
	std::string ext = filename_extension(path);
	if (ext == ".obj") {
		//model m(path);
		gameModel::ptr m = load_object(path);
		compile_model(path, m);

		// add the model at 0,0
		auto obj = gameObject::ptr(new gameObject());
		// make up a name for .obj models
		auto fname = basenameStr(path) + ":model";
		setNode(fname, obj, m);
		return obj;
	}

	else if (ext == ".gltf" || ext == ".glb") {
		model_map mods = load_gltf_models(path);
		compile_models(mods);

		for (auto& [name, model] : mods) {
			// add the models at 0,0
			auto obj = gameObject::ptr(new gameObject());
			setNode(name, obj, model);
			return obj;
		}
	}

	return nullptr;
}

gameImport::ptr grendx::loadScene(std::string path) {
	std::string ext = filename_extension(path);

	if (ext == ".gltf" || ext == ".glb") {
		auto [objs, mods] = load_gltf_scene(path);
		std::cerr << "load_scene(): loading scene" << std::endl;

		std::string import_name = "import["+std::to_string(objs->id)+"]";
		compile_models(mods);
		return objs;
	}

	return nullptr;
}

void game_editor::reload_shaders(gameMain *game) {
	for (auto& [name, shader] : game->rend->shaders) {
		if (!shader->reload()) {
			std::cerr << ">> couldn't reload shader: " << name << std::endl;
		}
	}
}

void game_editor::set_mode(enum mode newmode) {
	mode = newmode;
	inputBinds.setMode(mode);
}

void game_editor::handleCursorUpdate(gameMain *game) {
	// TODO: reuse this for cursor code
	auto align = [&] (float x) { return floor(x * fidelity)/fidelity; };
	entbuf.position = glm::vec3(
		align(cam->direction().x*edit_distance + cam->position().x),
		align(cam->direction().y*edit_distance + cam->position().y),
		align(cam->direction().z*edit_distance + cam->position().z));
}

void game_editor::logic(gameMain *game, float delta) {
	cam->updatePosition(delta);

	if (selectedNode) {
		for (auto& str : {"X-Axis", "Y-Axis", "Z-Axis",
		                  "X-Rotation", "Y-Rotation", "Z-Rotation"})
		{
			auto ptr = UI_objects->getNode(str);

			ptr->transform = selectedNode->transform;
			ptr->transform.scale = glm::vec3(1, 1, 1); // don't use selected scale
		}

		if (selectedNode->type == gameObject::objType::ReflectionProbe) {
			auto bbox = UI_objects->getNode("Bounding-Box");
			auto probe = std::dynamic_pointer_cast<gameReflectionProbe>(selectedNode);

			glm::vec3 bmin = probe->transform.position + probe->boundingBox.min;
			glm::vec3 bmax = probe->transform.position + probe->boundingBox.max;
			glm::vec3 center = 0.5f*(bmax + bmin);
			glm::vec3 extent = (bmax - bmin);

			assert(bbox != nullptr);
			bbox->visible = true;
			bbox->transform.position = center;
			bbox->transform.scale    = extent;

		} else {
			auto bbox = UI_objects->getNode("Bounding-Box");
			assert(bbox != nullptr);
			bbox->visible = false;
		}
	}

	handleMoveRotate(game);

	switch (mode) {
		case mode::AddObject:
		case mode::AddPointLight:
		case mode::AddSpotLight:
		case mode::AddDirectionalLight:
		case mode::AddReflectionProbe:
			{
				auto ptr = UI_objects->getNode("Cursor-Placement");
				assert(ptr != nullptr);
				handleCursorUpdate(game);
				ptr->transform.position = entbuf.position;
				break;
			}
		default: break;
	}
}


void game_editor::showLoadingScreen(gameMain *game) {
	// TODO: maybe not this
	loading_thing->draw(loading_img, nullptr);
	SDL_GL_SwapWindow(game->ctx.window);
}

bool game_editor::isUIObject(gameObject::ptr obj) {
	for (gameObject::ptr temp = obj; temp; temp = temp->parent) {
		if (temp == UI_objects) {
			return true;
		}
	}

	return false;
}

gameObject::ptr game_editor::getNonModel(gameObject::ptr obj) {
	for (gameObject::ptr temp = obj; temp; temp = temp->parent) {
		if (temp->type != gameObject::objType::Mesh
		    && temp->type != gameObject::objType::Model)
		{
			return temp;
		}
	}

	return nullptr;
}

void game_editor::clear(gameMain *game) {
	showLoadingScreen(game);

	cam->setPosition({0, 0, 0});
	models.clear();

	// TODO: clear() for state
	selectedNode = game->state->rootnode = gameObject::ptr(new gameObject());
}

// TODO: rename 'renderer' to 'rend' or something
void game_editor::render_imgui(gameMain *game) {
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void game_editor::render_editor(gameMain *game) {
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplSDL2_NewFrame(game->ctx.window);
	ImGui::NewFrame();

	menubar(game);

	if (show_metrics_window) {
		//ImGui::ShowMetricsWindow();
		metricsWindow(game);
	}

	if (show_map_window) {
		map_window(game);
	}

	if (show_object_select_window) {
		object_select_window(game);
	}

	if (selectedNode && show_object_editor_window) {
		objectEditorWindow(game);
	}
}
