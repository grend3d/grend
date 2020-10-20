#include <grend/engine.hpp>
#include <grend/utility.hpp>
#include <grend/texture-atlas.hpp>
#include <math.h>

using namespace grendx;

void renderQueue::add(gameObject::ptr obj,
                      float animTime,
                      glm::mat4 trans,
                      bool inverted)
{
	if (obj == nullptr || !obj->visible) {
		return;
	}

	glm::mat4 adjTrans = trans*obj->getTransform(animTime);
	//glm::mat4 adjTrans = obj->getTransform();

	//std::cerr << "add(): push" << std::endl;
	
	unsigned invcount = 0;
	for (unsigned i = 0; i < 3; i++)
		invcount += obj->transform.scale[i] < 0;

	// only want to invert face order if flipped an odd number of times
	if (invcount&1) {
		inverted = !inverted;
	}

	if (obj->type == gameObject::objType::Mesh) {
		gameMesh::ptr mesh = std::dynamic_pointer_cast<gameMesh>(obj);
		meshes.push_back({adjTrans, inverted, mesh});

	} else if (obj->type == gameObject::objType::Light) {
		gameLight::ptr light = std::dynamic_pointer_cast<gameLight>(obj);
		lights.push_back({adjTrans, inverted, light});

	} else if (obj->type == gameObject::objType::ReflectionProbe) {
		gameReflectionProbe::ptr probe =
			std::dynamic_pointer_cast<gameReflectionProbe>(obj);
		probes.push_back({adjTrans, inverted, probe});
	}

	if (obj->type == gameObject::objType::None
	    && obj->hasNode("skin")
	    && obj->hasNode("mesh"))
	{
		auto s = std::dynamic_pointer_cast<gameSkin>(obj->getNode("skin"));
		addSkinned(obj->getNode("mesh"), s, animTime, adjTrans, inverted);

	} else {
		for (auto& [name, ptr] : obj->nodes) {
			//std::cerr << "add(): subnode " << name << std::endl;
			add(ptr, animTime, adjTrans, inverted);
		}
	}

	//std::cerr << "add(): pop" << std::endl;
}

void renderQueue::addSkinned(gameObject::ptr obj,
                             gameSkin::ptr skin,
                             float animTime,
                             glm::mat4 trans,
                             bool inverted)
{
	if (obj == nullptr || !obj->visible) {
		return;
	}

	if (obj->type == gameObject::objType::Mesh) {
		auto m = std::dynamic_pointer_cast<gameMesh>(obj);
		skinnedMeshes[skin].push_back({trans, inverted, m});
	}

	for (auto& [name, ptr] : obj->nodes) {
		addSkinned(ptr, skin, animTime, trans, inverted);
	}
}

void renderQueue::updateLights(Program::ptr program, renderContext::ptr rctx) {
	// TODO: should probably apply the transform to light position
	for (auto& [_, __, light] : lights) {
		if (light->lightType == gameLight::lightTypes::Point) {
			// TODO: check against view frustum to see if this light is visible,
			//       avoid rendering shadow maps if not
			gameLightPoint::ptr plit =
				std::dynamic_pointer_cast<gameLightPoint>(light);

			auto& shatree = rctx->atlases.shadows->tree;
			for (unsigned i = 0; i < 6; i++) {
				if (!shatree.valid(plit->shadowmap[i])) {
					// TODO: configurable size
					plit->shadowmap[i] = shatree.alloc(256);
				}
			}

			drawShadowCubeMap(*this, plit, rctx);

		} else if (light->lightType == gameLight::lightTypes::Spot) {
			// TODO: shadow map

		} else if (light->lightType == gameLight::lightTypes::Directional) {
			// TODO: orthoganal shadow map
		}
	}
}

void
renderQueue::updateReflections(Program::ptr program, renderContext::ptr rctx) {
	for (auto& [_, __, probe] : probes) {
		auto& reftree = rctx->atlases.reflections->tree;
		for (unsigned i = 0; i < 6; i++) {
			if (!reftree.valid(probe->faces[i])) {
				// TODO: configurable size
				probe->faces[i] = reftree.alloc(256);
			}
		}

		drawReflectionProbe(*this, probe, rctx);
	}
}

void renderQueue::sort(void) {
	typedef std::tuple<glm::mat4, bool, gameMesh::ptr> draw_ent;

	// TODO: better way to do this, in-place?
	std::vector<std::tuple<glm::mat4, bool, gameMesh::ptr>> transparent;
	std::vector<std::tuple<glm::mat4, bool, gameMesh::ptr>> opaque;

	std::sort(meshes.begin(), meshes.end(),
		[&] (draw_ent& a, draw_ent& b) {
			// TODO: why not just define a struct
			auto& [a_trans, _,  a_mesh] = a;
			auto& [b_trans, __, b_mesh] = b;

			glm::vec4 ta = a_trans*glm::vec4(1);
			glm::vec4 tb = b_trans*glm::vec4(1);
			glm::vec3 va = glm::vec3(ta)/ta.w;
			glm::vec3 vb = glm::vec3(tb)/tb.w;

			return glm::distance(cam->position(), va)
				< glm::distance(cam->position(), vb);
		});

	for (auto& it : meshes) {
		auto [transform, inverted, mesh] = it;

		gameModel::ptr mod = std::dynamic_pointer_cast<gameModel>(mesh->parent);
		auto& mat = mod->materials[mesh->material];

		if (mat.blend != material::blend_mode::Opaque) {
			transparent.push_back(it);
		} else {
			opaque.push_back(it);
		}
	}

	std::reverse(transparent.begin(), transparent.end());

	// XXX: TODO: avoid clearing, do whole thing in-place
	meshes.clear();
	meshes.insert(meshes.end(), opaque.begin(), opaque.end());
	meshes.insert(meshes.end(), transparent.begin(), transparent.end());
}

void renderQueue::cull(unsigned width, unsigned height) {
	glm::mat4 view = cam->viewTransform();

	// XXX: need vector for sorting, deleting elements from vector for culling
	//      would make this quadratic in the worst case... copying isn't
	//      ideal, but should be fast enough, still linear time complexity
	std::vector<std::tuple<glm::mat4, bool, gameMesh::ptr>> tempMeshes;

	for (auto it = meshes.begin(); it != meshes.end(); it++) {
		auto& [trans, _, mesh] = *it;
		auto obb = trans * mesh->boundingBox;

		if (cam->boxInFrustum(obb)) {
			tempMeshes.push_back(*it);
		}
	}

	meshes = tempMeshes;

	for (auto& [skin, skmeshes] : skinnedMeshes) {
		std::vector<std::tuple<glm::mat4, bool, gameMesh::ptr>> tempSkinned;

		for (auto it = skmeshes.begin(); it != skmeshes.end(); it++) {
			auto& [trans, _, mesh] = *it;
			auto obb = (trans) * mesh->boundingBox;

			if (cam->boxInFrustum(obb)) {
				tempSkinned.push_back(*it);
			}
		}

		skmeshes = tempSkinned;
	}

	std::vector<std::tuple<glm::mat4, bool, gameLight::ptr>> tempLights;
	for (auto it = lights.begin(); it != lights.end(); it++) {
		auto& [trans, _, lit] = *it;

		glm::vec4 temp = (trans)*glm::vec4(0, 0, 0, 1);
		glm::vec3 pos = glm::vec3(temp) / temp.w;

		// conservative culling, keeps any lights that may possibly affect
		// what's in view, without considering direction of spotlights, shadows
		if (cam->sphereInFrustum(pos, lit->extent())) {
			tempLights.push_back(*it);
		}
	}

	lights = tempLights;
}

static void drawMesh(renderFlags& flags,
                     renderFramebuffer::ptr fb,
                     Program::ptr program,
                     glm::mat4& transform,
                     bool inverted,
                     gameMesh::ptr mesh)
{
	if (fb != nullptr && flags.stencil) {
		if (fb->drawn_meshes.size() < 0xff) {
			fb->drawn_meshes.push_back(mesh);
			glStencilFunc(GL_ALWAYS, fb->drawn_meshes.size(), ~0);

		} else {
			glStencilFunc(GL_ALWAYS, 0, ~0);
		}
	}

	glm::mat3 m_3x3_inv_transp =
		glm::transpose(glm::inverse(model_to_world(transform)));

	program->set("m", transform);
	program->set("m_3x3_inv_transp", m_3x3_inv_transp);

	gameModel::ptr mod = std::dynamic_pointer_cast<gameModel>(mesh->parent);
	assert(mod->compiled);
	set_material(program, mod->comped_model, mesh->material);
	auto& mat = mod->materials[mesh->material];

	// enable()/disable() cache state, no need to worry about toggling
	// too much state if queue is sorted
	if (mat.blend != material::blend_mode::Opaque) {
		// TODO: handle mask blends
		enable(GL_BLEND);
	} else {
		disable(GL_BLEND);
	}

	// TODO: need to keep track of the model face order
	if (flags.cull_faces) {
		set_face_order(inverted? GL_CW : GL_CCW);
	}

	auto& cmesh = mesh->comped_mesh;
	bind_vao(cmesh->vao);
	glDrawElements(GL_TRIANGLES, cmesh->elements_size,
				   GL_UNSIGNED_INT, cmesh->elements_offset);
}

unsigned renderQueue::flush(unsigned width,
                            unsigned height,
                            renderContext::ptr rctx)
{
	renderFlags flags = rctx->getFlags();
	unsigned drawnMeshes = 0;

	if (flags.sort)       { sort(); }
	if (flags.cull_faces) { enable(GL_CULL_FACE); }

	cam->setViewport(width, height);
	glm::mat4 view = cam->viewTransform();
	glm::mat4 projection = cam->projectionTransform();
	glm::mat4 v_inv = glm::inverse(view);

	flags.mainShader->bind();
	flags.mainShader->set("v", view);
	flags.mainShader->set("p", projection);
	flags.mainShader->set("v_inv", v_inv);

	for (auto& [skin, drawinfo] : skinnedMeshes) {
		float offset = -1.0;

		for (auto& [transform, inverted, mesh] : drawinfo) {
			// TODO: have time offset field for animated meshes
			if (offset != 0.0) {
				for (unsigned i = 0; i < skin->inverseBind.size(); i++) {
					float tim = SDL_GetTicks()/1000.f;
					gameObject::ptr temp = skin->joints[i];
					glm::mat4 accum = glm::mat4(1);

					for (; temp != skin && temp; temp = temp->parent) {
						accum = temp->getTransform(tim)*accum;
					}

					std::string sloc = "joints["+std::to_string(i)+"]";
					auto mat = accum*skin->inverseBind[i];
					flags.skinnedShader->set(sloc, mat);
				}

				// TODO: s/0.0/current offset
				offset = 0.0;
			}

			glm::vec4 apos = transform * glm::vec4(1);
			set_reflection_probe(
				nearest_reflection_probe(glm::vec3(apos)/apos.w),
				flags.skinnedShader,
				rctx->atlases);
			drawMesh(flags, nullptr, flags.skinnedShader,
			         transform, inverted, mesh);
		}

		// TODO: check whether this skin is already synced
		DO_ERROR_CHECK();
	}

	for (auto& [transform, inverted, mesh] : meshes) {
		glm::vec4 apos = transform * glm::vec4(1);
		set_reflection_probe(
			nearest_reflection_probe(glm::vec3(apos)/apos.w),
			flags.mainShader, rctx->atlases);

		drawMesh(flags, nullptr, flags.mainShader, transform, inverted, mesh);
		drawnMeshes++;
	}

	meshes.clear();
	lights.clear();
	probes.clear();
	skinnedMeshes.clear();

	return drawnMeshes;
}

unsigned renderQueue::flush(renderFramebuffer::ptr fb, renderContext::ptr rctx) {
	unsigned drawnMeshes = 0;
	renderFlags flags = rctx->getFlags();

	if (flags.sort) { sort(); }

	if (flags.cull_faces) {
		enable(GL_CULL_FACE);
		glFrontFace(GL_CCW);
	}

	if (flags.stencil) {
		enable(GL_STENCIL_TEST);
		glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
	}

	if (flags.depth) {
		enable(GL_DEPTH_TEST);
		glDepthMask(GL_TRUE);
		glDepthFunc(GL_LESS);
	}

	assert(fb != nullptr);
	fb->framebuffer->bind();

	disable(GL_SCISSOR_TEST);
	glViewport(0, 0, fb->width, fb->height);
	glScissor(0, 0, fb->width, fb->height);

	cam->setViewport(fb->width, fb->height);
	glm::mat4 view = cam->viewTransform();
	glm::mat4 projection = cam->projectionTransform();
	glm::mat4 v_inv = glm::inverse(view);

	flags.skinnedShader->bind();
	flags.skinnedShader->set("v", view);
	flags.skinnedShader->set("p", projection);
	flags.skinnedShader->set("v_inv", v_inv);
	flags.skinnedShader->set("cameraPosition", cam->position());
	shaderSync(flags.skinnedShader, rctx);
	gameSkin::ptr lastskin = nullptr;

	for (auto& [skin, drawinfo] : skinnedMeshes) {
		float offset = -1.0;

		for (auto& [transform, inverted, mesh] : drawinfo) {
			// TODO: have time offset field for animated meshes
			if (offset != 0.0) {
				for (unsigned i = 0; i < skin->inverseBind.size(); i++) {
					float tim = SDL_GetTicks()/1000.f;
					gameObject::ptr temp = skin->joints[i];
					glm::mat4 accum = glm::mat4(1);

					for (; temp != skin && temp; temp = temp->parent) {
						accum = temp->getTransform(tim)*accum;
					}

					std::string sloc = "joints["+std::to_string(i)+"]";
					auto mat = accum*skin->inverseBind[i];
					flags.skinnedShader->set(sloc, mat);
				}

				// TODO: s/0.0/current offset
				offset = 0.0;
			}

			glm::vec4 apos = transform * glm::vec4(1);
			set_reflection_probe(
				nearest_reflection_probe(glm::vec3(apos)/apos.w),
				flags.skinnedShader,
				rctx->atlases);
			drawMesh(flags, fb, flags.skinnedShader, transform, inverted, mesh);
			drawnMeshes++;
		}

		// TODO: check whether this skin is already synced
		DO_ERROR_CHECK();
	}

	flags.mainShader->bind();
	flags.mainShader->set("v", view);
	flags.mainShader->set("p", projection);
	flags.mainShader->set("v_inv", v_inv);
	shaderSync(flags.mainShader, rctx);

	for (auto& [transform, inverted, mesh] : meshes) {
		glm::vec4 apos = transform * glm::vec4(1);
		set_reflection_probe(
			nearest_reflection_probe(glm::vec3(apos)/apos.w),
			flags.mainShader, rctx->atlases);
		drawMesh(flags, fb, flags.mainShader, transform, inverted, mesh);
		drawnMeshes++;
	}

	// TODO: clear()
	meshes.clear();
	lights.clear();
	probes.clear();
	skinnedMeshes.clear();

	return drawnMeshes;
}

static void syncPlainUniforms(Program::ptr program,
	                          renderContext::ptr rctx,
	                          std::vector<gameLightPoint::ptr>& points,
	                          std::vector<gameLightSpot::ptr>& spots,
	                          std::vector<gameLightDirectional::ptr>& directionals)
{
	size_t pactive = min(MAX_LIGHTS, points.size());
	size_t sactive = min(MAX_LIGHTS, spots.size());
	size_t dactive = min(MAX_LIGHTS, directionals.size());

	program->set("active_points",       (GLint)pactive);
	program->set("active_spots",        (GLint)sactive);
	program->set("active_directionals", (GLint)dactive);

	for (size_t i = 0; i < pactive; i++) {
		gameLightPoint::ptr light = points[i];

		// TODO: also updating all these uniforms can't be very efficient for a lot of
		//       moving point lights... maybe look into how uniform buffer objects work
		std::string locstr = "points[" + std::to_string(i) + "]";

		program->set(locstr + ".position",      light->transform.position);
		program->set(locstr + ".diffuse",       light->diffuse);
		program->set(locstr + ".intensity",     light->intensity);
		program->set(locstr + ".radius",        light->radius);
		program->set(locstr + ".casts_shadows", light->casts_shadows);

		if (light->casts_shadows) {
			for (unsigned k = 0; k < 6; k++) {
				std::string sloc = locstr + ".shadowmap[" + std::to_string(k) + "]";
				auto vec = rctx->atlases.shadows->tex_vector(light->shadowmap[k]);
				program->set(sloc, vec);
			}
		}
	}

	for (size_t i = 0; i < sactive; i++) {
		gameLightSpot::ptr light = spots[i];

		// TODO: also updating all these uniforms can't be very efficient for a lot of
		//       moving point lights... maybe look into how uniform buffer objects work
		std::string locstr = "spots[" + std::to_string(i) + "]";
		// light points toward +X by default
		glm::vec3 rotvec =
			glm::mat3_cast(light->transform.rotation) * glm::vec3(1, 0, 0);

		program->set(locstr + ".position",      light->transform.position);
		program->set(locstr + ".diffuse",       light->diffuse);
		program->set(locstr + ".direction",     rotvec);
		program->set(locstr + ".intensity",     light->intensity);
		program->set(locstr + ".radius",        light->radius);
		program->set(locstr + ".angle",         light->angle);
		program->set(locstr + ".casts_shadows", light->casts_shadows);

		if (light->casts_shadows) {
			std::string sloc = locstr + ".shadowmap";
			auto vec = rctx->atlases.shadows->tex_vector(light->shadowmap);
			program->set(sloc, vec);
		}
	}

	for (size_t i = 0; i < dactive; i++) {
		gameLightDirectional::ptr light = directionals[i];

		// TODO: also updating all these uniforms can't be very efficient for a lot of
		//       moving point lights... maybe look into how uniform buffer objects work
		std::string locstr = "directionals[" + std::to_string(i) + "]";
		// light points toward +X by default
		glm::vec3 rotvec =
			glm::mat3_cast(light->transform.rotation) * glm::vec3(1, 0, 0);

		// position is ignored in the shader, but might as well set it anyway
		// TODO: probably remove position member from directional lights
		program->set(locstr + ".position",      light->transform.position);
		program->set(locstr + ".diffuse",       light->diffuse);
		program->set(locstr + ".direction",     rotvec);
		program->set(locstr + ".intensity",     light->intensity);
		program->set(locstr + ".casts_shadows", light->casts_shadows);

		if (light->casts_shadows) {
			std::string sloc = locstr + ".shadowmap";
			auto vec = rctx->atlases.shadows->tex_vector(light->shadowmap);
			program->set(sloc, vec);
		}
	}
}


static void syncUniformBuffer(Program::ptr program,
	                          renderContext::ptr rctx,
	                          std::vector<gameLightPoint::ptr>& points,
	                          std::vector<gameLightSpot::ptr>& spots,
	                          std::vector<gameLightDirectional::ptr>& directionals)
{
	size_t pactive = min(MAX_LIGHTS, points.size());
	size_t sactive = min(MAX_LIGHTS, spots.size());
	size_t dactive = min(MAX_LIGHTS, directionals.size());

	// TODO: keep copy of lightbuf around...
	lights_std140 lightbuf;

	lightbuf.uactive_point_lights       = pactive;
	lightbuf.uactive_spot_lights        = sactive;
	lightbuf.uactive_directional_lights = dactive;

	// so much nicer
	for (size_t i = 0; i < pactive; i++) {
		gameLightPoint::ptr light = points[i];
		packLight(light, lightbuf.upoint_lights + i, rctx);
	}

	for (size_t i = 0; i < sactive; i++) {
		gameLightSpot::ptr light = spots[i];
		packLight(light, lightbuf.uspot_lights + i, rctx);
	}

	for (size_t i = 0; i < dactive; i++) {
		gameLightDirectional::ptr light = directionals[i];
		packLight(light, lightbuf.udirectional_lights + i, rctx);
	}


	rctx->lightBuffer->update(&lightbuf, 0, sizeof(lightbuf));
	//glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(lights_std140), &lightbuf);
	//rctx->lightBuffer->unmap();
}

void renderQueue::shaderSync(Program::ptr program, renderContext::ptr rctx) {
	std::vector<gameLightPoint::ptr>       point_lights;
	std::vector<gameLightSpot::ptr>        spot_lights;
	std::vector<gameLightDirectional::ptr> directional_lights;

	for (auto& [trans, _, light] : lights) {
		switch (light->lightType) {
			case gameLight::lightTypes::Point:
				{
					gameLightPoint::ptr plit =
						std::dynamic_pointer_cast<gameLightPoint>(light);
					point_lights.push_back(plit);
				}
				break;

			case gameLight::lightTypes::Spot:
				{
					gameLightSpot::ptr slit =
						std::dynamic_pointer_cast<gameLightSpot>(light);
					spot_lights.push_back(slit);
				}
				break;

			case gameLight::lightTypes::Directional:
				{
					gameLightDirectional::ptr dlit =
						std::dynamic_pointer_cast<gameLightDirectional>(light);
					directional_lights.push_back(dlit);
				}
				break;

			default:
				break;
		}
	}

#if GLSL_VERSION >= 140
	program->setUniformBlock("lights", rctx->lightBuffer);
	syncUniformBuffer(program, rctx, point_lights,
	                  spot_lights, directional_lights);
	DO_ERROR_CHECK();

#else
	syncPlainUniforms(program, rctx, point_lights,
	                  spot_lights, directional_lights);
	DO_ERROR_CHECK();
#endif
}

gameReflectionProbe::ptr renderQueue::nearest_reflection_probe(glm::vec3 pos) {
	gameReflectionProbe::ptr ret = nullptr;
	float mindist = HUGE_VALF;

	// TODO: optimize this, O(N) is meh
	for (auto& [_, __, p] : probes) {
		float dist = glm::distance(pos, p->transform.position);
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

	program->set("refboxMin",        probe->transform.position + probe->boundingBox.min);
	program->set("refboxMax",        probe->transform.position + probe->boundingBox.max);
	program->set("refprobePosition", probe->transform.position);
}
