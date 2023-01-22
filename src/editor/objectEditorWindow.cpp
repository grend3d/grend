#include <grend/gameEditor.hpp>

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_sdl.h>
#include <imgui/backends/imgui_impl_opengl3.h>

using namespace grendx;
using namespace grendx::engine;

static void editPointLight(sceneLightPoint::ptr light) {
	ImGui::Text("%s", "Point light properties");
	ImGui::Separator();
	ImGui::Indent(16.f);
	ImGui::SliderFloat("radius", &light->radius, 0.01f, 3.f);
	ImGui::Unindent(16.f);
}

static void editSpotLight(sceneLightSpot::ptr light) {
	ImGui::Text("%s", "Spot light properties");
	ImGui::Separator();
	ImGui::Indent(16.f);
	ImGui::SliderFloat("radius", &light->radius, 0.01f, 3.f);
	ImGui::SliderFloat("angle", &light->angle, 0.f, 1.f);
	ImGui::Unindent(16.f);
}

static void editDirectionalLight(sceneLightDirectional::ptr light) {
	ImGui::Text("%s", "Directional light properties");
	ImGui::Separator();
	ImGui::Indent(16.f);
	//ImGui::Text("(This space intentionally left blank)");
	ImGui::Unindent(16.f);
}

static void editLight(sceneLight::ptr light) {
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
		case sceneLight::lightTypes::Point:
			editPointLight(ref_cast<sceneLightPoint>(light));
			break;

		case sceneLight::lightTypes::Spot:
			editSpotLight(ref_cast<sceneLightSpot>(light));
			break;

		case sceneLight::lightTypes::Directional:
			editDirectionalLight(ref_cast<sceneLightDirectional>(light));
			break;

		default:
			break;
	}
}

template <class T>
static void editRefProbe(T probe) {
	ImGui::Text("%s", "Reflection probe properties");
	ImGui::Separator();
	ImGui::Indent(16.f);
	ImGui::InputFloat3("Bounding box minimum", glm::value_ptr(probe->boundingBox.min));
	ImGui::InputFloat3("Bounding box maximum", glm::value_ptr(probe->boundingBox.max));
	ImGui::Checkbox("Static reflections", &probe->is_static);
	ImGui::Checkbox("Rendered", &probe->have_map);
	ImGui::Unindent(16.f);
}

static void editModel(sceneModel::ptr model) {
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

void gameEditorUI::objectEditorWindow() {
	auto selectedNode = editor->getSelectedNode();

	if (!selectedNode) {
		// should check before calling this, but just in case...
		return;
	}

	ImGui::Begin("Object editor", &showObjectEditorWindow, 
	             ImGuiWindowFlags_AlwaysAutoResize);

	ImGui::Text("%s", selectedNode->typeString());
	ImGui::Separator();
	ImGui::Indent(16.f);

	TRS selectedTransform = selectedNode->getTransformTRS();

	if (ImGui::InputFloat3("Position", glm::value_ptr(selectedTransform.position))
	    || ImGui::InputFloat3("Scale", glm::value_ptr(selectedTransform.scale))
	    || ImGui::InputFloat4("Rotation", glm::value_ptr(selectedTransform.rotation)))
	{
		selectedNode->setTransform(selectedTransform);
	}

	ImGui::Checkbox("Visible", &selectedNode->visible);
	ImGui::Unindent(16.f);

	if (selectedNode->type == sceneNode::objType::Light) {
		editLight(ref_cast<sceneLight>(selectedNode));

	} else if(selectedNode->type == sceneNode::objType::ReflectionProbe) {
		editRefProbe(ref_cast<sceneReflectionProbe>(selectedNode));

	} else if(selectedNode->type == sceneNode::objType::IrradianceProbe) {
		editRefProbe(ref_cast<sceneIrradianceProbe>(selectedNode));

	} else if(selectedNode->type == sceneNode::objType::Model) {
		editModel(ref_cast<sceneModel>(selectedNode));
	}

	ImGui::Separator();
	ImGui::Text("%s: %08x", "Animations channel", selectedNode->animChannel);

	ImGui::End();
}
