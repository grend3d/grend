#include <grend/engine.hpp>
#include <grend/model.hpp>
#include <vector>
#include <map>

using namespace grendx;

static std::map<std::string, material> default_materials = {
	{"(null)", {
				   .diffuse = {0.75, 0.75, 0.75, 1},
				   .ambient = {1, 1, 1, 1},
				   .specular = {0.5, 0.5, 0.5, 1},
				   .shininess = 15,

				   .diffuse_map     = "assets/tex/white.png",
				   .specular_map    = "assets/tex/white.png",
				   .normal_map      = "assets/tex/lightblue-normal.png",
				   .ambient_occ_map = "assets/tex/white.png",
			   }},

	{"Black",  {
				   .diffuse = {1, 0.8, 0.2, 1},
				   .ambient = {1, 0.8, 0.2, 1},
				   .specular = {1, 1, 1, 1},
				   .shininess = 50
			   }},

	{"Grey",   {
				   .diffuse = {0.1, 0.1, 0.1, 0.5},
				   .ambient = {0.1, 0.1, 0.1, 0.2},
				   .specular = {0.0, 0.0, 0.0, 0.05},
				   .shininess = 15
			   }},

	{"Yellow", {
				   .diffuse = {0.01, 0.01, 0.01, 1},
				   .ambient = {0, 0, 0, 1},
				   .specular = {0.2, 0.2, 0.2, 0.2},
				   .shininess = 20,
			   }},

	{"Gravel",  {
				   .diffuse = {1, 1, 1, 1},
				   .ambient = {1, 1, 1, 1},
				   .specular = {1, 1, 1, 1},
				   .shininess = 5,

				   .diffuse_map  = "assets/tex/dims/Textures/Gravel.JPG",
				   .specular_map = "assets/tex/white.png",
				   .normal_map   = "assets/tex/dims/Textures/Textures_N/Gravel_N.jpg",

				   // materials from freepbr.com are way higher quality, but I'm not sure how
				   // github would feel about hosting a few hundred megabytes to gigabytes of
				   // textures

				   /*
				   .diffuse_map  = "assets/tex/octostone-Unreal-Engine/octostoneAlbedo.png",
				   .specular_map = "assets/tex/octostone-Unreal-Engine/octostoneMetallic.png",
				   .normal_map   = "assets/tex/octostone-Unreal-Engine/octostoneNormalc.png",
				   .ambient_occ_map = "assets/tex/octostone-Unreal-Engine/octostoneAmbient_Occlusionc.png",
				   */

				   /*
				   .diffuse_map  = "assets/tex/octostone-Unreal-Engine/256px-octostoneAlbedo.png",
				   .specular_map = "assets/tex/octostone-Unreal-Engine/256px-octostoneMetallic.png",
				   .normal_map   = "assets/tex/octostone-Unreal-Engine/256px-octostoneNormalc.png",
				   .ambient_occ_map = "assets/tex/octostone-Unreal-Engine/256px-octostoneAmbient_Occlusionc.png",
				   */

				   /*
				   .diffuse_map  = "assets/tex/octostone-Unreal-Engine/128px-octostoneAlbedo.png",
				   .specular_map = "assets/tex/octostone-Unreal-Engine/128px-octostoneMetallic.png",
				   .normal_map   = "assets/tex/octostone-Unreal-Engine/128px-octostoneNormalc.png",
				   .ambient_occ_map = "assets/tex/octostone-Unreal-Engine/128px-octostoneAmbient_Occlusionc.png",
				   */
			   }},

	{"Steel",  {
				   .diffuse = {1, 1, 1, 1},
				   .ambient = {1, 1, 1, 1},
				   .specular = {1, 1, 1, 1},
				   .shininess = 35,

				   .diffuse_map  = "assets/tex/rubberduck-tex/199.JPG",
				   .specular_map = "assets/tex/white.png",
				   .normal_map   = "assets/tex/rubberduck-tex/199_norm.JPG",
				   .ambient_occ_map = "assets/tex/white.png",

				   /*
				   .diffuse_map  = "assets/tex/iron-rusted4-Unreal-Engine/iron-rusted4-basecolor.png",
				   .specular_map = "assets/tex/iron-rusted4-Unreal-Engine/iron-rusted4-metalness.png",
				   .normal_map   = "assets/tex/iron-rusted4-Unreal-Engine/iron-rusted4-normal.png",
				   */
			   }},

	{"Brick",  {
				   .diffuse = {1, 1, 1, 1},
				   .ambient = {1, 1, 1, 1},
				   .specular = {1, 1, 1, 1},
				   .shininess = 3,

				   .diffuse_map  = "assets/tex/rubberduck-tex/179.JPG",
				   .specular_map = "assets/tex/white.png",
				   .normal_map   = "assets/tex/rubberduck-tex/179_norm.JPG",
				   .ambient_occ_map = "assets/tex/white.png",
				   /*
				   .diffuse_map  = "assets/tex/dims/Textures/Chimeny.JPG",
				   .specular_map = "assets/tex/white.png",
				   .normal_map   = "assets/tex/dims/Textures/Textures_N/Chimeny_N.jpg",
				   */
			   }},


	{"Rock",  {
				   .diffuse = {1, 1, 1, 1},
				   .ambient = {1, 1, 1, 1},
				   .specular = {1, 1, 1, 0.5},
				   .shininess = 5,

				   .diffuse_map  = "assets/tex/rubberduck-tex/165.JPG",
				   .specular_map = "assets/tex/white.png",
				   .normal_map   = "assets/tex/rubberduck-tex/165_norm.JPG",
				   .ambient_occ_map = "assets/tex/white.png",
			   }},

	{"Wood",  {
				   .diffuse = {1, 1, 1, 1},
				   .ambient = {1, 1, 1, 1},
				   .specular = {1, 1, 1, 0.3},
				   .shininess = 5,

				   .diffuse_map  = "assets/tex/dims/Textures/Boards.JPG",
				   .specular_map = "assets/tex/white.png",
				   .normal_map   = "assets/tex/dims/Textures/Textures_N/Boards_N.jpg",
				   .ambient_occ_map = "assets/tex/white.png",
			   }},

	{"Clover",  {
				   .diffuse = {1, 1, 1, 1},
				   .ambient = {1, 1, 1, 1},
				   .specular = {1, 1, 1, 0.5},
				   .shininess = 1,

				   .diffuse_map  = "assets/tex/dims/Textures/GroundCover.JPG",
				   .specular_map = "assets/tex/white.png",
				   .normal_map   = "assets/tex/dims/Textures/Textures_N/GroundCover_N.jpg",
				   .ambient_occ_map = "assets/tex/white.png",
			   }},
};

engine::engine() {
	for (auto& thing : default_materials) {
		if (!thing.second.diffuse_map.empty()) {
			diffuse_handles[thing.first] = glman.load_texture(thing.second.diffuse_map);
		}

		if (!thing.second.specular_map.empty()) {
			specular_handles[thing.first] = glman.load_texture(thing.second.specular_map);
		}

		if (!thing.second.normal_map.empty()) {
			normmap_handles[thing.first] = glman.load_texture(thing.second.normal_map);
		}

		if (!thing.second.ambient_occ_map.empty()) {
			aomap_handles[thing.first] = glman.load_texture(thing.second.ambient_occ_map);
		}
	}
}

void engine::set_material(gl_manager::compiled_model& obj, std::string mat_name) {
	if (obj.materials.find(mat_name) == obj.materials.end()) {
		// TODO: maybe show a warning
		set_default_material(mat_name);

	} else {
		material& mat = obj.materials[mat_name];

		glUniform4f(glGetUniformLocation(shader.first, "anmaterial.diffuse"),
				mat.diffuse.x, mat.diffuse.y, mat.diffuse.z, mat.diffuse.w);
		glUniform4f(glGetUniformLocation(shader.first, "anmaterial.ambient"),
				mat.ambient.x, mat.ambient.y, mat.ambient.z, mat.ambient.w);
		glUniform4f(glGetUniformLocation(shader.first, "anmaterial.specular"),
				mat.specular.x, mat.specular.y, mat.specular.z, mat.specular.w);
		glUniform1f(glGetUniformLocation(shader.first, "anmaterial.shininess"),
				mat.shininess);

		glActiveTexture(GL_TEXTURE0);
		if (!mat.diffuse_map.empty()) {
			glBindTexture(GL_TEXTURE_2D, obj.mat_textures[mat_name].first);
			glUniform1i(u_diffuse_map, 0);

		} else {
			glBindTexture(GL_TEXTURE_2D, diffuse_handles[fallback_material].first);
			glUniform1i(u_diffuse_map, 0);
		}

		glActiveTexture(GL_TEXTURE1);
		if (!mat.specular_map.empty()) {
			// TODO: specular maps
		} else {
			glBindTexture(GL_TEXTURE_2D, specular_handles[fallback_material].first);
			glUniform1i(u_specular_map, 1);
		}

		glActiveTexture(GL_TEXTURE2);
		if (!mat.normal_map.empty()) {
			// TODO: normal maps

		} else {
			glBindTexture(GL_TEXTURE_2D, normmap_handles[fallback_material].first);
			glUniform1i(u_normal_map, 2);
		}

		glActiveTexture(GL_TEXTURE3);
		if (!mat.ambient_occ_map.empty()) {
			// TODO: normal maps

		} else {
			glBindTexture(GL_TEXTURE_2D, aomap_handles[fallback_material].first);
			glUniform1i(u_ao_map, 3);
		}
	}
}

void engine::set_default_material(std::string mat_name) {
	if (default_materials.find(mat_name) == default_materials.end()) {
		// TODO: really show an error here
		mat_name = fallback_material;
		puts("asdf");
	}

	material& mat = default_materials[mat_name];

	glUniform4f(glGetUniformLocation(shader.first, "anmaterial.diffuse"),
			mat.diffuse.x, mat.diffuse.y, mat.diffuse.z, mat.diffuse.w);
	glUniform4f(glGetUniformLocation(shader.first, "anmaterial.ambient"),
			mat.ambient.x, mat.ambient.y, mat.ambient.z, mat.ambient.w);
	glUniform4f(glGetUniformLocation(shader.first, "anmaterial.specular"),
			mat.specular.x, mat.specular.y, mat.specular.z, mat.specular.w);
	glUniform1f(glGetUniformLocation(shader.first, "anmaterial.shininess"),
			mat.shininess);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, diffuse_handles[mat.diffuse_map.empty()?
	                                                 fallback_material
	                                                 : mat_name].first);
	glUniform1i(u_diffuse_map, 0);


	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, specular_handles[mat.specular_map.empty()?
	                                                 fallback_material
	                                                 : mat_name].first);
	glUniform1i(u_specular_map, 1);

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, normmap_handles[mat.normal_map.empty()?
	                                                 fallback_material
	                                                 : mat_name].first);
	glUniform1i(u_normal_map, 2);
}

void engine::set_mvp(glm::mat4 mod, glm::mat4 view, glm::mat4 projection) {
	glm::mat4 v_inv = glm::inverse(view);

	glUniformMatrix4fv(glGetUniformLocation(shader.first, "v"),
			1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(glGetUniformLocation(shader.first, "p"),
			1, GL_FALSE, glm::value_ptr(projection));

	glUniformMatrix4fv(glGetUniformLocation(shader.first, "v_inv"),
			1, GL_FALSE, glm::value_ptr(v_inv));
}

static glm::mat4 model_to_world(glm::mat4 model) {
	glm::quat rotation = glm::quat_cast(model);
	//rotation = -glm::conjugate(rotation);

	return glm::mat4_cast(rotation);
}

void engine::set_m(glm::mat4 mod) {
	glm::mat3 m_3x3_inv_transp = glm::transpose(glm::inverse(model_to_world(mod)));

	glUniformMatrix4fv(glGetUniformLocation(shader.first, "m"),
			1, GL_FALSE, glm::value_ptr(mod));
	glUniformMatrix3fv(glGetUniformLocation(shader.first, "m_3x3_inv_transp"),
			1, GL_FALSE, glm::value_ptr(m_3x3_inv_transp));
}
