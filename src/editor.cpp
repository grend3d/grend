#include <grend/gameEditor.hpp>
#include <grend/gameState.hpp>
#include <grend/engine.hpp>
#include <grend/glManager.hpp>
#include <grend/geometryGeneration.hpp>

#include <imgui/imgui.h>
#include <imgui/examples/imgui_impl_sdl.h>
#include <imgui/examples/imgui_impl_opengl3.h>

using namespace grendx;

static gameModel::ptr  physmodel;

gameEditor::gameEditor(gameMain *game) : gameView() {
	objects = gameObject::ptr(new gameObject());
	cam->setFar(1000.0);
	/*
	testpost = makePostprocessor<rOutput>(game->rend->shaders["tonemap"],
	                                      SCREEN_SIZE_X, SCREEN_SIZE_Y);
										  */
	post = renderPostChain::ptr(new renderPostChain(
		{game->rend->shaders["tonemap"], game->rend->shaders["psaa"]},
		SCREEN_SIZE_X, SCREEN_SIZE_Y));

	loading_thing = makePostprocessor<rOutput>(
		loadProgram(GR_PREFIX "shaders/out/postprocess.vert",
		             GR_PREFIX "shaders/out/texpresent.frag"),
		SCREEN_SIZE_X,
		SCREEN_SIZE_Y
	);

	loading_img = genTexture();
	loading_img->buffer(materialTexture(GR_PREFIX "assets/tex/loading-splash.png"));

	clear(game);
	initImgui(game);
	loadUIModels();

	auto moda = std::make_shared<gameObject>();
	auto modb = std::make_shared<gameObject>();
	physmodel = load_object(GR_PREFIX "assets/obj/smoothsphere.obj");
	compileModel("testphys", physmodel);

	setNode("lmao", moda, physmodel);
	setNode("lmao", modb, physmodel);
	setNode("fooa", game->state->physObjects, moda);
	setNode("foob", game->state->physObjects, modb);
	game->phys->addSphere(moda, glm::vec3(0, 10, 0), 1.0, 1.0);
	game->phys->addSphere(modb, glm::vec3(-10, 10, 0), 1.0, 1.0);

	bindCookedMeshes();
	loadInputBindings(game);
	setMode(mode::View);

	// XXX
	showObjectEditorWindow = true;
};

class clicker : public gameObject {
	public:
		clicker(gameEditor *ptr, enum gameEditor::mode click)
			: gameObject(), editor(ptr), clickmode(click) {}; 

		virtual void onLeftClick() {
			std::cerr << "have mode: " << clickmode << std::endl;
			editor->setMode(clickmode);

			if (auto p = parent.lock()) {
				p->onLeftClick();
			}
		}

		gameEditor *editor;
		enum gameEditor::mode clickmode;
};

void gameEditor::loadUIModels(void) {
	// TODO: Need to swap Z/Y pointer and spinner models
	//       blender coordinate system isn't the same as opengl's (duh)
	std::string dir = GR_PREFIX "assets/obj/UI/";
	UIModels["X-Axis-Pointer"] = load_object(dir + "X-Axis-Pointer.obj");
	UIModels["Y-Axis-Pointer"] = load_object(dir + "Y-Axis-Pointer.obj");
	UIModels["Z-Axis-Pointer"] = load_object(dir + "Z-Axis-Pointer.obj");
	UIModels["X-Axis-Rotation-Spinner"]
		= load_object(dir + "X-Axis-Rotation-Spinner.obj");
	UIModels["Y-Axis-Rotation-Spinner"]
		= load_object(dir + "Y-Axis-Rotation-Spinner.obj");
	UIModels["Z-Axis-Rotation-Spinner"]
		= load_object(dir + "Z-Axis-Rotation-Spinner.obj");
	UIModels["Cursor-Placement"]
		= load_object(dir + "Cursor-Placement.obj");
	UIModels["Bounding-Box"] = generate_cuboid(1.f, 1.f, 1.f);

	UIObjects = gameObject::ptr(new gameObject());
	gameObject::ptr xptr = gameObject::ptr(new clicker(this, mode::MoveX));
	gameObject::ptr yptr = gameObject::ptr(new clicker(this, mode::MoveZ));
	gameObject::ptr zptr = gameObject::ptr(new clicker(this, mode::MoveY));
	gameObject::ptr xrot = gameObject::ptr(new clicker(this, mode::RotateX));
	gameObject::ptr yrot = gameObject::ptr(new clicker(this, mode::RotateZ));
	gameObject::ptr zrot = gameObject::ptr(new clicker(this, mode::RotateY));
	gameObject::ptr orientation = gameObject::ptr(new gameObject());
	gameObject::ptr cursor      = gameObject::ptr(new gameObject());
	gameObject::ptr bbox        = gameObject::ptr(new gameObject());

	setNode("X-Axis",           xptr,   UIModels["X-Axis-Pointer"]);
	setNode("X-Rotation",       xrot,   UIModels["X-Axis-Rotation-Spinner"]);
	setNode("Y-Axis",           yptr,   UIModels["Y-Axis-Pointer"]);
	setNode("Y-Rotation",       yrot,   UIModels["Y-Axis-Rotation-Spinner"]);
	setNode("Z-Axis",           zptr,   UIModels["Z-Axis-Pointer"]);
	setNode("Z-Rotation",       zrot,   UIModels["Z-Axis-Rotation-Spinner"]);
	setNode("Cursor-Placement", cursor, UIModels["Cursor-Placement"]);
	setNode("Bounding-Box",     bbox,   UIModels["Bounding-Box"]);

	setNode("X-Axis", orientation, xptr);
	setNode("Y-Axis", orientation, yptr);
	setNode("Z-Axis", orientation, zptr);
	setNode("X-Rotation", orientation, xrot);
	setNode("Y-Rotation", orientation, yrot);
	setNode("Z-Rotation", orientation, zrot);

	setNode("Orientation-Indicator", UIObjects, orientation);
	setNode("Cursor-Placement", UIObjects, cursor);
	setNode("Bounding-Box", UIObjects, bbox);

	bbox->visible = false;
	orientation->visible = false;
	cursor->visible = false;

	compileModels(UIModels);
	bindCookedMeshes();
}

void gameEditor::render(gameMain *game) {
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
	que.add(UIObjects);
	que.flush(game->rend->framebuffer, game->rend);

	game->rend->setFlags(game->rend->getDefaultFlags());
	renderWorld(game, cam);
	renderWorldObjects(game);

	// TODO: function to do this
	int winsize_x, winsize_y;
	SDL_GetWindowSize(game->ctx.window, &winsize_x, &winsize_y);
	// TODO: move this to input (on resize event)
	//post->setSize(winsize_x, winsize_y);
	post->draw(game->rend->framebuffer);

// XXX: FIXME: imgui on es2 results in a blank screen, for whatever reason
//             the postprocessing shader doesn't see anything from the
//             render framebuffer, although the depth/stencil buffer seems
//             to be there...
#if GLSL_VERSION > 100
	renderEditor(game);
	renderImgui(game);
#endif
}

void gameEditor::renderWorldObjects(gameMain *game) {
	auto flags = game->rend->getFlags();

	// XXX: wasteful, a bit wrong
	static gameObject::ptr probeObj = std::make_shared<gameObject>();
	setNode("model", probeObj, physmodel);

	renderQueue tempque(cam);
	renderQueue que(cam);
	tempque.add(game->state->rootnode);

	if (showProbes) {
		auto  probeFlags = flags;
		auto& refShader = game->rend->shaders["refprobe_debug"];
		auto& irradShader = game->rend->shaders["irradprobe_debug"];

		probeFlags.mainShader = probeFlags.skinnedShader = refShader;
		game->rend->setFlags(probeFlags);
		refShader->bind();

		for (auto& [_, __, probe] : tempque.probes) {
			probeObj->transform.position = probe->transform.position;
			probeObj->transform.scale    = glm::vec3(0.5);

			for (unsigned i = 0; i < 6; i++) {
				std::string loc = "cubeface["+std::to_string(i)+"]";
				glm::vec3 facevec =
					game->rend->atlases.reflections->tex_vector(probe->faces[0][i]);
				refShader->set(loc, facevec);
			}

			que.add(probeObj);
			que.flush(game->rend->framebuffer, game->rend);
		}

		probeFlags.mainShader = probeFlags.skinnedShader = irradShader;
		game->rend->setFlags(probeFlags);
		irradShader->bind();

		for (auto& [_, __, probe] : tempque.irradProbes) {
			probeObj->transform.position = probe->transform.position;
			probeObj->transform.scale    = glm::vec3(0.5);

			for (unsigned i = 0; i < 6; i++) {
				std::string loc = "cubeface["+std::to_string(i)+"]";
				glm::vec3 facevec =
					game->rend->atlases.irradiance->tex_vector(probe->faces[i]); 
				irradShader->set(loc, facevec);
			}

			que.add(probeObj);
			que.flush(game->rend->framebuffer, game->rend);
		}
	}

	if (showLights) {
		auto unshadedFlags = flags;
		unshadedFlags.mainShader = unshadedFlags.skinnedShader
			= game->rend->shaders["unshaded"];

		game->rend->setFlags(unshadedFlags);
		for (auto& [_, __, light] : tempque.lights) {
			probeObj->transform.position = light->transform.position;
			probeObj->transform.scale = glm::vec3(0.5);
			que.add(probeObj);
			que.flush(game->rend->framebuffer, game->rend);
		}
	}
}

void gameEditor::initImgui(gameMain *game) {
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
		compileModel(path, m);

		// add the model at 0,0
		auto obj = gameObject::ptr(new gameObject());
		// make up a name for .obj models
		auto fname = basenameStr(path) + ":model";
		setNode(fname, obj, m);
		return obj;
	}

	else if (ext == ".gltf" || ext == ".glb") {
		modelMap mods = load_gltf_models(path);
		compileModels(mods);

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
		compileModels(mods);
		return objs;
	}

	return nullptr;
}

void gameEditor::reloadShaders(gameMain *game) {
	for (auto& [name, shader] : game->rend->shaders) {
		if (!shader->reload()) {
			std::cerr << ">> couldn't reload shader: " << name << std::endl;
		}
	}
}

void gameEditor::setMode(enum mode newmode) {
	mode = newmode;
	inputBinds.setMode(mode);
}

void gameEditor::handleCursorUpdate(gameMain *game) {
	// TODO: reuse this for cursor code
	auto align = [&] (float x) { return floor(x * fidelity)/fidelity; };
	entbuf.position = glm::vec3(
		align(cam->direction().x*editDistance + cam->position().x),
		align(cam->direction().y*editDistance + cam->position().y),
		align(cam->direction().z*editDistance + cam->position().z));
}

void gameEditor::logic(gameMain *game, float delta) {
	cam->updatePosition(delta);

	auto orientation = UIObjects->getNode("Orientation-Indicator");
	auto cursor      = UIObjects->getNode("Cursor-Placement");
	assert(orientation && cursor);

	orientation->visible =
		   selectedNode != nullptr
		&& !selectedNode->parent.expired()
		&& selectedNode != game->state->rootnode;

	cursor->visible =
		   mode == mode::AddObject
		|| mode == mode::AddPointLight
		|| mode == mode::AddSpotLight
		|| mode == mode::AddDirectionalLight
		|| mode == mode::AddReflectionProbe
		|| mode == mode::AddIrradianceProbe;

	if (selectedNode) {
		for (auto& str : {"X-Axis", "Y-Axis", "Z-Axis",
		                  "X-Rotation", "Y-Rotation", "Z-Rotation"})
		{
			auto ptr = orientation->getNode(str);

			ptr->transform = selectedNode->transform;
			ptr->transform.scale = glm::vec3(1, 1, 1); // don't use selected scale
		}

		if (selectedNode->type == gameObject::objType::ReflectionProbe) {
			auto bbox = UIObjects->getNode("Bounding-Box");
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
			auto bbox = UIObjects->getNode("Bounding-Box");
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
		case mode::AddIrradianceProbe:
			{
				auto ptr = UIObjects->getNode("Cursor-Placement");
				assert(ptr != nullptr);
				handleCursorUpdate(game);
				ptr->transform.position = entbuf.position;
				break;
			}
		default: break;
	}
}


void gameEditor::showLoadingScreen(gameMain *game) {
	// TODO: maybe not this
	loading_thing->draw(loading_img, nullptr);
	SDL_GL_SwapWindow(game->ctx.window);
}

bool gameEditor::isUIObject(gameObject::ptr obj) {
	for (gameObject::ptr temp = obj; temp; temp = temp->parent.lock()) {
		if (temp == UIObjects) {
			return true;
		}
	}

	return false;
}

gameObject::ptr gameEditor::getNonModel(gameObject::ptr obj) {
	for (gameObject::ptr temp = obj; temp; temp = temp->parent.lock()) {
		if (temp->type != gameObject::objType::Mesh
		    && temp->type != gameObject::objType::Model)
		{
			return temp;
		}
	}

	return nullptr;
}

void gameEditor::clear(gameMain *game) {
	showLoadingScreen(game);

	cam->setPosition({0, 0, 0});
	models.clear();

	// TODO: clear() for state
	selectedNode = game->state->rootnode = gameObject::ptr(new gameObject());
}

// TODO: rename 'renderer' to 'rend' or something
void gameEditor::renderImgui(gameMain *game) {
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void gameEditor::renderEditor(gameMain *game) {
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplSDL2_NewFrame(game->ctx.window);
	ImGui::NewFrame();

	menubar(game);

	if (showMetricsWindow) {
		//ImGui::ShowMetricsWindow();
		metricsWindow(game);
	}

	if (showMapWindow) {
		mapWindow(game);
	}

	if (showObjectSelectWindow) {
		objectSelectWindow(game);
	}

	if (selectedNode && showObjectEditorWindow) {
		objectEditorWindow(game);
	}
}
