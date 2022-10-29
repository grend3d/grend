#include <grend-config.h>

#include <grend/gameEditor.hpp>
#include <grend/gameState.hpp>
#include <grend/engine.hpp>
#include <grend/glManager.hpp>
#include <grend/geometryGeneration.hpp>
#include <grend/loadScene.hpp>
#include <grend/scancodes.hpp>
#include <grend/interpolation.hpp>
#include <grend/renderUtils.hpp>
#include <grend/jobQueue.hpp>

#include <grend/ecs/ecs.hpp>
#include <grend/ecs/shader.hpp>
#include <grend/ecs/sceneComponent.hpp>

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_sdl.h>
#include <imgui/backends/imgui_impl_opengl3.h>

using namespace grendx;

static sceneModel::ptr  physmodel;

void StyleColorsGrendDark(ImGuiStyle* dst = nullptr);

gameEditor::gameEditor(gameMain *game)
	: gameView()
{
	auto rend = game->services.resolve<renderContext>();

	LogCallback([this] (LogType type, const std::string& msg) {
		this->logEntries.push_back(msg);
	});

	objects = sceneNode::ptr(new sceneNode());
	cam->setFar(1000.0);

	// don't apply post-processing filters if this is an embedded profile
	// (so no tonemapping/HDR)
#ifdef NO_FLOATING_FB
	post = renderPostChain::ptr(new renderPostChain(
		{loadPostShader(GR_PREFIX "shaders/baked/texpresent.frag",
		                rend->globalShaderOptions)},
		game->settings.targetResX, game->settings.targetResY));
#else
	post = renderPostChain::ptr(new renderPostChain(
		{
#if 0
		loadPostShader(GR_PREFIX "shaders/baked/deferred-metal-roughness-pbr.frag",
		                rend->globalShaderOptions),
#endif
		loadPostShader(GR_PREFIX "shaders/baked/tonemap.frag",
		                rend->globalShaderOptions)
		},
		game->settings.targetResX, game->settings.targetResY));
#endif

	loading_thing = makePostprocessor<rOutput>(
		loadProgram(GR_PREFIX "shaders/baked/postprocess.vert",
		            GR_PREFIX "shaders/baked/texpresent.frag",
		            rend->globalShaderOptions),
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

void gameEditor::render(gameMain *game, renderFramebuffer::ptr fb) {
#if !defined(__ANDROID__) && GLSL_VERSION > 100
	renderEditor(game);
	ImGui::Render();
#endif

	auto rend  = game->services.resolve<renderContext>();
	auto state = game->services.resolve<gameState>();

	renderQueue que;
	renderFlags flags = rend->getLightingFlags();
	//flags.features |= renderFlags::Features::StencilTest;

	// TODO: avoid clearing then reallocating all of this state...
	//       lots of allocations
	clickState.clear();
	auto world = buildClickableQueue(game, clickState);
	world.add(flags, state->rootnode);
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

	renderFlags unshadedFlags = rend->getLightingFlags("unshaded");
	renderFlags constantFlags = rend->getLightingFlags("constant-color");

	renderOptions frontOpts, backOpts;
	frontOpts.features |=  renderOptions::Features::DepthTest;
	frontOpts.features |=  renderOptions::Features::StencilTest;
	frontOpts.features &= ~renderOptions::Features::DepthMask;
	backOpts.features = frontOpts.features;

	frontOpts.depthFunc = renderOptions::Less;
	backOpts.depthFunc  = renderOptions::GreaterEqual;

	// TODO: should have uniform setting flags that pass values
	//       to the underlying shaders...
	renderQueue por = que;

	for (auto& var : constantFlags.variants) {
		for (auto& shader : var.shaders) {
			shader->bind();
			shader->set("outputColor", glm::vec4(0.1, 0.1, 0.1, 1.0));
		}
	}

	flush(por, cam, rend->framebuffer, rend, constantFlags, backOpts);

	por = que;
	flush(por, cam, rend->framebuffer, rend, unshadedFlags, frontOpts);

	// TODO: function to do this
	int winsize_x, winsize_y;
	SDL_GetWindowSize(game->ctx.window, &winsize_x, &winsize_y);

	setPostUniforms(post, game, cam);
	post->draw(rend->framebuffer);

// XXX: FIXME: imgui on es2 results in a blank screen, for whatever reason
//             the postprocessing shader doesn't see anything from the
//             render framebuffer, although the depth/stencil buffer seems
//             to be there...
//
//             also, now that I'm trying things on android, seems
//             the phone I'm testing on has a driver bug triggered
//             by one of the imgui draw calls, but only on es3... boy oh boy
#if !defined(__ANDROID__) && GLSL_VERSION > 100
	renderImguiData(game);
#endif
}

void gameEditor::renderWorldObjects(gameMain *game) {
	auto rend  = game->services.resolve<renderContext>();
	auto state = game->services.resolve<gameState>();

	DO_ERROR_CHECK();

	// XXX: wasteful, a bit wrong
	static sceneNode::ptr probeObj = std::make_shared<sceneNode>();
	setNode("model", probeObj, physmodel);

	renderQueue tempque;
	renderQueue que;
	tempque.add(state->rootnode);

	if (showProbes) {
		renderFlags probeFlags;
		auto refShader = rend->internalShaders["refprobe_debug"];
		auto irradShader = rend->internalShaders["irradprobe_debug"];

		// TODO: function in renderFlags that sets every shader
		for (auto& v : probeFlags.variants) {
			for (auto& p : v.shaders) {
				p = refShader;
			}
		}

		refShader->bind();

		for (auto& entry : tempque.probes) {
			probeObj->setTransform((TRS) {
				.position = entry.center,
				.scale    = glm::vec3(0.5),
			});

			for (unsigned i = 0; i < 6; i++) {
				std::string loc = "cubeface["+std::to_string(i)+"]";
				glm::vec3 facevec =
					rend->atlases.reflections->tex_vector(entry.data->faces[0][i]);
				refShader->set(loc, facevec);
				DO_ERROR_CHECK();
			}

			que.add(probeObj);
			DO_ERROR_CHECK();
			flush(que, cam, rend->framebuffer, rend, probeFlags, renderOptions());
		}

		for (auto& v : probeFlags.variants) {
			for (auto& p : v.shaders) {
				p = irradShader;
			}
		}

		irradShader->bind();

		for (auto& entry : tempque.irradProbes) {
			probeObj->setTransform((TRS) {
				.position = entry.center,
				.scale    = glm::vec3(0.5),
			});

			for (unsigned i = 0; i < 6; i++) {
				std::string loc = "cubeface["+std::to_string(i)+"]";
				glm::vec3 facevec =
					rend->atlases.irradiance->tex_vector(entry.data->faces[i]);
				irradShader->set(loc, facevec);
			}

			que.add(probeObj);
			DO_ERROR_CHECK();
			flush(que, cam, rend->framebuffer, rend, probeFlags, renderOptions());
		}
	}

	if (showLights) {
		renderFlags unshadedFlags = rend->getLightingFlags("unshaded");

		for (auto& entry : tempque.lights) {
			glm::vec3 pos = entry.center;

			if (cam->sphereInFrustum(BSphere {pos, 0.5})) {
				probeObj->setTransform((TRS) {
					.position = entry.center,
					.scale    = glm::vec3(0.5),
				});

				que.add(probeObj);
				flush(que, cam, rend->framebuffer,
				      rend, unshadedFlags, renderOptions());
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

	StyleColorsGrendDark();
	//ImGui::StyleColorsDark();
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
		LogInfo("load_scene(): loading scene: ");
		// TODO: this is kind of redundant now, unless I want this to also
		//       be able to load .map files from here... could be useful
		return load_gltf_scene(path);

	} else if (ext == ".map") {
		LogInfo("load_scene(): loading map: ");
		// TODO: need to detect and avoid recursive map loads,
		//       otherwise this will loop and consume all memory
		return loadMapData(path);

	} else if (ext == ".obj") {
		LogInfo("load_scene(): loading object");
		sceneModel::ptr m = load_object(path);

		// add the model at 0,0
		auto obj = sceneImport::ptr(new sceneImport(path));
		// make up a name for .obj models
		auto fname = basenameStr(path) + ":model";

		setNode(fname, obj, m);
		modelMap models = {{ fname, m }};
		return importPair {obj, models};
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
	auto jobs = game->services.resolve<jobQueue>();

	auto fut = jobs->addAsync([=] () {
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
			jobs->addDeferred([=] () {
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
	// TODO: redo this (again (sigh))
#if 0
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
			LogErrorFmt(">> couldn't reload shader: ", name);
		}
	}
#endif
}

void gameEditor::setMode(enum mode newmode) {
	mode = newmode;
	inputBinds.setMode(mode);
}

void gameEditor::handleCursorUpdate(gameMain *game) {
	cursorBuf.position = cam->direction()*editDistance + cam->position();
	align(cursorBuf.position, snapAmount * snapEnabled);
}

void gameEditor::update(gameMain *game, float delta) {
	auto state = game->services.resolve<gameState>();

	static glm::vec3 target = glm::vec3(0);
	glm::mat3 m = {cam->right(), cam->up(), cam->direction()};
	glm::vec3 vpos = m*(cam->velocity());

	target = interp::average(vpos, target, 8.f, delta);
	cam->setPosition(cam->position() + target*delta);

	auto orientation = UIObjects->getNode("Orientation-Indicator");
	auto cursor      = UIObjects->getNode("Cursor-Placement");
	assert(orientation && cursor);

	orientation->visible =
		   selectedNode != nullptr
		&& !selectedNode->parent.expired()
		&& selectedNode != state->rootnode;

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
			glm::mat4 localToWorld = fullTranslation(selectedNode);
			newtrans.position = applyTransform(localToWorld);

			// project position relative to camera onto camera direction vector
			// to get depth, then scale with depth to keep size constant on screen
			glm::vec3 u = newtrans.position - cam->position();
			glm::vec3 v = cam->direction();

			float t = glm::dot(u, v) / glm::dot(v, v);
			newtrans.scale = glm::vec3(t*0.20f);
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
	auto state = game->services.resolve<gameState>();

	showLoadingScreen(game);

	cam->setPosition({0, 0, 0});
	models.clear();

	// TODO: clear() for state
	selectedNode = state->rootnode = sceneNode::ptr(new sceneNode());
}

// TODO: rename 'renderer' to 'rend' or something
void gameEditor::renderImguiData(gameMain *game) {
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

#if 1
// messing around with an IDE-style layout, will set this up properly eventually
#include <imgui/imgui_internal.h>
static void initDocking(gameMain *game) {
	ImGuiIO& io = ImGui::GetIO();
	//ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;
	ImGuiDockNodeFlags dockspace_flags
		= ImGuiDockNodeFlags_PassthruCentralNode
		| ImGuiDockNodeFlags_NoDockingInCentralNode
		;

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
		const ImGuiViewport *viewport = ImGui::GetMainViewport();

		ImGui::SetNextWindowPos(viewport->WorkPos);
		ImGui::SetNextWindowSize(viewport->WorkSize);
		ImGui::SetNextWindowViewport(viewport->ID);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
		ImGui::Begin("dockspace", nullptr, window_flags);
		ImGui::PopStyleVar();
		ImGui::PopStyleVar();
		ImGui::PopStyleVar();

		ImGuiID dockspace_id = ImGui::GetID("MyDockspace");
		ImGui::DockSpace(dockspace_id, ImVec2(0.f, 0.f), dockspace_flags);

		static bool initialized = false;
		if (!initialized) {
			initialized = true;

			ImGui::DockBuilderRemoveNode(dockspace_id);
			ImGui::DockBuilderAddNode(dockspace_id, dockspace_flags | ImGuiDockNodeFlags_DockSpace);
			ImGui::DockBuilderSetNodeSize(dockspace_id, ImVec2(1920, 1080));

			auto dock_id_left      = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Left, 0.2, nullptr, &dockspace_id);
			auto dock_id_right     = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Right, 0.25, nullptr, &dockspace_id);
			auto dock_id_down      = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Down, 0.3, nullptr, &dockspace_id);
			auto dock_id_leftbota  = ImGui::DockBuilderSplitNode(dock_id_left, ImGuiDir_Down, 0.5, nullptr, &dock_id_left);
			auto dock_id_leftbotb  = ImGui::DockBuilderSplitNode(dock_id_leftbota, ImGuiDir_Down, 0.5, nullptr, &dock_id_leftbota);

			ImGui::DockBuilderDockWindow("Down",  dock_id_down);
			ImGui::DockBuilderDockWindow("Left",  dock_id_left);
			ImGui::DockBuilderDockWindow("Right", dock_id_right);

			ImGui::DockBuilderDockWindow("Files",         dock_id_left);
			ImGui::DockBuilderDockWindow("Objects",       dock_id_left);
			ImGui::DockBuilderDockWindow("Entities",      dock_id_right);
			ImGui::DockBuilderDockWindow("Entity Editor", dock_id_right);
			ImGui::DockBuilderDockWindow("Engine log",    dock_id_down);

			ImGui::DockBuilderDockWindow("Object editor",  dock_id_leftbota);
			ImGui::DockBuilderDockWindow("Engine metrics", dock_id_leftbotb);


			ImGui::DockBuilderFinish(dockspace_id);

			ImVec2 vMin = ImGui::GetWindowContentRegionMin();
			ImVec2 vMax = ImGui::GetWindowContentRegionMax();

			ImGui::GetForegroundDrawList()->AddRect(vMin, vMax, IM_COL32(255, 255, 0, 255));

		}

		/*
		ImGui::Begin("Left");
		ImGui::Text("left");
		ImGui::End();

		ImGui::Begin("Down");
		ImGui::Text("down");
		ImGui::End();
		*/

		auto rend = game->services.resolve<renderContext>();
		auto dock_main = ImGui::DockBuilderGetCentralNode(dockspace_id);
		auto& pos  = dock_main->Pos;
		auto& size = dock_main->Size;
		ImVec2 end = {pos.x + size.x, pos.y + size.y};
		ImGui::GetForegroundDrawList()->AddRect(pos, end, IM_COL32(255, 255, 0, 255));

		rend->setViewport({pos.x, pos.y}, {size.x, size.y});

		ImGui::End();
	}
}
#endif

void gameEditor::renderEditor(gameMain *game) {
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplSDL2_NewFrame(game->ctx.window);
	ImGui::NewFrame();

	initDocking(game);
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
	if (showAddEntityWindow)    addEntityWindow(game);
	if (showProfilerWindow)     profilerWindow(game);
	if (showSettingsWindow)     settingsWindow(game);

	if (showEntityEditorWindow) entityEditorWindow(game);
	if (showEntityListWindow)   entityListWindow(game);
	if (showLogWindow)          logWindow(game);
	if (showFilePane)           filepane.render(game);

	if (selectedNode && showObjectEditorWindow)
		objectEditorWindow(game);

	/* TODO:
	if (showEntityEditorWindow) {
		// TODO
		//entityEditorWindow(game);
	}
	*/
}

void StyleColorsGrendDark(ImGuiStyle* dst)
{
	using namespace ImGui;
    ImGuiStyle* style = dst ? dst : &ImGui::GetStyle();
    ImVec4* colors = style->Colors;

    colors[ImGuiCol_Text]                   = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
    colors[ImGuiCol_TextDisabled]           = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
    colors[ImGuiCol_WindowBg]               = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
    colors[ImGuiCol_ChildBg]                = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_PopupBg]                = ImVec4(0.11f, 0.11f, 0.14f, 0.92f);
    colors[ImGuiCol_Border]                 = ImVec4(0.50f, 0.50f, 0.50f, 0.50f);
    colors[ImGuiCol_BorderShadow]           = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg]                = ImVec4(0.43f, 0.43f, 0.43f, 0.39f);
    colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.47f, 0.47f, 0.69f, 0.40f);
    colors[ImGuiCol_FrameBgActive]          = ImVec4(0.42f, 0.41f, 0.64f, 0.69f);
    colors[ImGuiCol_TitleBg]                = ImVec4(0.33f, 0.60f, 0.30f, 0.83f);
    colors[ImGuiCol_TitleBgActive]          = ImVec4(0.32f, 0.32f, 0.63f, 0.87f);
    colors[ImGuiCol_TitleBgCollapsed]       = ImVec4(0.40f, 0.40f, 0.80f, 0.20f);
    colors[ImGuiCol_MenuBarBg]              = ImVec4(0.40f, 0.40f, 0.55f, 0.80f);
    colors[ImGuiCol_ScrollbarBg]            = ImVec4(0.20f, 0.25f, 0.30f, 0.60f);
    colors[ImGuiCol_ScrollbarGrab]          = ImVec4(0.40f, 0.40f, 0.80f, 0.30f);
    colors[ImGuiCol_ScrollbarGrabHovered]   = ImVec4(0.40f, 0.40f, 0.80f, 0.40f);
    colors[ImGuiCol_ScrollbarGrabActive]    = ImVec4(0.41f, 0.39f, 0.80f, 0.60f);
    colors[ImGuiCol_CheckMark]              = ImVec4(0.90f, 0.90f, 0.90f, 0.50f);
    colors[ImGuiCol_SliderGrab]             = ImVec4(1.00f, 1.00f, 1.00f, 0.30f);
    colors[ImGuiCol_SliderGrabActive]       = ImVec4(0.41f, 0.39f, 0.80f, 0.60f);
    colors[ImGuiCol_Button]                 = ImVec4(0.35f, 0.40f, 0.61f, 0.62f);
    colors[ImGuiCol_ButtonHovered]          = ImVec4(0.40f, 0.48f, 0.71f, 0.79f);
    colors[ImGuiCol_ButtonActive]           = ImVec4(0.46f, 0.54f, 0.80f, 1.00f);
    colors[ImGuiCol_Header]                 = ImVec4(0.40f, 0.40f, 0.90f, 0.45f);
    colors[ImGuiCol_HeaderHovered]          = ImVec4(0.45f, 0.45f, 0.90f, 0.80f);
    colors[ImGuiCol_HeaderActive]           = ImVec4(0.53f, 0.53f, 0.87f, 0.80f);
    colors[ImGuiCol_Separator]              = ImVec4(0.50f, 0.50f, 0.50f, 0.60f);
    colors[ImGuiCol_SeparatorHovered]       = ImVec4(0.60f, 0.60f, 0.70f, 1.00f);
    colors[ImGuiCol_SeparatorActive]        = ImVec4(0.70f, 0.70f, 0.90f, 1.00f);
    colors[ImGuiCol_ResizeGrip]             = ImVec4(1.00f, 1.00f, 1.00f, 0.10f);
    colors[ImGuiCol_ResizeGripHovered]      = ImVec4(0.78f, 0.82f, 1.00f, 0.60f);
    colors[ImGuiCol_ResizeGripActive]       = ImVec4(0.78f, 0.82f, 1.00f, 0.90f);
    colors[ImGuiCol_Tab]                    = ImLerp(colors[ImGuiCol_Header],       colors[ImGuiCol_TitleBgActive], 0.80f);
    colors[ImGuiCol_TabHovered]             = colors[ImGuiCol_HeaderHovered];
    colors[ImGuiCol_TabActive]              = ImLerp(colors[ImGuiCol_HeaderActive], colors[ImGuiCol_TitleBgActive], 0.60f);
    colors[ImGuiCol_TabUnfocused]           = ImLerp(colors[ImGuiCol_Tab],          colors[ImGuiCol_TitleBg], 0.80f);
    colors[ImGuiCol_TabUnfocusedActive]     = ImLerp(colors[ImGuiCol_TabActive],    colors[ImGuiCol_TitleBg], 0.40f);
    //colors[ImGuiCol_DockingPreview]         = colors[ImGuiCol_Header] * ImVec4(1.0f, 1.0f, 1.0f, 0.7f);
    colors[ImGuiCol_DockingPreview]         = ImVec4(1.0f, 1.0f, 1.0f, 0.3f);
    colors[ImGuiCol_DockingEmptyBg]         = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    colors[ImGuiCol_PlotLines]              = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered]       = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogram]          = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered]   = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
    colors[ImGuiCol_TableHeaderBg]          = ImVec4(0.27f, 0.27f, 0.38f, 1.00f);
    colors[ImGuiCol_TableBorderStrong]      = ImVec4(0.31f, 0.31f, 0.45f, 1.00f);   // Prefer using Alpha=1.0 here
    colors[ImGuiCol_TableBorderLight]       = ImVec4(0.26f, 0.26f, 0.28f, 1.00f);   // Prefer using Alpha=1.0 here
    colors[ImGuiCol_TableRowBg]             = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_TableRowBgAlt]          = ImVec4(1.00f, 1.00f, 1.00f, 0.07f);
    colors[ImGuiCol_TextSelectedBg]         = ImVec4(0.00f, 0.00f, 1.00f, 0.35f);
    colors[ImGuiCol_DragDropTarget]         = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
    colors[ImGuiCol_NavHighlight]           = colors[ImGuiCol_HeaderHovered];
    colors[ImGuiCol_NavWindowingHighlight]  = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg]      = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg]       = ImVec4(0.20f, 0.20f, 0.20f, 0.35f);
}

