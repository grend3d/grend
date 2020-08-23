#include <grend/engine.hpp>
#include <grend/utility.hpp>
#include <grend/texture-atlas.hpp>
#include <math.h>

using namespace grendx;

void renderQueue::add(gameObject::ptr obj, glm::mat4 trans, bool inverted) {
	if (obj == nullptr) {
		// shouldn't happen, but just in case
		return;
	}

	glm::mat4 adjTrans = trans*obj->getTransform();
	//glm::mat4 adjTrans = obj->getTransform();

	//std::cerr << "add(): push" << std::endl;
	
	if (obj->scale.x < 0 || obj->scale.y < 0 || obj->scale.z < 0) {
		inverted = !inverted;
	}

	switch (obj->type) {
		case gameObject::objType::Mesh:
			{
				gameMesh::ptr mesh = std::dynamic_pointer_cast<gameMesh>(obj);
				meshes.push_back({adjTrans, inverted, mesh});
			}
			break;

		case gameObject::objType::Light:
			{
				gameLight::ptr light = std::dynamic_pointer_cast<gameLight>(obj);
				lights.push_back({adjTrans, inverted, light});
			}
			break;

		case gameObject::objType::ReflectionProbe:
			{
				gameReflectionProbe::ptr probe =
					std::dynamic_pointer_cast<gameReflectionProbe>(obj);
				probes.push_back({adjTrans, inverted, probe});
			}
			break;

		default:
			break;
	}

	for (auto& [name, ptr] : obj->nodes) {
		//std::cerr << "add(): subnode " << name << std::endl;
		add(ptr, adjTrans, inverted);
	}

	//std::cerr << "add(): pop" << std::endl;
}

void renderQueue::updateLights(Program::ptr program, renderAtlases& atlases) {
	// TODO: should probably apply the transform to light position
	for (auto& [_, __, light] : lights) {
		if (light->lightType == gameLight::lightTypes::Point) {
			gameLightPoint::ptr plit =
				std::dynamic_pointer_cast<gameLightPoint>(light);
			drawShadowCubeMap(*this, plit, program, atlases);
		}
	}
}

void
renderQueue::updateReflections(Program::ptr program,
                              renderAtlases& atlases,
                              skybox& sky)
{
	for (auto& [_, __, probe] : probes) {
		drawReflectionProbe(*this, probe, program, atlases, sky);
	}
}

void renderQueue::sort(void) {

}

void renderQueue::cull(void) {

}

void renderQueue::flush(unsigned width,
                        unsigned height,
                        Program::ptr program,
                        renderAtlases& atlases)
{
	enable(GL_CULL_FACE);

	glm::mat4 view = cam->viewTransform();
	glm::mat4 projection = cam->projectionTransform(width, height);
	glm::mat4 v_inv = glm::inverse(view);

	program->set("v", view);
	program->set("p", projection);
	program->set("v_inv", v_inv);

	//std::cerr << "got here" << std::endl;

	for (auto& [transform, inverted, mesh] : meshes) {
		glm::mat3 m_3x3_inv_transp =
			glm::transpose(glm::inverse(model_to_world(transform)));

		program->set("m", transform);
		program->set("m_3x3_inv_transp", m_3x3_inv_transp);

		gameModel::ptr mod = std::dynamic_pointer_cast<gameModel>(mesh->parent);
		assert(mod->compiled);
		set_material(program, mod->comped_model, mesh->material);

		// TODO: need to keep track of the model face order
		set_face_order(inverted? GL_CW : GL_CCW);

		auto& cmesh = mesh->comped_mesh;
		bind_vao(cmesh->vao);
		glDrawElements(GL_TRIANGLES, cmesh->elements_size,
		               GL_UNSIGNED_INT, cmesh->elements_offset);
	}

	meshes.clear();
	lights.clear();
	probes.clear();
}

void renderQueue::flush(renderFramebuffer::ptr fb,
                        Program::ptr program,
                        renderAtlases& atlases)
{
	fb->framebuffer->bind();
	fb->clear();
	program->bind();
	shaderSync(program, atlases);

	glViewport(0, 0, fb->width, fb->height);
	glScissor(0, 0, fb->width, fb->height);
	glFrontFace(GL_CCW);
	glClearColor(0.1, 0.1, 0.1, 1);
	//glClearColor(1.0, 0.1, 0.1, 1);
	glClearStencil(0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	disable(GL_SCISSOR_TEST);
	enable(GL_DEPTH_TEST);
	enable(GL_STENCIL_TEST);
	glDepthMask(GL_TRUE);
	glDepthFunc(GL_LESS);
	glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

	glm::mat4 view = cam->viewTransform();
	glm::mat4 projection = cam->projectionTransform(fb->width, fb->height);
	glm::mat4 v_inv = glm::inverse(view);

	program->set("v", view);
	program->set("p", projection);
	program->set("v_inv", v_inv);

	//std::cerr << "got here" << std::endl;

	for (auto& [transform, inverted, mesh] : meshes) {
		if (fb->drawn_meshes.size() < 0xff) {
			fb->drawn_meshes.push_back(mesh);
			glStencilFunc(GL_ALWAYS, fb->drawn_meshes.size(), ~0);

		} else {
			glStencilFunc(GL_ALWAYS, 0, ~0);
		}

		//std::cerr << "also got here" << std::endl;

		glm::mat3 m_3x3_inv_transp =
			glm::transpose(glm::inverse(model_to_world(transform)));

		program->set("m", transform);
		program->set("m_3x3_inv_transp", m_3x3_inv_transp);

		glm::vec4 apos = transform * glm::vec4(1);
		set_reflection_probe(nearest_reflection_probe(glm::vec3(apos)/apos.w),
		                     program, atlases);

		gameModel::ptr mod = std::dynamic_pointer_cast<gameModel>(mesh->parent);
		assert(mod->compiled);
		set_material(program, mod->comped_model, mesh->material);

		// TODO: need to keep track of the model face order
		set_face_order(inverted? GL_CW : GL_CCW);

		auto& cmesh = mesh->comped_mesh;
		bind_vao(cmesh->vao);
		glDrawElements(GL_TRIANGLES, cmesh->elements_size,
		               GL_UNSIGNED_INT, cmesh->elements_offset);
	}

	meshes.clear();
	lights.clear();
	probes.clear();
}

void renderQueue::shaderSync(Program::ptr program, renderAtlases& atlases) {
	std::vector<gameLightPoint::ptr> point_lights;
	// TODO: other lights

	for (auto& [trans, _, light] : lights) {
		switch (light->lightType) {
			case gameLight::lightTypes::Point:
				{
					gameLightPoint::ptr plit =
						std::dynamic_pointer_cast<gameLightPoint>(light);
					point_lights.push_back(plit);
				}
				break;

			default:
				break;
		}
	}

	size_t active = min(MAX_LIGHTS, point_lights.size());

	program->set("active_point_lights", (GLint)active);

	for (size_t i = 0; i < active; i++) {
		//std::cerr << "shaderSync(): setting point light " << i << std::endl;
		// TODO: check light index
		gameLightPoint::ptr light = point_lights[i];

		// TODO: also updating all these uniforms can't be very efficient for a lot of
		//       moving point lights... maybe look into how uniform buffer objects work
		std::string locstr = "point_lights[" + std::to_string(i) + "]";

		program->set(locstr + ".position",      light->position);
		program->set(locstr + ".diffuse",       light->diffuse);
		program->set(locstr + ".radius",        light->radius);
		program->set(locstr + ".intensity",     light->intensity);
		program->set(locstr + ".casts_shadows", light->casts_shadows);

		if (light->casts_shadows) {
			for (unsigned k = 0; k < 6; k++) {
				std::string sloc = locstr + ".shadowmap[" + std::to_string(k) + "]";
				auto vec = atlases.shadows->tex_vector(light->shadowmap[k]);
				program->set(sloc, vec);
			}
		}
	}
}

gameReflectionProbe::ptr renderQueue::nearest_reflection_probe(glm::vec3 pos) {
	gameReflectionProbe::ptr ret = nullptr;
	float mindist = HUGE_VALF;

	// TODO: optimize this, O(N) is meh
	for (auto& [_, __, p] : probes) {
		float dist = glm::distance(pos, p->position);
		if (!ret || dist < mindist) {
			mindist = dist;
			ret = p;
		}
	}

	return ret;
}

void renderQueue::set_reflection_probe(gameReflectionProbe::ptr probe,
                                       Program::ptr program,
                                       renderAtlases& atlases)
{
	if (!probe) {
		return;
	}

	for (unsigned i = 0; i < 6; i++) {
		std::string sloc = "reflection_probe[" + std::to_string(i) + "]";
		glm::vec3 facevec = atlases.reflections->tex_vector(probe->faces[i]); 
		program->set(sloc, facevec);
		DO_ERROR_CHECK();
	}
}
