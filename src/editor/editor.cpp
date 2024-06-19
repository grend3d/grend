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
#include <grend/gridDraw.hpp>

#include <grend/ecs/ecs.hpp>
#include <grend/ecs/shader.hpp>
#include <grend/ecs/sceneComponent.hpp>

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_sdl.h>
#include <imgui/backends/imgui_impl_opengl3.h>

using namespace grendx;
using namespace grendx::engine;

void StyleColorsGrendDark(ImGuiStyle* dst = nullptr);

gameEditor::gameEditor()
	: gameView()
{
	auto rend = Resolve<renderContext>();
	auto ctx  = Resolve<SDLContext>();
	auto ecs  = Resolve<ecs::entityManager>();

	LogCallback([this] (LogType type, const std::string& msg) {
		this->logEntries.push_back(msg);
	});

	objects = ecs->construct<sceneNode>();
	cam->setFar(1000.0);

	// don't apply post-processing filters if this is an embedded profile
	// (so no tonemapping/HDR)
#ifdef NO_FLOATING_FB
	post = renderPostChain::ptr(new renderPostChain(
		{loadPostShader(GR_PREFIX "shaders/baked/texpresent.frag",
		                rend->globalShaderOptions)},
		ctx->getSettings().targetResX, ctx->getSettings().targetResY));
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
		ctx->getSettings().targetResX, ctx->getSettings().targetResY));
#endif

	loading_thing = makePostprocessor<rOutput>(
		loadProgram(GR_PREFIX "shaders/baked/postprocess.vert",
		            GR_PREFIX "shaders/baked/texpresent.frag",
		            rend->globalShaderOptions),
		ctx->getSettings().targetResX, ctx->getSettings().targetResY
	);

	// XXX: constructing a full shared pointer for this is a bit wasteful...
	loading_img = genTexture();
	loading_img->buffer(std::make_shared<textureData>(GR_PREFIX "assets/tex/loading-splash.png"));

	clear();
	initImgui();
	loadUIModels();

	loadInputBindings();
	setMode(mode::View);
};

void gameEditor::loadUIModels(void) {
	auto ecs  = Resolve<ecs::entityManager>();

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
	UIModels["DebugSphere"] = load_object(dir + "../smoothsphere.obj");

	//UIObjects = sceneNode::ptr(new sceneNode());
	UIObjects = ecs->construct<sceneNode>();
	sceneNode::ptr xptr = ecs->construct<sceneNode>();
	sceneNode::ptr yptr = ecs->construct<sceneNode>();
	sceneNode::ptr zptr = ecs->construct<sceneNode>();
	sceneNode::ptr xrot = ecs->construct<sceneNode>();
	sceneNode::ptr yrot = ecs->construct<sceneNode>();
	sceneNode::ptr zrot = ecs->construct<sceneNode>();
	//sceneNode::ptr xptr = sceneNode::ptr(new clicker(this, mode::MoveX));
	//sceneNode::ptr yptr = sceneNode::ptr(new clicker(this, mode::MoveY));
	//sceneNode::ptr zptr = sceneNode::ptr(new clicker(this, mode::MoveZ));
	//sceneNode::ptr xrot = sceneNode::ptr(new clicker(this, mode::RotateX));
	//sceneNode::ptr yrot = sceneNode::ptr(new clicker(this, mode::RotateY));
	//sceneNode::ptr zrot = sceneNode::ptr(new clicker(this, mode::RotateZ));
	sceneNode::ptr orientation = ecs->construct<sceneNode>();
	sceneNode::ptr cursor      = ecs->construct<sceneNode>();
	sceneNode::ptr bbox        = ecs->construct<sceneNode>();

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

void gameEditor::render(renderFramebuffer::ptr fb) {
	auto rend  = Resolve<renderContext>();
	auto state = Resolve<gameState>();
	auto ctx   = Resolve<SDLContext>();
	auto phys  = Resolve<physics>();

	auto vportPos  = rend->getDrawOffset();
	auto vportSize = rend->getDrawSize();
	glViewport(vportPos.x, fb->height - (vportPos.y + vportSize.y), vportSize.x, vportSize.y);

	static gridDrawer grid;
	grid.buildGrid(cam->position());
	grid.flushLines(cam->viewProjTransform());

	// TODO: clean up messy renderQueue stuff here
	multiRenderQueue world;
	renderQueue que;
	renderFlags flags = rend->getLightingFlags();
	//flags.features |= renderFlags::Features::StencilTest;

	// TODO: avoid clearing then reallocating all of this state...
	//       lots of allocations
	clickState.clear();

	renderQueue tempque;
	buildClickableImports(clickState, state->rootnode, tempque);
	buildClickableQueue(clickState, world);
	world.add(flags, tempque);

	drawMultiQueue(world, fb, cam);
	renderWorldObjects();

	auto p = UIObjects->getNode("Orientation-Indicator");
	auto m = p->transform.getMatrix();

	if (getSelectedEntity()) {
		que.add(p->getNode("X-Axis"),     1, m);
		que.add(p->getNode("Y-Axis"),     2, m);
		que.add(p->getNode("Z-Axis"),     3, m);
		que.add(p->getNode("X-Rotation"), 4, m);
		que.add(p->getNode("Y-Rotation"), 5, m);
		que.add(p->getNode("Z-Rotation"), 6, m);
	}

	auto cursor = UIObjects->getNode("Cursor-Placement");
	auto bbox   = UIObjects->getNode("Bounding-Box");
	que.add(cursor, 0, m);
	que.add(bbox,   0, m);

	renderFlags unshadedFlags = rend->getLightingFlags("unshaded");
	renderFlags constantFlags = rend->getLightingFlags("constant-color");

	renderOptions frontOpts, backOpts;
	frontOpts.features |=  renderOptions::Features::DepthTest;
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

	rend->defaultSkybox->draw(cam, rend->framebuffer);

	// TODO: function to do this
	int winsize_x, winsize_y;
	SDL_GetWindowSize(ctx->window, &winsize_x, &winsize_y);

	setPostUniforms(post, cam);
	post->draw(rend->framebuffer);

	glViewport(vportPos.x, fb->height - (vportPos.y + vportSize.y), vportSize.x, vportSize.y);
	if (phys) {
		phys->drawDebug(cam->viewProjTransform());
	}
}

void gameEditor::renderWorldObjects() {
	auto rend  = Resolve<renderContext>();
	auto state = Resolve<gameState>();
	auto ecs   = Resolve<ecs::entityManager>();

	DO_ERROR_CHECK();

	sceneNode::ptr probeObj = UIModels["DebugSphere"];

	renderQueue tempque;
	renderQueue que;

	// TODO: Wait... this is now just building a full render queue to find just
	//       lights and probes in the scene for editing, it never draws meshes
	tempque.add(state->rootnode);

	if (showProbes) {
		renderFlags probeFlags;
		auto irradShader = rend->internalShaders["irradprobe_debug"];
		auto refShader   = rend->internalShaders["refprobe_debug"];

		// TODO: function in renderFlags that sets every shader
		for (auto& v : probeFlags.variants) {
			for (auto& p : v.shaders) {
				p = refShader;
			}
		}

		refShader->bind();

		for (auto& entry : tempque.probes) {
			probeObj->transform.set({
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
			probeObj->transform.set({
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
				probeObj->transform.set({
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

void gameEditor::initImgui() {
	auto ctx  = Resolve<SDLContext>();

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	//io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

	io.Fonts->AddFontFromFileTTF(GR_PREFIX "assets/fonts/Roboto-Regular.ttf",
	                             12*ctx->getSettings().UIScale);

	StyleColorsGrendDark();
	//ImGui::StyleColorsDark();
	//ImGui::StyleColorsClassic();
	//ImGui::StyleColorsLight();
	ImGui_ImplSDL2_InitForOpenGL(ctx->window, ctx->glcontext);
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
	auto ecs   = Resolve<ecs::entityManager>();

	std::string ext = filename_extension(path);
	if (ext == ".obj") {
		sceneModel::ptr m = load_object(path);

		// add the model at 0,0
		auto obj = ecs->construct<sceneNode>();
		// make up a name for .obj models
		auto fname = basenameStr(path) + ":model";

		setNode(fname, obj, m);
		modelMap models = {{ fname, m }};
		return objectPair {obj, models};
	}

	else if (ext == ".gltf" || ext == ".glb") {
		modelMap mods = load_gltf_models(path);
		auto obj = ecs->construct<sceneNode>();

		for (auto& [name, model] : mods) {
			// add the models at 0,0
			setNode(name, obj, model);
		}

		return objectPair {obj, mods};
	}

	return {resultError, "loadModel: unknown file extension: " + ext};
}

result<objectPair> grendx::loadSceneData(std::string path) noexcept {
	auto ecs = Resolve<ecs::entityManager>();

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
		// TODO: add source file link here
		auto obj = ecs->construct<sceneNode>();
		// make up a name for .obj models
		auto fname = basenameStr(path) + ":model";

		setNode(fname, obj, m);
		modelMap models = {{ fname, m }};
		return objectPair {obj, models};
	}

	return {resultError, std::format("loadSceneData: unknown file extension for {}: '{}'", path, ext)};
}

result<sceneNode::ptr> grendx::loadSceneCompiled(std::string path) noexcept {
	if (auto res = loadSceneData(path)) {
		auto [obj, models] = *res;
		compileModels(models);
		return obj;

	} else {
		return {resultError, res.what()};
	}
}

// TODO: return result type here somehow
std::pair<sceneNode::ptr, std::future<bool>>
grendx::loadSceneAsyncCompiled(std::string path) {
	auto jobs = Resolve<jobQueue>();
	auto ecs  = Resolve<ecs::entityManager>();

	auto ret = ecs->construct<sceneNode>();
	// TODO: add source file link here

	auto fut = jobs->addAsync([=] () {
		if (auto res = loadSceneData(path)) {
			auto [obj, models] = *res;

			// apparently you can't (officially) capture destructured bindings, only variables...
			// ffs
			sceneNode::ptr objptr = obj;
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

void gameEditor::reloadShaders() {
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

void gameEditor::handleCursorUpdate() {
	cursorBuf.position = cam->direction()*editDistance + cam->position();
	align(cursorBuf.position, snapAmount * snapEnabled);
}

void gameEditor::update(float delta) {
	auto state = Resolve<gameState>();

	static glm::vec3 target = glm::vec3(0);
	glm::mat3 m = {cam->right(), cam->up(), cam->direction()};
	glm::vec3 vpos = m*(cam->velocity());

	target = interp::average(vpos, target, 8.f, delta);
	cam->setPosition(cam->position() + target*delta);

	auto orientation = UIObjects->getNode("Orientation-Indicator");
	auto cursor      = UIObjects->getNode("Cursor-Placement");
	assert(orientation && cursor);

	auto selectedNode = getSelectedNode();
	auto selectedEnt  = getSelectedEntity();

	orientation->visible =
		selectedEnt || (selectedNode
		                && selectedNode->parent
		                && selectedNode != state->rootnode);

	cursor->visible =
		   mode == mode::AddObject
		|| mode == mode::AddPointLight
		|| mode == mode::AddSpotLight
		|| mode == mode::AddDirectionalLight
		|| mode == mode::AddReflectionProbe
		|| mode == mode::AddIrradianceProbe
		;

	if (selectedEnt) {
		for (auto& str : {"X-Axis", "Y-Axis", "Z-Axis",
		                  "X-Rotation", "Y-Rotation", "Z-Rotation"})
		{
			auto ptr = orientation->getNode(str);

			TRS newtrans = selectedEnt->transform.getTRS();
			glm::mat4 localToWorld = selectedNode
				? fullTranslation(selectedNode)
				: selectedEnt->transform.getMatrix();

			newtrans.position = extractTranslation(localToWorld);

			// project position relative to camera onto camera direction vector
			// to get depth, then scale with depth to keep size constant on screen
			glm::vec3 u = newtrans.position - cam->position();
			glm::vec3 v = cam->direction();

			float t = glm::dot(u, v) / glm::dot(v, v);
			newtrans.scale = glm::vec3(t*0.20f);
			ptr->transform.set(newtrans);
		}

		if (auto probe = dynamic_ref_cast<sceneReflectionProbe>(selectedEnt)) {
			auto bbox = UIObjects->getNode("Bounding-Box");

			TRS transform = probe->transform.getTRS();
			glm::vec3 bmin = transform.position + probe->boundingBox.min;
			glm::vec3 bmax = transform.position + probe->boundingBox.max;
			glm::vec3 center = 0.5f*(bmax + bmin);
			glm::vec3 extent = (bmax - bmin);

			assert(bbox != nullptr);
			bbox->visible = true;
			bbox->transform.set({
				.position = center,
				.scale    = extent,
			});

		} else {
			auto bbox = UIObjects->getNode("Bounding-Box");
			assert(bbox != nullptr);
			bbox->visible = false;
		}

		handleMoveRotate();
	}


	{
		auto ptr = UIObjects->getNode("Cursor-Placement");

		if (ptr) {
			handleCursorUpdate();
			ptr->transform.set({ .position = cursorBuf.position, });
		}
	}
}


void gameEditor::showLoadingScreen() {
	auto ctx  = Resolve<SDLContext>();

	// TODO: maybe not this
	loading_thing->draw(loading_img, nullptr);
	SDL_GL_SwapWindow(ctx->window);
}

bool gameEditor::isUIObject(sceneNode::ptr obj) {
	for (sceneNode::ptr temp = obj; temp; temp = temp->parent) {
		if (temp == UIObjects) {
			return true;
		}
	}

	return false;
}

sceneNode::ptr gameEditor::getNonModel(sceneNode::ptr obj) {
	for (sceneNode::ptr temp = obj; temp; temp = temp->parent) {
		if (temp->type != sceneNode::objType::Mesh
		    && temp->type != sceneNode::objType::Model)
		{
			return temp;
		}
	}

	return nullptr;
}

void gameEditor::clear() {
	auto state = Resolve<gameState>();
	auto ecs   = Resolve<ecs::entityManager>();

	showLoadingScreen();

	cam->setPosition({0, 0, 0});
	models.clear();

	// TODO: clear() for state
	state->rootnode = ecs->construct<sceneNode>();
	setSelectedEntity(state->rootnode);
}
