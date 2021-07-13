#include <grend-config.h>

#include <grend/gameEditor.hpp>
#include <grend/gameState.hpp>
#include <grend/engine.hpp>
#include <grend/glManager.hpp>
#include <grend/geometryGeneration.hpp>

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_sdl.h>
#include <imgui/backends/imgui_impl_opengl3.h>

using namespace grendx;

static gameModel::ptr  physmodel;

gameEditor::gameEditor(gameMain *game) : gameView() {
	objects = gameObject::ptr(new gameObject());
	cam->setFar(1000.0);
	/*
	testpost = makePostprocessor<rOutput>(game->rend->shaders["tonemap"],
	                                      SCREEN_SIZE_X, SCREEN_SIZE_Y);
										  */

	// don't apply post-processing filters if this is an embedded profile
	// (so no tonemapping/HDR)
#ifdef NO_FLOATING_FB
	post = renderPostChain::ptr(new renderPostChain(
		{loadPostShader(GR_PREFIX "shaders/baked/texpresent.frag",
		                game->rend->globalShaderOptions)},
		SCREEN_SIZE_X, SCREEN_SIZE_Y));
#else
	post = renderPostChain::ptr(new renderPostChain(
		//{loadPostShader(GR_PREFIX "shaders/baked/texpresent.frag",
		 //               game->rend->globalShaderOptions)},
		//{game->rend->postShaders["tonemap"], game->rend->postShaders["psaa"]},
		{game->rend->postShaders["psaa"]},
		SCREEN_SIZE_X, SCREEN_SIZE_Y));
#endif

	loading_thing = makePostprocessor<rOutput>(
		loadProgram(GR_PREFIX "shaders/baked/postprocess.vert",
		            GR_PREFIX "shaders/baked/texpresent.frag",
		            game->rend->globalShaderOptions),
		SCREEN_SIZE_X,
		SCREEN_SIZE_Y
	);

	// XXX: constructing a full shared pointer for this is a bit wasteful...
	loading_img = genTexture();
	loading_img->buffer(std::make_shared<materialTexture>(GR_PREFIX "assets/tex/loading-splash.png"));

	clear(game);
	initImgui(game);
	loadUIModels();

	auto moda = std::make_shared<gameObject>();
	auto modb = std::make_shared<gameObject>();
	physmodel = load_object(GR_PREFIX "assets/obj/smoothsphere.obj");
	compileModel("testphys", physmodel);

	/*
	setNode("lmao", moda, physmodel);
	setNode("lmao", modb, physmodel);
	setNode("fooa", game->state->physObjects, moda);
	setNode("foob", game->state->physObjects, modb);
	game->phys->addSphere(nullptr, glm::vec3(0, 10, 0), 1.0, 1.0);
	game->phys->addSphere(nullptr, glm::vec3(-10, 10, 0), 1.0, 1.0);
	*/

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
	gameObject::ptr yptr = gameObject::ptr(new clicker(this, mode::MoveY));
	gameObject::ptr zptr = gameObject::ptr(new clicker(this, mode::MoveZ));
	gameObject::ptr xrot = gameObject::ptr(new clicker(this, mode::RotateX));
	gameObject::ptr yrot = gameObject::ptr(new clicker(this, mode::RotateY));
	gameObject::ptr zrot = gameObject::ptr(new clicker(this, mode::RotateZ));
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
	auto flags = game->rend->getLightingFlags();

	renderWorld(game, cam, flags);
	renderWorldObjects(game);

	// TODO: this results in cursor not being clickable if the render
	//       object buffer has more than the stencil buffer can hold...
	//       also current stencil buffer access results in syncronizing the pipeline,
	//       need an overall better solution for clickable things

	que.add(UIObjects);

	renderFlags unshadedFlags = game->rend->getLightingFlags("unshaded");
	renderFlags constantFlags = game->rend->getLightingFlags("constant-color");
	unshadedFlags.depthTest = constantFlags.depthTest = true;
	unshadedFlags.depthMask = constantFlags.depthMask = false;

	renderQueue por = que;
	for (auto& prog : {constantFlags.mainShader,
	                   constantFlags.skinnedShader,
	                   constantFlags.instancedShader})
	{
		prog->bind();
		prog->set("outputColor", glm::vec4(0.2, 0.05, 0.0, 0.5));
	}

	// TODO: depth mode in render flags
	glDepthFunc(GL_GEQUAL);
	por.flush(game->rend->framebuffer, game->rend, constantFlags);

	por = que;
	glDepthFunc(GL_LESS);
	por.flush(game->rend->framebuffer, game->rend, unshadedFlags);

	// TODO: function to do this
	int winsize_x, winsize_y;
	SDL_GetWindowSize(game->ctx.window, &winsize_x, &winsize_y);
	// TODO: move this to input (on resize event)
	//post->setSize(winsize_x, winsize_y);
	post->setUniform("exposure", game->rend->exposure);
	// TODO: need to do this in player views too
	post->setUniform("time_ms", SDL_GetTicks() * 1.f);
	post->draw(game->rend->framebuffer);

// XXX: FIXME: imgui on es2 results in a blank screen, for whatever reason
//             the postprocessing shader doesn't see anything from the
//             render framebuffer, although the depth/stencil buffer seems
//             to be there...
//
//             also, now that I'm trying things on android, seems
//             the phone I'm testing on has a driver bug triggered
//             by one of the imgui draw calls, but only on es3... boy oh boy
#if !defined(__ANDROID__) && GLSL_VERSION > 100
	renderEditor(game);
	renderImgui(game);
#endif
}

void gameEditor::renderWorldObjects(gameMain *game) {
	DO_ERROR_CHECK();

	// XXX: wasteful, a bit wrong
	static gameObject::ptr probeObj = std::make_shared<gameObject>();
	setNode("model", probeObj, physmodel);

	renderQueue tempque(cam);
	renderQueue que(cam);
	tempque.add(game->state->rootnode);

	if (showProbes) {
		renderFlags probeFlags;
		auto& refShader = game->rend->internalShaders["refprobe_debug"];
		auto& irradShader = game->rend->internalShaders["irradprobe_debug"];

		probeFlags.mainShader = probeFlags.skinnedShader =
			probeFlags.instancedShader = probeFlags.billboardShader = refShader;

		refShader->bind();

		for (auto& [_, center, __, probe] : tempque.probes) {
			probeObj->setTransform((TRS) {
				.position = center,
				.scale    = glm::vec3(0.5),
			});

			for (unsigned i = 0; i < 6; i++) {
				std::string loc = "cubeface["+std::to_string(i)+"]";
				glm::vec3 facevec =
					game->rend->atlases.reflections->tex_vector(probe->faces[0][i]);
				refShader->set(loc, facevec);
				DO_ERROR_CHECK();
			}

			que.add(probeObj);
			DO_ERROR_CHECK();
			que.flush(game->rend->framebuffer, game->rend, probeFlags);
		}

		probeFlags.mainShader = probeFlags.skinnedShader =
			probeFlags.instancedShader = irradShader;
		irradShader->bind();

		for (auto& [_, center, __, probe] : tempque.irradProbes) {
			probeObj->setTransform((TRS) {
				.position = center,
				.scale    = glm::vec3(0.5),
			});

			for (unsigned i = 0; i < 6; i++) {
				std::string loc = "cubeface["+std::to_string(i)+"]";
				glm::vec3 facevec =
					game->rend->atlases.irradiance->tex_vector(probe->faces[i]); 
				irradShader->set(loc, facevec);
			}

			que.add(probeObj);
			que.flush(game->rend->framebuffer, game->rend, probeFlags);
		}
	}

	if (showLights) {
		renderFlags unshadedFlags = game->rend->getLightingFlags("unshaded");

		for (auto& [_, center, __, light] : tempque.lights) {
			glm::vec3 pos = center;

			if (cam->sphereInFrustum(pos, 0.5)) {
				probeObj->setTransform((TRS) {
					.position = center,
					.scale    = glm::vec3(0.5),
				});

				que.add(probeObj);
				que.flush(game->rend->framebuffer, game->rend, unshadedFlags);
			}
		}
	}
}

void gameEditor::initImgui(gameMain *game) {
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();


	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	//io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

	ImGui::StyleColorsDark();
	ImGui_ImplSDL2_InitForOpenGL(game->ctx.window, game->ctx.glcontext);
	// TODO: make the glsl version here depend on GL version/the string in
	//       shaders/version.glsl
	//ImGui_ImplOpenGL3_Init("#version 100");
	//ImGui_ImplOpenGL3_Init("#version 300 es");
	ImGui_ImplOpenGL3_Init("#version " GLSL_STRING);
}

std::pair<gameObject::ptr, modelMap> grendx::loadModel(std::string path) {
	std::string ext = filename_extension(path);
	if (ext == ".obj") {
		//model m(path);
		gameModel::ptr m = load_object(path);
		//compileModel(path, m);

		// add the model at 0,0
		auto obj = gameObject::ptr(new gameObject());
		// make up a name for .obj models
		auto fname = basenameStr(path) + ":model";
		setNode(fname, obj, m);
		//return obj;
		modelMap models = {{ fname, m }};
		return {obj, models};
	}

	else if (ext == ".gltf" || ext == ".glb") {
		modelMap mods = load_gltf_models(path);
		auto obj = std::make_shared<gameObject>();
		//compileModels(mods);

		for (auto& [name, model] : mods) {
			// add the models at 0,0
			setNode(name, obj, model);
		}

		return {obj, mods};
	}

	return {nullptr, {}};
}

std::pair<gameImport::ptr, modelMap> grendx::loadSceneData(std::string path) {
	std::string ext = filename_extension(path);

	if (ext == ".gltf" || ext == ".glb") {
		std::cerr << "load_scene(): loading scene" << std::endl;
		// TODO: this is kind of redundant now, unless I want this to also
		//       be able to load .map files from here... could be useful
		return load_gltf_scene(path);
	}

	return {nullptr, {}};
}

gameImport::ptr grendx::loadSceneCompiled(std::string path) {
	auto [obj, models] = loadSceneData(path);

	if (obj) {
		compileModels(models);
	}

	return obj;
}

gameImport::ptr grendx::loadSceneAsyncCompiled(gameMain *game, std::string path) {
	auto ret = std::make_shared<gameImport>(path);

	auto fut = game->jobs->addAsync([=] () {
		auto [obj, models] = loadSceneData(path);

		// apparently you can't (officially) capture destructured bindings, only variables...
		// ffs
		gameImport::ptr objptr = obj;
		modelMap modelptr = models;

		if (obj) {
			game->jobs->addDeferred([=] () {
				compileModels(modelptr);
				setNode("asyncLoaded", ret, objptr);
				return true;
			});
		}

		return obj != nullptr;
	});

	return ret;
}


void gameEditor::reloadShaders(gameMain *game) {
	// push everything into one vector for simplicity, this will be
	// run only from the editor so it doesn't need to be really efficient
	std::vector<std::pair<std::string, Program::ptr>> shaders;

	for (auto& [name, flags] : game->rend->lightingShaders) {
		shaders.push_back({name, flags.mainShader});
		shaders.push_back({name, flags.skinnedShader});
		shaders.push_back({name, flags.instancedShader});
	}

	for (auto& [name, flags] : game->rend->probeShaders) {
		shaders.push_back({name, flags.mainShader});
		shaders.push_back({name, flags.skinnedShader});
		shaders.push_back({name, flags.instancedShader});
	}

	for (auto& [name, prog] : game->rend->postShaders) {
		shaders.push_back({name, prog});
	}

	/*
	for (auto& [name, prog] : game->rend->internalShaders) {
		shaders.push_back({name, prog});
	}
	*/

	for (auto& [name, prog] : shaders) {
		if (!prog->reload()) {
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
	cursorBuf.position = glm::vec3(
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
		|| mode == mode::AddIrradianceProbe
		|| showAddEntityWindow;

	if (selectedNode) {
		for (auto& str : {"X-Axis", "Y-Axis", "Z-Axis",
		                  "X-Rotation", "Y-Rotation", "Z-Rotation"})
		{
			auto ptr = orientation->getNode(str);

			TRS newtrans = selectedNode->getTransformTRS();
			newtrans.scale = glm::vec3(glm::distance(newtrans.position, cam->position()) * 0.22);
			ptr->setTransform(newtrans);
		}

		if (selectedNode->type == gameObject::objType::ReflectionProbe) {
			auto bbox = UIObjects->getNode("Bounding-Box");
			auto probe = std::dynamic_pointer_cast<gameReflectionProbe>(selectedNode);

			TRS transform = probe->getTransformTRS();
			glm::vec3 bmin = transform.position + probe->boundingBox.min;
			glm::vec3 bmax = transform.position + probe->boundingBox.max;
			glm::vec3 center = 0.5f*(bmax + bmin);
			glm::vec3 extent = (bmax - bmin);

			assert(bbox != nullptr);
			bbox->visible = true;
			bbox->setTransform((TRS) {
				.position = center,
				.scale    = extent,
			});

		} else {
			auto bbox = UIObjects->getNode("Bounding-Box");
			assert(bbox != nullptr);
			bbox->visible = false;
		}

		handleMoveRotate(game);
	}


	{
		auto ptr = UIObjects->getNode("Cursor-Placement");

		if (ptr) {
			handleCursorUpdate(game);
			ptr->setTransform((TRS) { .position = cursorBuf.position, });
		}
	}

	/*
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
	*/
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

	ImGuiIO& io = ImGui::GetIO();
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
		SDL_Window *w = SDL_GL_GetCurrentWindow();
		SDL_GLContext g = SDL_GL_GetCurrentContext();
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
		SDL_GL_MakeCurrent(w, g);
	}
}

void gameEditor::renderEditor(gameMain *game) {
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplSDL2_NewFrame(game->ctx.window);
	ImGui::NewFrame();

	menubar(game);

	// TODO: this could probably be reduced to like a map of
	//       window names to states...
	//       as simple as name -> opened?
	//       or name -> pair<opened, draw()>, something like that
	//       or even name -> draw(), and being in the map implies it's opened...
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

	if (showEntitySelectWindow) {
		entitySelectWindow(game);
	}

	if (showAddEntityWindow) {
		addEntityWindow(game);
	}

	if (showEntityEditorWindow) {
		// TODO
		//entityEditorWindow(game);
	}

	if (showProfilerWindow) {
		profilerWindow(game);
	}

	if (showSettingsWindow) {
		settingsWindow(game);
	}
}
