#include <grend/game_editor.hpp>

#include <imgui/imgui.h>
#include <imgui/examples/imgui_impl_sdl.h>
#include <imgui/examples/imgui_impl_opengl3.h>

using namespace grendx;

static void editPointLight(gameMain *game, gameLightPoint::ptr light) {
	ImGui::Text("Point light properties");
	ImGui::Separator();
	ImGui::Indent(16.f);
	ImGui::SliderFloat("radius", &light->radius, 0.01f, 3.f);
	ImGui::Unindent(16.f);
}

static void editSpotLight(gameMain *game, gameLightSpot::ptr light) {
	ImGui::Text("Spot light properties");
	ImGui::Separator();
	ImGui::Indent(16.f);
	ImGui::SliderFloat("radius", &light->radius, 0.01f, 3.f);
	ImGui::SliderFloat("angle", &light->angle, 0.f, 1.f);
	ImGui::Unindent(16.f);
}

static void editDirectionalLight(gameMain *game, gameLightDirectional::ptr light) {
	ImGui::Text("Directional light properties");
	ImGui::Separator();
	ImGui::Indent(16.f);
	//ImGui::Text("(This space intentionally left blank)");
	ImGui::Unindent(16.f);
}

static void editLight(gameMain *game, gameLight::ptr light) {
	ImGui::Text("Light properties");
	ImGui::Indent(16.f);
	ImGui::Separator();
	ImGui::ColorEdit4("Color", glm::value_ptr(light->diffuse));
	ImGui::SliderFloat("Intensity", &light->intensity, 0.f, 1000.f);
	ImGui::Checkbox("Casts shadows", &light->casts_shadows);
	ImGui::Checkbox("Static shadows", &light->is_static);
	ImGui::Checkbox("Rendered", &light->have_map);
	ImGui::Unindent(16.f);

	switch (light->lightType) {
		case gameLight::lightTypes::Point:
			editPointLight(game, std::dynamic_pointer_cast<gameLightPoint>(light));
			break;

		case gameLight::lightTypes::Spot:
			editSpotLight(game, std::dynamic_pointer_cast<gameLightSpot>(light));
			break;

		case gameLight::lightTypes::Directional:
			editDirectionalLight(game, std::dynamic_pointer_cast<gameLightDirectional>(light));
			break;

		default:
			break;
	}
}

static void editRefProbe(gameMain *game, gameReflectionProbe::ptr probe) {
	ImGui::Text("Reflection probe properties");
	ImGui::Separator();
	ImGui::Indent(16.f);
	ImGui::Checkbox("Static reflections", &probe->is_static);
	ImGui::Checkbox("Rendered", &probe->have_map);
	ImGui::Unindent(16.f);
}

void game_editor::objectEditorWindow(gameMain *game) {
	if (!selectedNode) {
		// should check before calling this, but just in case...
		return;
	}

	ImGui::Begin("Object editor", &show_object_editor_window, 
	             ImGuiWindowFlags_AlwaysAutoResize);

	ImGui::Text(selectedNode->typeString().c_str());
	ImGui::Separator();
	ImGui::Indent(16.f);
	ImGui::InputFloat3("position",
		glm::value_ptr(selectedNode->transform.position));
	ImGui::InputFloat3("scale",
		glm::value_ptr(selectedNode->transform.scale));
	ImGui::InputFloat4("rotation",
		glm::value_ptr(selectedNode->transform.rotation));
	ImGui::Unindent(16.f);

	if (selectedNode->type == gameObject::objType::Light) {
		editLight(game, std::dynamic_pointer_cast<gameLight>(selectedNode));

	} else if(selectedNode->type == gameObject::objType::ReflectionProbe) {
		editRefProbe(game,
			std::dynamic_pointer_cast<gameReflectionProbe>(selectedNode));
	}

	{
		ImGui::Text("Animations");
		ImGui::Separator();
		ImGui::Indent(16.f);

		for (auto& chan : selectedNode->animations) {
			ImGui::SliderFloat(chan->group->name.c_str(),
			                   &chan->group->weight, 0.f, 1.f);
			ImGui::Separator();
		}
		ImGui::Unindent(16.f);
	}

	ImGui::End();
}
