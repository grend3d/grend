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

void entity::drawEditor(component *comp) {
	auto *ent = beginType<entity>(comp);

	TRS selectedTransform = ent->transform.getTRS();

	if (ImGui::InputFloat3("Position", glm::value_ptr(selectedTransform.position))
	 || ImGui::InputFloat3("Scale",    glm::value_ptr(selectedTransform.scale))
	 || ImGui::InputFloat4("Rotation", glm::value_ptr(selectedTransform.rotation)))
	{
		ent->transform.set(selectedTransform);
	}

	//ImGui::Checkbox("Visible", &ent->visible);
	endType();
}

void rigidBody::drawEditor(component *comp) {
	auto *body = beginType<rigidBody>(comp);

	if (ImGui::InputFloat ("Mass",     &body->mass)
	 || ImGui::InputFloat3("Position", glm::value_ptr(body->position)))
	{
		// TODO:
	}

	endType();
}

void rigidBodySphere::drawEditor(component *comp) {
	auto *body = beginType<rigidBodySphere>(comp);

	if (ImGui::InputFloat("Radius", &body->radius))
	{
		// TODO:
	}

	endType();
}

void rigidBodyCapsule::drawEditor(component *comp) {
	auto *body = beginType<rigidBodyCapsule>(comp);

	if (ImGui::InputFloat("Radius", &body->radius)
	 || ImGui::InputFloat("Height", &body->height))
	{
		// TODO:
	}

	endType();
}

void rigidBodyBox::drawEditor(component *comp) {
	auto *body = beginType<rigidBodyBox>(comp);

	if (ImGui::InputFloat3("Center", glm::value_ptr(body->extent.center))
	 || ImGui::InputFloat3("Extent", glm::value_ptr(body->extent.extent)))
	{
		// TODO:
	}

	endType();
}

void rigidBodyCylinder::drawEditor(component *comp) {
	auto *body = beginType<rigidBodyCylinder>(comp);

	if (ImGui::InputFloat3("Center", glm::value_ptr(body->extent.center))
	 || ImGui::InputFloat3("Extent", glm::value_ptr(body->extent.extent)))
	{
		// TODO:
	}

	endType();
}

void sceneComponent::drawEditor(component *comp) {
	auto *scene = beginType<sceneComponent>(comp);

	if (!scene->editBuffer) {
		scene->editBuffer = new sceneComponent::Buf();
		strncpy(scene->editBuffer->data(),
				scene->getPath().c_str(),
				sceneComponent::BufSize - 1);
	}

	char *data = scene->editBuffer->data();
	ImGui::InputText("##edit", data, sceneComponent::BufSize);

	if (ImGui::BeginDragDropTarget()) {
		if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("DRAG_FILENAME")) {
			const char *fname = (const char*)payload->Data;
			strncpy(data, fname, sceneComponent::BufSize - 1);

			scene->load(data, sceneComponent::Reference);
		}
	}

	ImGui::SameLine();
	if (ImGui::Button("OK")) {
		scene->load(data, sceneComponent::Reference);
	}

	endType();
}
