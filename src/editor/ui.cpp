#include <grend/gameEditor.hpp>

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

gameEditorUI::gameEditorUI() {
	// XXX
	showFilePane           = true;
	showLogWindow          = true;
	showMetricsWindow      = true;
	showEntityEditorWindow = true;
	showMapWindow          = true;
}

void gameEditorUI::handleEvent(const SDL_Event& ev) {
	ImGui_ImplSDL2_ProcessEvent(&ev);
	currentView->handleEvent(ev);
}

void gameEditorUI::render(renderFramebuffer::ptr fb) {
#if !defined(__ANDROID__) && GLSL_VERSION > 100
	renderEditor();
	ImGui::Render();
#endif

	currentView->render(fb);

// XXX: FIXME: imgui on es2 results in a blank screen, for whatever reason
//             the postprocessing shader doesn't see anything from the
//             render framebuffer, although the depth/stencil buffer seems
//             to be there...
//
//             also, now that I'm trying things on android, seems
//             the phone I'm testing on has a driver bug triggered
//             by one of the imgui draw calls, but only on es3... boy oh boy
#if !defined(__ANDROID__) && GLSL_VERSION > 100
	renderImguiData();
#endif
}

void gameEditorUI::update(float delta) {
	currentView->update(delta);
}

void gameEditorUI::renderImguiData() {
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
static void initDocking() {
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

			auto dock_id_left  = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Left, 0.25,
			                                                 nullptr, &dockspace_id);
			auto dock_id_right = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Right, 0.33,
			                                                 nullptr, &dockspace_id);
			auto dock_id_down  = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Down, 0.25,
			                                                 nullptr, &dockspace_id);

			ImGuiID dock_id_leftbota;
			ImGuiID dock_id_leftbotb;

			ImGuiID dock_id_rightbota;
			ImGuiID dock_id_rightbotb;

			ImGui::DockBuilderSplitNode(dock_id_left,  ImGuiDir_Down, 0.25, &dock_id_leftbota,  &dock_id_leftbotb);
			ImGui::DockBuilderSplitNode(dock_id_right, ImGuiDir_Down, 0.25, &dock_id_rightbota, &dock_id_rightbotb);

			ImGui::DockBuilderDockWindow("Files",          dock_id_leftbotb);
			ImGui::DockBuilderDockWindow("Engine metrics", dock_id_leftbota);

			ImGui::DockBuilderDockWindow("Objects",       dock_id_rightbotb);
			ImGui::DockBuilderDockWindow("Entities",      dock_id_rightbotb);
			ImGui::DockBuilderDockWindow("Entity Editor", dock_id_rightbota);
			ImGui::DockBuilderDockWindow("Engine log",    dock_id_down);

			ImGui::DockBuilderFinish(dockspace_id);

			ImVec2 vMin = ImGui::GetWindowContentRegionMin();
			ImVec2 vMax = ImGui::GetWindowContentRegionMax();
		}

		auto rend = Resolve<renderContext>();

		auto dock_main = ImGui::DockBuilderGetCentralNode(dockspace_id);
		auto& pos  = dock_main->Pos;
		auto& size = dock_main->Size;
		ImVec2 end = {pos.x + size.x, pos.y + size.y};

		rend->setViewport({pos.x, pos.y}, {size.x, size.y});

		ImGui::End();
	}
}
#endif

void gameEditorUI::renderEditor() {
	auto ctx  = Resolve<SDLContext>();

	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplSDL2_NewFrame(ctx->window);
	ImGui::NewFrame();


	initDocking();

	menubar();

	// TODO: this could probably be reduced to like a map of
	//       window names to states...
	//       as simple as name -> opened?
	//       or name -> pair<opened, draw()>, something like that
	//       or even name -> draw(), and being in the map implies it's opened...
	if (showMetricsWindow)      metricsWindow();
		//ImGui::ShowMetricsWindow();
	if (showMapWindow)          mapWindow();
	if (showObjectSelectWindow) objectSelectWindow();
	if (showAddEntityWindow)    addEntityWindow();
	if (showProfilerWindow)     profilerWindow();
	if (showSettingsWindow)     settingsWindow();

	if (showEntityEditorWindow) entityEditorWindow();
	if (showEntityListWindow)   entityListWindow();
	if (showLogWindow)          logWindow();
	if (showFilePane)           filepane.render();

	if (showObjectEditorWindow) objectEditorWindow();

	// avoid having imgui take focus when starting
	static bool initialized = false;
	if (!initialized) {
		ImGui::SetWindowFocus(nullptr);
		initialized = true;
	}


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

