#include <grend/gameEditor.hpp>
#include <grend/utility.hpp>

#include <grend/ecs/ecs.hpp>
#include <grend/ecs/serializer.hpp>

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_sdl.h>
#include <imgui/backends/imgui_impl_opengl3.h>

using namespace grendx;
using namespace grendx::ecs;

void entity::drawEditor(component *comp) {
	entity *ent = static_cast<entity*>(comp);

	ImGui::Text("Entity");
	ImGui::Separator();
	ImGui::Indent(16.f);

	TRS selectedTransform = ent->transform;
	//TRS selectedTransform = ent->getTransformTRS();

	if (ImGui::InputFloat3("Position", glm::value_ptr(selectedTransform.position))
	 || ImGui::InputFloat3("Scale",    glm::value_ptr(selectedTransform.scale))
	 || ImGui::InputFloat4("Rotation", glm::value_ptr(selectedTransform.rotation)))
	{
		ent->transform = selectedTransform;
		//ent->setTransform(selectedTransform);
	}

	//ImGui::Checkbox("Visible", &ent->visible);
	ImGui::Unindent(16.f);
}

