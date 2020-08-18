#include <grend/game_editor.hpp>

#define IMGUI_IMPL_OPENGL_LOADER_GLEW
#include <imgui/imgui.h>
#include <imgui/examples/imgui_impl_sdl.h>
#include <imgui/examples/imgui_impl_opengl3.h>

using namespace grendx;

void game_editor::lights_window(gameMain *main) {
	ImGui::Begin("Lights", &show_lights_window);
	ImGui::Columns(2);

	// TODO: probably don't need this, can have in "inspector" window
	//       that displays object info

#if 0
	for (auto& [id, light] : rend->point_lights) {
		std::string name = "Point light " + std::to_string(id);

		ImGui::Separator();
		if (ImGui::Selectable(name.c_str(), selected_light == (int)id)) {
			selected_light = id;
		}

		ImGui::NextColumn();
		ImGui::InputFloat3("position", glm::value_ptr(light.position));

		if (selected_light == (int)id) {
			ImGui::ColorEdit4("color", glm::value_ptr(light.diffuse));
			ImGui::SliderFloat("intensity", &light.intensity, 0.f, 1000.f);
			ImGui::SliderFloat("radius", &light.radius, 0.01f, 3.f);
			ImGui::Checkbox("casts shadows", &light.casts_shadows);
			ImGui::Checkbox("static shadows", &light.static_shadows);

			rend->draw_model_lines({
				.name = "smoothsphere",
				.transform = glm::translate(glm::vec3(light.position))
					* glm::scale(glm::vec3(light_extent(&light, light_threshold))),
			});
		}

		ImGui::NextColumn();
	}

	for (auto& [id, light] : rend->spot_lights) {
		std::string name = "Spot light " + std::to_string(id);

		ImGui::Separator();
		if (ImGui::Selectable(name.c_str(), selected_light == (int)id)) {
			selected_light = id;
		}

		ImGui::NextColumn();
		ImGui::InputFloat3("position", glm::value_ptr(light.position));

		if (selected_light == (int)id) {
			ImGui::ColorEdit4("color", glm::value_ptr(light.diffuse));
			// TODO: normalize, better representation
			ImGui::SliderFloat3("direction", glm::value_ptr(light.direction),
				-1.f, 1.f);
			ImGui::SliderFloat("intensity", &light.intensity, 0.f, 1000.f);
			ImGui::SliderFloat("radius", &light.radius, 0.01f, 3.f);
			ImGui::SliderFloat("angle", &light.angle, 0.0f, 1.f);
			ImGui::Checkbox("casts shadows", &light.casts_shadows);
			ImGui::Checkbox("static shadows", &light.static_shadows);

			/*
			rend->draw_model_lines("smoothsphere",
				glm::translate(glm::vec3(light.position))
				* glm::scale(glm::vec3(light_extent(&light, light_threshold))));
				*/
		}

		ImGui::NextColumn();
	}

	for (auto& [id, light] : rend->directional_lights) {
		std::string name = "directional light " + std::to_string(id);

		ImGui::Separator();
		if (ImGui::Selectable(name.c_str(), selected_light == (int)id)) {
			selected_light = id;
		}

		ImGui::NextColumn();
		ImGui::InputFloat3("position", glm::value_ptr(light.position));

		if (selected_light == (int)id) {
			ImGui::ColorEdit4("color", glm::value_ptr(light.diffuse));
			// TODO: normalize, better representation
			ImGui::SliderFloat3("direction", glm::value_ptr(light.direction),
				-1.f, 1.f);
			ImGui::SliderFloat("intensity", &light.intensity, 0.f, 1000.f);
			ImGui::Checkbox("casts shadows", &light.casts_shadows);
			ImGui::Checkbox("static shadows", &light.static_shadows);

			rend->draw_model_lines({
				.name = "smoothsphere",
				.transform = glm::translate(glm::vec3(light.position))
			});
		}

		ImGui::NextColumn();
	}


	ImGui::Columns(1);
	ImGui::Separator();

	if (ImGui::Button("Add Point Light")) {
		set_mode(mode::AddPointLight);
	}

	ImGui::SameLine();
	if (ImGui::Button("Add Spot Light")) {
		set_mode(mode::AddSpotLight);
	}

	ImGui::SameLine();
	if (ImGui::Button("Add Directional Light")) {
		set_mode(mode::AddDirectionalLight);
	}

	ImGui::SameLine();
	if (ImGui::Button("Delete Light")) {
		rend->free_light(selected_light);
	}
	*/
#endif

	ImGui::End();
}
