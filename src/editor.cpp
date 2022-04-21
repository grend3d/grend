#include <grend-config.h>

#include <grend/gameEditor.hpp>
#include <grend/gameState.hpp>
#include <grend/engine.hpp>
#include <grend/glManager.hpp>
#include <grend/geometryGeneration.hpp>
#include <grend/loadScene.hpp>
#include <grend/scancodes.hpp>

#include <grend/ecs/ecs.hpp>
#include <grend/ecs/shader.hpp>
#include <grend/ecs/sceneComponent.hpp>

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_sdl.h>
#include <imgui/backends/imgui_impl_opengl3.h>

using namespace grendx;

static sceneModel::ptr  physmodel;

gameEditor::gameEditor(gameMain *game)
	: gameView()
{
	objects = sceneNode::ptr(new sceneNode());
	cam->setFar(1000.0);

	// don't apply post-processing filters if this is an embedded profile
	// (so no tonemapping/HDR)
#ifdef NO_FLOATING_FB
	post = renderPostChain::ptr(new renderPostChain(
		{loadPostShader(GR_PREFIX "shaders/baked/texpresent.frag",
		                game->rend->globalShaderOptions)},
		game->settings.targetResX, game->settings.targetResY));
#else
	post = renderPostChain::ptr(new renderPostChain(
		{loadPostShader(GR_PREFIX "shaders/baked/dither.frag",
		                game->rend->globalShaderOptions)},
		game->settings.targetResX, game->settings.targetResY));
#endif

	loading_thing = makePostprocessor<rOutput>(
		loadProgram(GR_PREFIX "shaders/baked/postprocess.vert",
		            GR_PREFIX "shaders/baked/texpresent.frag",
		            game->rend->globalShaderOptions),
		game->settings.targetResX, game->settings.targetResY
	);

	// XXX: constructing a full shared pointer for this is a bit wasteful...
	loading_img = genTexture();
	loading_img->buffer(std::make_shared<materialTexture>(GR_PREFIX "assets/tex/loading-splash.png"));

	clear(game);
	initImgui(game);
	loadUIModels();

	auto moda = std::make_shared<sceneNode>();
	auto modb = std::make_shared<sceneNode>();
	physmodel = load_object(GR_PREFIX "assets/obj/smoothsphere.obj");
	compileModel("testphys", physmodel);

	loadInputBindings(game);
	setMode(mode::View);

	// XXX
	showObjectEditorWindow = true;
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

	UIObjects = sceneNode::ptr(new sceneNode());
	sceneNode::ptr xptr = std::make_shared<sceneNode>();
	sceneNode::ptr yptr = std::make_shared<sceneNode>();
	sceneNode::ptr zptr = std::make_shared<sceneNode>();
	sceneNode::ptr xrot = std::make_shared<sceneNode>();
	sceneNode::ptr yrot = std::make_shared<sceneNode>();
	sceneNode::ptr zrot = std::make_shared<sceneNode>();
	//sceneNode::ptr xptr = sceneNode::ptr(new clicker(this, mode::MoveX));
	//sceneNode::ptr yptr = sceneNode::ptr(new clicker(this, mode::MoveY));
	//sceneNode::ptr zptr = sceneNode::ptr(new clicker(this, mode::MoveZ));
	//sceneNode::ptr xrot = sceneNode::ptr(new clicker(this, mode::RotateX));
	//sceneNode::ptr yrot = sceneNode::ptr(new clicker(this, mode::RotateY));
	//sceneNode::ptr zrot = sceneNode::ptr(new clicker(this, mode::RotateZ));
	sceneNode::ptr orientation = std::make_shared<sceneNode>();
	sceneNode::ptr cursor      = std::make_shared<sceneNode>();
	sceneNode::ptr bbox        = std::make_shared<sceneNode>();

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
}

using entClicks = std::vector<std::pair<uint32_t, grendx::ecs::entity*>>;

grendx::multiRenderQueue
buildClickableQueue(gameMain *game,
                    camera::ptr cam,
                    entClicks& clicks)
{
	using namespace grendx;
	using namespace ecs;
	//auto drawable = searchEntities(game->entities.get(), {getTypeName<abstractShader>()});
	auto drawable = game->entities->search<abstractShader>();

	multiRenderQueue que;
	uint32_t renderID = 10;

	for (entity *ent : drawable) {
		auto flags = ent->get<abstractShader>();
		auto trans = ent->node->getTransformMatrix();
		auto scenes = ent->getAll<sceneComponent>();
		// TODO: do clicky things

		renderFlags foo = flags->getShader();
		foo.features |= renderFlags::Features::StencilTest;

		que.add(foo, ent->node, renderID);
		clicks.push_back({renderID, ent});

		for (auto it = scenes.first; it != scenes.second; it++) {
			sceneComponent *comp = static_cast<sceneComponent*>(it->second);
			que.add(foo, comp->getNode(), renderID, trans);
		}

		renderID++;
	}

	return que;
}

void gameEditor::render(gameMain *game, renderFramebuffer::ptr fb) {
	renderQueue que;
	renderFlags flags = game->rend->getLightingFlags();
	flags.features |= renderFlags::Features::StencilTest;

	// TODO: avoid clearing then reallocating all of this state...
	//       lots of allocations
	clickState.clear();
	auto world = buildClickableQueue(game, cam, clickState);
	world.add(flags, game->state->rootnode);
	drawMultiQueue(game, world, fb, cam);
	renderWorldObjects(game);

	auto p = UIObjects->getNode("Orientation-Indicator");
	auto m = p->getTransformMatrix();

	if (selectedNode) {
		que.add(p->nodes["X-Axis"],     1, m);
		que.add(p->nodes["Y-Axis"],     2, m);
		que.add(p->nodes["Z-Axis"],     3, m);
		que.add(p->nodes["X-Rotation"], 4, m);
		que.add(p->nodes["Y-Rotation"], 5, m);
		que.add(p->nodes["Z-Rotation"], 6, m);
	}

	auto cursor = UIObjects->getNode("Cursor-Placement");
	auto bbox   = UIObjects->getNode("Bounding-Box");
	que.add(cursor, 0, m);
	que.add(bbox,   0, m);

	renderFlags unshadedFlags = game->rend->getLightingFlags("unshaded");
	renderFlags constantFlags = game->rend->getLightingFlags("constant-color");

	constantFlags.features |=  renderFlags::Features::DepthTest;
	constantFlags.features |=  renderFlags::Features::StencilTest;
	constantFlags.features &= ~renderFlags::Features::DepthMask;
	unshadedFlags.features = constantFlags.features;

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
	flush(por, cam, game->rend->framebuffer, game->rend, constantFlags);

	por = que;
	glDepthFunc(GL_LESS);
	flush(por, cam, game->rend->framebuffer, game->rend, unshadedFlags);

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
	static sceneNode::ptr probeObj = std::make_shared<sceneNode>();
	setNode("model", probeObj, physmodel);

	renderQueue tempque;
	renderQueue que;
	tempque.add(game->state->rootnode);

	if (showProbes) {
		renderFlags probeFlags;
		auto& refShader = game->rend->internalShaders["refprobe_debug"];
		auto& irradShader = game->rend->internalShaders["irradprobe_debug"];

		probeFlags.mainShader = probeFlags.skinnedShader =
			probeFlags.instancedShader = probeFlags.billboardShader = refShader;

		refShader->bind();

		for (auto& entry : tempque.probes) {
			probeObj->setTransform((TRS) {
				.position = entry.center,
				.scale    = glm::vec3(0.5),
			});

			for (unsigned i = 0; i < 6; i++) {
				std::string loc = "cubeface["+std::to_string(i)+"]";
				glm::vec3 facevec =
					game->rend->atlases.reflections->tex_vector(entry.data->faces[0][i]);
				refShader->set(loc, facevec);
				DO_ERROR_CHECK();
			}

			que.add(probeObj);
			DO_ERROR_CHECK();
			flush(que, cam, game->rend->framebuffer, game->rend, probeFlags);
		}

		probeFlags.mainShader
			= probeFlags.skinnedShader
			= probeFlags.instancedShader
			= irradShader;
		irradShader->bind();

		for (auto& entry : tempque.irradProbes) {
			probeObj->setTransform((TRS) {
				.position = entry.center,
				.scale    = glm::vec3(0.5),
			});

			for (unsigned i = 0; i < 6; i++) {
				std::string loc = "cubeface["+std::to_string(i)+"]";
				glm::vec3 facevec =
					game->rend->atlases.irradiance->tex_vector(entry.data->faces[i]);
				irradShader->set(loc, facevec);
			}

			que.add(probeObj);
			flush(que, cam, game->rend->framebuffer, game->rend, probeFlags);
		}
	}

	if (showLights) {
		renderFlags unshadedFlags = game->rend->getLightingFlags("unshaded");

		for (auto& entry : tempque.lights) {
			glm::vec3 pos = entry.center;

			if (cam->sphereInFrustum(pos, 0.5)) {
				probeObj->setTransform((TRS) {
					.position = entry.center,
					.scale    = glm::vec3(0.5),
				});

				que.add(probeObj);
				flush(que, cam, game->rend->framebuffer, game->rend, unshadedFlags);
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

	io.Fonts->AddFontFromFileTTF(GR_PREFIX "assets/fonts/Roboto-Regular.ttf",
	                             12*game->settings.UIScale);

	ImGui::StyleColorsDark();
	//ImGui::StyleColorsClassic();
	//ImGui::StyleColorsLight();
	ImGui_ImplSDL2_InitForOpenGL(game->ctx.window, game->ctx.glcontext);
	ImGui_ImplOpenGL3_Init("#version " GLSL_STRING);
}

void gameEditor::addEditorCallback(editCallback func) {
	callbacks.push_back(func);
}

void gameEditor::runCallbacks(sceneNode::ptr node, editAction action) {
	for (auto& f : callbacks) {
		f(node, action);
	}
}

result<objectPair> grendx::loadModel(std::string path) noexcept {
	std::string ext = filename_extension(path);
	if (ext == ".obj") {
		sceneModel::ptr m = load_object(path);

		// add the model at 0,0
		auto obj = sceneNode::ptr(new sceneNode());
		// make up a name for .obj models
		auto fname = basenameStr(path) + ":model";

		setNode(fname, obj, m);
		modelMap models = {{ fname, m }};
		return objectPair {obj, models};
	}

	else if (ext == ".gltf" || ext == ".glb") {
		modelMap mods = load_gltf_models(path);
		auto obj = std::make_shared<sceneNode>();

		for (auto& [name, model] : mods) {
			// add the models at 0,0
			setNode(name, obj, model);
		}

		return objectPair {obj, mods};
	}

	return {resultError, "loadModel: unknown file extension: " + ext};
}

//std::pair<sceneImport::ptr, modelMap>
result<importPair> grendx::loadSceneData(std::string path) noexcept {
	std::string ext = filename_extension(path);

	if (ext == ".gltf" || ext == ".glb") {
		std::cerr << "load_scene(): loading scene: " << path << std::endl;
		// TODO: this is kind of redundant now, unless I want this to also
		//       be able to load .map files from here... could be useful
		return load_gltf_scene(path);

	} else if (ext == ".map") {
		std::cerr << "load_scene(): loading map: " << path << std::endl;
		// TODO: need to detect and avoid recursive map loads,
		//       otherwise this will loop and consume all memory
		return loadMapData(path);
	}

	return {resultError, "loadSceneData: unknown file extension: " + ext};
}

result<sceneImport::ptr> grendx::loadSceneCompiled(std::string path) noexcept {
	if (auto res = loadSceneData(path)) {
		auto [obj, models] = *res;
		compileModels(models);
		return obj;

	} else {
		return {resultError, res.what()};
	}
}

// TODO: return result type here somehow
std::pair<sceneImport::ptr, std::future<bool>>
grendx::loadSceneAsyncCompiled(gameMain *game, std::string path) {
	auto ret = std::make_shared<sceneImport>(path);

	auto fut = game->jobs->addAsync([=] () {
		if (auto res = loadSceneData(path)) {
			auto [obj, models] = *res;

			// apparently you can't (officially) capture destructured bindings, only variables...
			// ffs
			sceneImport::ptr objptr = obj;
			modelMap modelptr = models;

			setNode("asyncData", ret, objptr);

			// TODO: hmm, seems there's no way to wait on compilation in the system I've
			//       set up here... compileModels() neeeds to be run on the main thread,
			//       if this function waits on it, then something from the main thread
			//       waits for this function to complete, that would result in a deadlock
			//
			//       this means it would be possible to get to the rendering step with a valid
			//       model that just hasn't been compiled yet... hmmmmmmmmmmmmmmm
			game->jobs->addDeferred([=] () {
				compileModels(modelptr);
				//setNode("asyncLoaded", ret, std::make_shared<sceneNode>());
				return true;
			});

			return true;

		} else {
			return false;
		}
	});

	return {ret, std::move(fut)};
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

void gameEditor::update(gameMain *game, float delta) {
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

		if (selectedNode->type == sceneNode::objType::ReflectionProbe) {
			auto bbox = UIObjects->getNode("Bounding-Box");
			auto probe = std::dynamic_pointer_cast<sceneReflectionProbe>(selectedNode);

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
}


void gameEditor::showLoadingScreen(gameMain *game) {
	// TODO: maybe not this
	loading_thing->draw(loading_img, nullptr);
	SDL_GL_SwapWindow(game->ctx.window);
}

bool gameEditor::isUIObject(sceneNode::ptr obj) {
	for (sceneNode::ptr temp = obj; temp; temp = temp->parent.lock()) {
		if (temp == UIObjects) {
			return true;
		}
	}

	return false;
}

sceneNode::ptr gameEditor::getNonModel(sceneNode::ptr obj) {
	for (sceneNode::ptr temp = obj; temp; temp = temp->parent.lock()) {
		if (temp->type != sceneNode::objType::Mesh
		    && temp->type != sceneNode::objType::Model)
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
	selectedNode = game->state->rootnode = sceneNode::ptr(new sceneNode());
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

#if 0
// messing around with an IDE-style layout, will set this up properly eventually
#include <imgui/imgui_internal.h>
static void initDocking(void) {
	ImGuiIO& io = ImGui::GetIO();
	ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_PassthruCentralNode;
	ImGuiWindowFlags window_flags
		= ImGuiWindowFlags_NoTitleBar
		| ImGuiWindowFlags_NoCollapse
		| ImGuiWindowFlags_NoResize
		| ImGuiWindowFlags_NoMove
		| ImGuiWindowFlags_NoBringToFrontOnFocus
		| ImGuiWindowFlags_NoNavFocus
		| ImGuiWindowFlags_NoBackground
		;

	if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable) {
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::Begin("dockspace", nullptr, window_flags);
		ImGui::PopStyleVar();

		ImGuiID dockspace_id = ImGui::GetID("dockspace");
		ImGui::DockSpace(dockspace_id, ImVec2(1280, 720), dockspace_flags);

		static bool initialized = false;
		if (!initialized) {
			initialized = true;

			ImGui::DockBuilderRemoveNode(dockspace_id);
			ImGui::DockBuilderAddNode(dockspace_id, dockspace_flags | ImGuiDockNodeFlags_DockSpace);
			ImGui::DockBuilderSetNodeSize(dockspace_id, ImVec2(1280, 720));

			auto dock_id_left = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Left, 0.2, nullptr, &dockspace_id);
			auto dock_id_down = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Down, 0.2, nullptr, &dockspace_id);

			ImGui::DockBuilderDockWindow("Down", dock_id_down);
			ImGui::DockBuilderDockWindow("Left", dock_id_left);
			ImGui::DockBuilderFinish(dockspace_id);
		}

		ImGui::Begin("Left");
		ImGui::Text("left");
		ImGui::End();

		ImGui::Begin("Down");
		ImGui::Text("down");
		ImGui::End();

		ImGui::End();
	}
}
#endif

void gameEditor::renderEditor(gameMain *game) {
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplSDL2_NewFrame(game->ctx.window);
	ImGui::NewFrame();

	// initDocking();
	menubar(game);

	// TODO: this could probably be reduced to like a map of
	//       window names to states...
	//       as simple as name -> opened?
	//       or name -> pair<opened, draw()>, something like that
	//       or even name -> draw(), and being in the map implies it's opened...
	if (showMetricsWindow)      metricsWindow(game);
		//ImGui::ShowMetricsWindow();
	if (showMapWindow)          mapWindow(game);
	if (showObjectSelectWindow) objectSelectWindow(game);
	if (showEntitySelectWindow) entitySelectWindow(game);
	if (showAddEntityWindow)    addEntityWindow(game);
	if (showProfilerWindow)     profilerWindow(game);
	if (showSettingsWindow)     settingsWindow(game);

	if (selectedNode && showObjectEditorWindow)
		objectEditorWindow(game);

	/* TODO:
	if (showEntityEditorWindow) {
		// TODO
		//entityEditorWindow(game);
	}
	*/
}
