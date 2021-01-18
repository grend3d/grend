#include <grend/gameEditor.hpp>

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_sdl.h>
#include <imgui/backends/imgui_impl_opengl3.h>

using namespace grendx;

static void editPointLight(gameMain *game, gameLightPoint::ptr light) {
	ImGui::Text("%s", "Point light properties");
	ImGui::Separator();
	ImGui::Indent(16.f);
	ImGui::SliderFloat("radius", &light->radius, 0.01f, 3.f);
	ImGui::Unindent(16.f);
}

static void editSpotLight(gameMain *game, gameLightSpot::ptr light) {
	ImGui::Text("%s", "Spot light properties");
	ImGui::Separator();
	ImGui::Indent(16.f);
	ImGui::SliderFloat("radius", &light->radius, 0.01f, 3.f);
	ImGui::SliderFloat("angle", &light->angle, 0.f, 1.f);
	ImGui::Unindent(16.f);
}

static void editDirectionalLight(gameMain *game, gameLightDirectional::ptr light) {
	ImGui::Text("%s", "Directional light properties");
	ImGui::Separator();
	ImGui::Indent(16.f);
	//ImGui::Text("(This space intentionally left blank)");
	ImGui::Unindent(16.f);
}

static void editLight(gameMain *game, gameLight::ptr light) {
	ImGui::Text("%s", "Light properties");
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

template <class T>
static void editRefProbe(gameMain *game, T probe) {
	ImGui::Text("%s", "Reflection probe properties");
	ImGui::Separator();
	ImGui::Indent(16.f);
	ImGui::InputFloat3("Bounding box minimum", glm::value_ptr(probe->boundingBox.min));
	ImGui::InputFloat3("Bounding box maximum", glm::value_ptr(probe->boundingBox.max));
	ImGui::Checkbox("Static reflections", &probe->is_static);
	ImGui::Checkbox("Rendered", &probe->have_map);
	ImGui::Unindent(16.f);
}

static void editModel(gameMain *game, gameModel::ptr model) {
	if (model->comped_model) {
		// TODO: reimplement material editing stuff, will be able to do much
		//       more after refactoring stuff here
#if 0
		for (auto& [name, mat] : model->comped_model->materials) {
			std::string matname = "Material " + name;
			ImGui::Separator();
			ImGui::Text(matname.c_str());
			ImGui::Indent(16.f);

			ImGui::ColorEdit4("Diffuse factor", glm::value_ptr(mat.factors.diffuse));
			ImGui::InputFloat4("Emission factor", glm::value_ptr(mat.factors.emissive));
			ImGui::SliderFloat("Roughness", &mat.factors.roughness, 0.f, 1.f);
			ImGui::SliderFloat("Metalness", &mat.factors.metalness, 0.f, 1.f);
			ImGui::SliderFloat("Opacity", &mat.factors.opacity, 0.f, 1.f);
			ImGui::SliderFloat("Refraction index", &mat.factors.refract_idx, 0.1f, 10.f);

			ImGui::Unindent(16.f);
		}
#endif
	}
}

void gameEditor::objectEditorWindow(gameMain *game) {
	if (!selectedNode) {
		// should check before calling this, but just in case...
		return;
	}

	ImGui::Begin("Object editor", &showObjectEditorWindow, 
	             ImGuiWindowFlags_AlwaysAutoResize);

	ImGui::Text("%s", selectedNode->typeString().c_str());
	ImGui::Separator();
	ImGui::Indent(16.f);
	ImGui::InputFloat3("Position",
		glm::value_ptr(selectedNode->transform.position));
	ImGui::InputFloat3("Scale",
		glm::value_ptr(selectedNode->transform.scale));
	ImGui::InputFloat4("Rotation",
		glm::value_ptr(selectedNode->transform.rotation));
	ImGui::Checkbox("Visible", &selectedNode->visible);
	ImGui::Unindent(16.f);

	if (selectedNode->type == gameObject::objType::Light) {
		editLight(game, std::dynamic_pointer_cast<gameLight>(selectedNode));

	} else if(selectedNode->type == gameObject::objType::ReflectionProbe) {
		editRefProbe(game,
			std::dynamic_pointer_cast<gameReflectionProbe>(selectedNode));

	} else if(selectedNode->type == gameObject::objType::IrradianceProbe) {
		editRefProbe(game,
			std::dynamic_pointer_cast<gameIrradianceProbe>(selectedNode));

	} else if(selectedNode->type == gameObject::objType::Model) {
		editModel(game, std::dynamic_pointer_cast<gameModel>(selectedNode));
	}

	{
		ImGui::Text("%s", "Animations");
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
