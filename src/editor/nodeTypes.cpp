#include <grend/gameEditor.hpp>
#include <grend/utility.hpp>

#include <grend/ecs/ecs.hpp>
#include <grend/ecs/serializer.hpp>
#include <grend/ecs/rigidBody.hpp>
#include <grend/ecs/sceneComponent.hpp>

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_sdl.h>
#include <imgui/backends/imgui_impl_opengl3.h>

using namespace grendx;
using namespace grendx::ecs;

template <typename T>
static T* beginType(component *comp) {
	auto name = demangle(getTypeName<T>());

	ImGui::Text("%s", name.c_str());
	ImGui::Separator();
	ImGui::Indent(16.f);
	ImGui::PushID(0);

	return static_cast<T*>(comp);
}

static void endType() {
	ImGui::PopID();
	ImGui::Unindent(16.f);
}

void sceneNode::drawEditor(component *comp) {
	auto *ent = beginType<sceneNode>(comp);

	ImGui::Checkbox("Visible", &ent->visible);

	if (ent->animChannel) {
		ImGui::Text("%d", ent->animChannel);
	}

	// TODO: extra properties

	endType();
}

void sceneImport::drawEditor(component *comp) {
	auto *ent = beginType<sceneImport>(comp);

	ImGui::Text("Source: '%s'", ent->sourceFile.c_str());

	endType();
}

static inline bool InputUInt(const char* label, unsigned *v) {
	return ImGui::InputInt(label, (int *)v);
}

void sceneParticles::drawEditor(component *comp) {
	auto *ent = beginType<sceneParticles>(comp);

	if (ImGui::InputFloat("Radius",          &ent->radius)
	        || InputUInt("Max Instances",    &ent->maxInstances)
	        || InputUInt("Active Instances", &ent->activeInstances))
	{
	}

	endType();
}

void sceneBillboardParticles::drawEditor(component *comp) {
	auto *ent = beginType<sceneBillboardParticles>(comp);

	if (ImGui::InputFloat("Radius",          &ent->radius)
	        || InputUInt("Max Instances",    &ent->maxInstances)
	        || InputUInt("Active Instances", &ent->activeInstances))
	{
	}

	endType();
}

void sceneLight::drawEditor(component *comp) {
	auto *ent = beginType<sceneLight>(comp);

	ImGui::ColorEdit4("Color", glm::value_ptr(ent->diffuse));
	ImGui::SliderFloat("Intensity", &ent->intensity, 0.f, 1000.f);
	ImGui::Checkbox("Casts shadows", &ent->casts_shadows);
	ImGui::Checkbox("Static shadows", &ent->is_static);

	endType();
}

void sceneLightPoint::drawEditor(component *comp) {
	auto *ent = beginType<sceneLightPoint>(comp);

	ImGui::SliderFloat("Radius", &ent->radius, 0.01f, 3.f);

	endType();
}

void sceneLightSpot::drawEditor(component *comp) {
	auto *ent = beginType<sceneLightSpot>(comp);

	ImGui::SliderFloat("Radius", &ent->radius, 0.01f, 3.f);
	ImGui::SliderFloat("Angle",  &ent->angle, 0.f, 1.f);

	endType();
}

void sceneLightDirectional::drawEditor(component *comp) {
	auto *ent = beginType<sceneLightSpot>(comp);

	ImGui::Text("TODO: maybe add stuff here");

	endType();
}

void sceneReflectionProbe::drawEditor(component *comp) {
	auto *ent = beginType<sceneReflectionProbe>(comp);

	ImGui::InputFloat3("Bounding box minimum", glm::value_ptr(ent->boundingBox.min));
	ImGui::InputFloat3("Bounding box maximum", glm::value_ptr(ent->boundingBox.max));
	ImGui::Checkbox("Static reflections", &ent->is_static);
	ImGui::Checkbox("Rendered", &ent->have_map);

	endType();
}

void sceneIrradianceProbe::drawEditor(component *comp) {
	auto *ent = beginType<sceneIrradianceProbe>(comp);

	ImGui::InputFloat3("Bounding box minimum", glm::value_ptr(ent->boundingBox.min));
	ImGui::InputFloat3("Bounding box maximum", glm::value_ptr(ent->boundingBox.max));
	ImGui::Checkbox("Static reflections", &ent->is_static);
	ImGui::Checkbox("Rendered", &ent->have_map);

	endType();
}
