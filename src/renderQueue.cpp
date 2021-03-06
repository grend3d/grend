#include <grend/engine.hpp>
#include <grend/utility.hpp>
#include <grend/textureAtlas.hpp>
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

	glm::mat4 adjTrans =
		// default transform is identity, so no need to waste
		// cycles on the matrix multiply
		obj->hasDefaultTransform()
			? trans
			: trans*obj->getTransformMatrix(animTime);

	unsigned invcount = 0;
	for (unsigned i = 0; i < 3; i++)
		invcount += obj->getTransformTRS().scale[i] < 0;

	// only want to invert face order if flipped an odd number of times
	if (invcount&1) {
		inverted = !inverted;
	}

	if (obj->type == gameObject::objType::Mesh) {
		gameMesh::ptr mesh = std::dynamic_pointer_cast<gameMesh>(obj);
		glm::vec3 center = applyTransform(adjTrans);
		meshes.push_back({adjTrans, center, inverted, mesh});

	} else if (obj->type == gameObject::objType::Light) {
		gameLight::ptr light = std::dynamic_pointer_cast<gameLight>(obj);
		glm::vec3 center = applyTransform(adjTrans);
		lights.push_back({adjTrans, center, inverted, light});

	} else if (obj->type == gameObject::objType::ReflectionProbe) {
		gameReflectionProbe::ptr probe =
			std::dynamic_pointer_cast<gameReflectionProbe>(obj);
		glm::vec3 center = applyTransform(adjTrans);
		probes.push_back({adjTrans, center, inverted, probe});

	} else if (obj->type == gameObject::objType::IrradianceProbe) {
		gameIrradianceProbe::ptr probe =
			std::dynamic_pointer_cast<gameIrradianceProbe>(obj);
		glm::vec3 center = applyTransform(adjTrans);
		irradProbes.push_back({adjTrans, center, inverted, probe});
	}

	if (obj->type == gameObject::objType::None
	    && obj->hasNode("skin")
	    && obj->hasNode("mesh"))
	{
		auto s = std::dynamic_pointer_cast<gameSkin>(obj->getNode("skin"));
		addSkinned(obj->getNode("mesh"), s, animTime, adjTrans, inverted);

	} else if (obj->type == gameObject::objType::Particles) {
		auto p = std::dynamic_pointer_cast<gameParticles>(obj);
		addInstanced(obj, p, adjTrans, inverted);

	} else if (obj->type == gameObject::objType::BillboardParticles) {
		auto p = std::dynamic_pointer_cast<gameBillboardParticles>(obj);
		addBillboards(obj, p, adjTrans, inverted);

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
		glm::vec3 center = applyTransform(trans, boxCenter(m->boundingBox));
		skinnedMeshes[skin].push_back({trans, center, inverted, m});
	}

	for (auto& [name, ptr] : obj->nodes) {
		addSkinned(ptr, skin, animTime, trans, inverted);
	}
}

void renderQueue::addInstanced(gameObject::ptr obj,
                               gameParticles::ptr particles,
                               glm::mat4 trans,
                               bool inverted)
{
	if (obj == nullptr || !obj->visible) {
		return;
	}

	glm::mat4 adjTrans = trans*obj->getTransformMatrix();

	unsigned invcount = 0;
	for (unsigned i = 0; i < 3; i++)
		invcount += obj->getTransformTRS().scale[i] < 0;

	// only want to invert face order if flipped an odd number of times
	inverted ^= invcount & 1;

	if (obj->type == gameObject::objType::Mesh) {
		auto m = std::dynamic_pointer_cast<gameMesh>(obj);
		instancedMeshes.push_back({adjTrans, inverted, particles, m});
	}

	for (auto& [name, ptr] : obj->nodes) {
		addInstanced(ptr, particles, adjTrans, inverted);
	}
}

void renderQueue::addBillboards(gameObject::ptr obj,
                                gameBillboardParticles::ptr particles,
                                glm::mat4 trans,
                                bool inverted)
{
	if (obj == nullptr || !obj->visible) {
		return;
	}

	glm::mat4 adjTrans = trans*obj->getTransformMatrix();

	unsigned invcount = 0;
	for (unsigned i = 0; i < 3; i++)
		invcount += obj->getTransformTRS().scale[i] < 0;

	// only want to invert face order if flipped an odd number of times
	inverted ^= invcount & 1;

	if (obj->type == gameObject::objType::Mesh) {
		auto m = std::dynamic_pointer_cast<gameMesh>(obj);
		billboardMeshes.push_back({adjTrans, inverted, particles, m});
	}

	for (auto& [name, ptr] : obj->nodes) {
		addBillboards(ptr, particles, adjTrans, inverted);
	}
}

void renderQueue::updateLights(renderContext::ptr rctx) {
	// TODO: should probably apply the transform to light position
	for (auto& light : lights) {
		if (!light.data->casts_shadows) {
			continue;
		}

		if (light.data->lightType == gameLight::lightTypes::Point) {
			// TODO: check against view frustum to see if this light is visible,
			//       avoid rendering shadow maps if not
			gameLightPoint::ptr plit =
				std::dynamic_pointer_cast<gameLightPoint>(light.data);

			auto& shatree = rctx->atlases.shadows->tree;
			for (unsigned i = 0; i < 6; i++) {
				if (!shatree.valid(plit->shadowmap[i])) {
					// TODO: configurable size
					plit->shadowmap[i] = shatree.alloc(rctx->settings.shadowSize);
				}
			}

			drawShadowCubeMap(*this, plit, light.transform, rctx);

		} else if (light.data->lightType == gameLight::lightTypes::Spot) {
			gameLightSpot::ptr slit =
				std::dynamic_pointer_cast<gameLightSpot>(light.data);

			auto& shatree = rctx->atlases.shadows->tree;
			if (!shatree.valid(slit->shadowmap)) {
				// TODO: configurable size
				slit->shadowmap = shatree.alloc(rctx->settings.shadowSize);
			}

			drawSpotlightShadow(*this, slit, light.transform, rctx);

		} else if (light.data->lightType == gameLight::lightTypes::Directional) {
			// TODO: orthoganal shadow map
		}
	}
}

void
renderQueue::updateReflections(renderContext::ptr rctx) {
	auto& reftree = rctx->atlases.reflections->tree;
	auto& radtree = rctx->atlases.irradiance->tree;

	for (auto& probe : probes) {
		// allocate from reflection atlas for top level reflections
		for (unsigned i = 0; i < 6; i++) {
			if (!reftree.valid(probe.data->faces[0][i])) {
				// TODO: configurable size
				probe.data->faces[0][i] = reftree.alloc(rctx->settings.reflectionSize);
			}
		}

		// allocate from irradiance atlas for lower level (blurrier) reflections
		for (unsigned k = 1; k < 5; k++) {
			for (unsigned i = 0; i < 6; i++) {
				unsigned adj = rctx->settings.reflectionSize >> k;
				adj = adj? adj : 1;

				if (!radtree.valid(probe.data->faces[k][i])) {
					// TODO: configurable size
					probe.data->faces[k][i] = radtree.alloc(adj);
				}
			}
		}

		probe.data->have_convolved = true;
		drawReflectionProbe(*this, probe.data, probe.transform, rctx);
	}

	for (auto& radprobe : irradProbes) {
		for (unsigned i = 0; i < 6; i++) {
			// TODO: configurable size
			if (!radtree.valid(radprobe.data->faces[i])) {
				radprobe.data->faces[i] = radtree.alloc(rctx->settings.lightProbeSize);
			}

			// TODO: coefficients

			if (!reftree.valid(radprobe.data->source->faces[0][i])) {
				radprobe.data->source->faces[0][i] =
					reftree.alloc(rctx->settings.lightProbeSize*4);
			}
		}

		radprobe.data->source->have_convolved = false;
		drawIrradianceProbe(*this, radprobe.data, radprobe.transform, rctx);
	}
}

void renderQueue::sort(void) {
	//typedef std::tuple<glm::mat4, bool, gameMesh::ptr>  draw_ent;
	//typedef std::tuple<glm::mat4, bool, gameLight::ptr> light_ent;

	// TODO: better way to do this, in-place?
	std::vector<queueEnt<gameMesh::ptr>> transparent;
	std::vector<queueEnt<gameMesh::ptr>> opaque;

	std::sort(lights.begin(), lights.end(),
		[&] (queueEnt<gameLight::ptr>& a, queueEnt<gameLight::ptr>& b) {
			return glm::distance(cam->position(), a.center)
			     < glm::distance(cam->position(), b.center);
		});

	std::sort(meshes.begin(), meshes.end(),
		[&] (queueEnt<gameMesh::ptr>& a, queueEnt<gameMesh::ptr>& b) {
			return glm::distance(cam->position(), a.center)
			     < glm::distance(cam->position(), b.center);
		});

	for (auto& it : meshes) {
		//auto [transform, inverted, mesh] = it;

		if (it.data->comped_mesh
		    && it.data->comped_mesh->blend != material::blend_mode::Opaque)
		{
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

void renderQueue::cull(unsigned width, unsigned height, float lightext) {
	glm::mat4 view = cam->viewTransform();

	// XXX: need vector for sorting, deleting elements from vector for culling
	//      would make this quadratic in the worst case... copying isn't
	//      ideal, but should be fast enough, still linear time complexity
	//std::vector<std::tuple<glm::mat4, bool, gameMesh::ptr>> tempMeshes;
	std::vector<queueEnt<gameMesh::ptr>> tempMeshes;

	for (auto it = meshes.begin(); it != meshes.end(); it++) {
		auto& [trans, _, __, mesh] = *it;
		auto obb = trans * mesh->boundingBox;

		if (cam->boxInFrustum(obb)) {
			tempMeshes.push_back(*it);
		}
	}

	meshes = tempMeshes;

	for (auto& [skin, skmeshes] : skinnedMeshes) {
		//std::vector<std::tuple<glm::mat4, bool, gameMesh::ptr>> tempSkinned;
		std::vector<queueEnt<gameMesh::ptr>> tempSkinned;

		for (auto it = skmeshes.begin(); it != skmeshes.end(); it++) {
			auto& [trans, _, __, mesh] = *it;
			auto obb = (trans) * mesh->boundingBox;

			if (cam->boxInFrustum(obb)) {
				tempSkinned.push_back(*it);
			}
		}

		skmeshes = tempSkinned;
	}

	//std::vector<std::tuple<glm::mat4, bool, gameLight::ptr>> tempLights;
	std::vector<queueEnt<gameLight::ptr>> tempLights;
	for (auto it = lights.begin(); it != lights.end(); it++) {
		auto& [trans, _, __, lit] = *it;

		// conservative culling, keeps any lights that may possibly affect
		// what's in view, without considering direction of spotlights, shadows
		if (cam->sphereInFrustum(applyTransform(trans), lit->extent(lightext))) {
			tempLights.push_back(*it);
		}
	}
	lights = tempLights;

	std::vector<std::tuple<glm::mat4, bool, gameParticles::ptr,
						   gameMesh::ptr>> tempInstanced;
	for (auto it = instancedMeshes.begin(); it != instancedMeshes.end(); it++) {
		auto& [trans, _, particles, __] = *it;

		if (cam->sphereInFrustum(applyTransform(trans), particles->radius)) {
			tempInstanced.push_back(*it);
		}
	}
	instancedMeshes = tempInstanced;
}

void renderQueue::batch(void) {
	std::map<gameMesh*, unsigned> meshCount;
	bool canBatch = false;
	// TODO: configuration option
	unsigned minbatch = 16;

	for (auto& [_, __, ___, mesh] : meshes) {
		canBatch |= ++meshCount[mesh.get()] >= minbatch;
	}

	if (!canBatch) {
		// no meshes meet minimum batch requirement, would be slower
		// (in principle) to batch anyway
		return;
	}

	std::map<gameMesh::ptr, gameParticles::ptr> batches;
	std::vector<queueEnt<gameMesh::ptr>> tempMeshes;

	for (auto it = meshes.begin(); it != meshes.end(); it++) {
		auto& [trans, __, ___, mesh] = *it;

		if (meshCount[mesh.get()] < minbatch) {
			tempMeshes.push_back(*it);
			continue;
		}

		if (batches.find(mesh) == batches.end()) {
			batches[mesh] = std::make_shared<gameParticles>();
		}

		auto& batch = batches[mesh];

		if (batch->activeInstances < batch->maxInstances) {
			// TODO: if more than 256 meshes, add new instance buffer
			batch->positions[batch->activeInstances] = trans;
			batch->activeInstances++;
		}
	}

	for (auto& [mesh, particles] : batches) {
		particles->syncBuffer();
		instancedMeshes.push_back({ glm::mat4(1), false, particles, mesh });
	}

	meshes = tempMeshes;
}

void renderQueue::clear(void) {
	meshes.clear();
	skinnedMeshes.clear();
	lights.clear();
	probes.clear();
	irradProbes.clear();
	instancedMeshes.clear();
	billboardMeshes.clear();
}

static void drawMesh(renderFlags& flags,
                     renderFramebuffer::ptr fb,
                     Program::ptr program,
                     glm::mat4& transform,
                     bool inverted,
                     gameMesh::ptr mesh)
{
	if (!mesh->comped_mesh) {
		// mesh not compiled, can't draw
		// TODO: toggleable warnings
		return;
	}

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

	if (!flags.shadowmap) {
		set_material(program, mesh->comped_mesh);

		// enable()/disable() cache state, no need to worry about toggling
		// too much state if queue is sorted
		if (mesh->comped_mesh->blend != material::blend_mode::Opaque)
		{
			// TODO: handle mask blends
			enable(GL_BLEND);
		} else {
			disable(GL_BLEND);
		}
	}

	// TODO: need to keep track of the model face order
	if (flags.cull_faces) {
		setFaceOrder(inverted? GL_CW : GL_CCW);
	}

	bindVao(mesh->comped_mesh->vao);

	/*
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glLineWidth(2.0);
	enable(GL_LINE_SMOOTH);
	*/
	glDrawElements(GL_TRIANGLES,
	               mesh->comped_mesh->elements->currentSize,
	               GL_UNSIGNED_INT, 0);
	DO_ERROR_CHECK();
	//glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

static void drawMeshInstanced(renderFlags& flags,
                              renderFramebuffer::ptr fb,
                              Program::ptr program,
                              glm::mat4& transform,
                              bool inverted,
                              gameParticles::ptr particles,
                              gameMesh::ptr mesh)
{
	if (!mesh->comped_mesh) {
		// mesh not compiled, nothing to draw
		return;
	}

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

	if (!flags.shadowmap) {
		set_material(program, mesh->comped_mesh);

		// enable()/disable() cache state, no need to worry about toggling
		// too much state if queue is sorted
		if (mesh->comped_mesh->blend != material::blend_mode::Opaque) {
			// TODO: handle mask blends
			enable(GL_BLEND);
		} else {
			disable(GL_BLEND);
		}
	}

	// TODO: need to keep track of the model face order
	if (flags.cull_faces) {
		setFaceOrder(inverted? GL_CW : GL_CCW);
	}

	bindVao(mesh->comped_mesh->vao);

#if GLSL_VERSION >= 140
	particles->syncBuffer();
	program->setUniformBlock("instanceTransforms", particles->ubuffer,
	                         UBO_INSTANCE_TRANSFORMS);
	glDrawElementsInstanced(
		GL_TRIANGLES,
		mesh->comped_mesh->elements->currentSize,
		GL_UNSIGNED_INT, 0, particles->activeInstances);
	DO_ERROR_CHECK();

#else
	for (unsigned i = 0; i < particles->activeInstances; i++) {
		// TODO: fallback
	}
#endif
}

static void drawBillboards(renderFlags& flags,
                           renderFramebuffer::ptr fb,
                           Program::ptr program,
                           glm::mat4& transform,
                           bool inverted,
                           gameBillboardParticles::ptr particles,
                           gameMesh::ptr mesh)
{
	if (!mesh->comped_mesh) {
		// mesh not compiled, nothing to draw
		return;
	}

	if (!flags.shadowmap) {
		set_material(program, mesh->comped_mesh);

		// enable()/disable() cache state, no need to worry about toggling
		// too much state if queue is sorted
		if (mesh->comped_mesh->blend != material::blend_mode::Opaque) {
			// TODO: handle mask blends
			enable(GL_BLEND);
		} else {
			disable(GL_BLEND);
		}
	}

	glm::mat3 m_3x3_inv_transp =
		glm::transpose(glm::inverse(model_to_world(transform)));

	program->set("m", transform);
	program->set("m_3x3_inv_transp", m_3x3_inv_transp);

	enable(GL_BLEND);

	if (flags.cull_faces) {
		setFaceOrder(inverted? GL_CW : GL_CCW);
	}

	bindVao(mesh->comped_mesh->vao);

#if GLSL_VERSION >= 140
	//SDL_Log("Drawing billboard mesh... %u instances", particles->activeInstances);
	particles->syncBuffer();
	program->setUniformBlock("billboardPositions", particles->ubuffer,
	                         UBO_INSTANCE_TRANSFORMS);
	glDrawElementsInstanced(
		GL_TRIANGLES,
		mesh->comped_mesh->elements->currentSize,
		GL_UNSIGNED_INT, 0, particles->activeInstances);
	DO_ERROR_CHECK();

#else
	for (unsigned i = 0; i < particles->activeInstances; i++) {
		// TODO: fallback
	}
#endif
}


unsigned renderQueue::flush(unsigned width,
                            unsigned height,
                            renderContext::ptr rctx,
                            renderFlags& flags)
{
	unsigned drawnMeshes = 0;

	if (flags.sort)       { sort(); }
	if (flags.cull_faces) { enable(GL_CULL_FACE); }

	cam->setViewport(width, height);
	glm::mat4 view = cam->viewTransform();
	glm::mat4 projection = cam->projectionTransform();
	glm::mat4 v_inv = glm::inverse(view);

	flags.skinnedShader->bind();
	flags.skinnedShader->set("v", view);
	flags.skinnedShader->set("p", projection);
	flags.skinnedShader->set("v_inv", v_inv);
	flags.skinnedShader->set("cameraPosition", cam->position());

	for (auto& [skin, drawinfo] : skinnedMeshes) {
		for (auto& mesh : drawinfo) {
			skin->sync(flags.skinnedShader);

			if (!flags.shadowmap) {
				rctx->setIrradianceProbe(
					nearest_irradiance_probe(mesh.center),
					flags.skinnedShader);
			}

			drawMesh(flags, nullptr, flags.skinnedShader,
			         mesh.transform, mesh.inverted, mesh.data);
		}

		// TODO: check whether this skin is already synced
		DO_ERROR_CHECK();
	}

	flags.mainShader->bind();
	flags.mainShader->set("v", view);
	flags.mainShader->set("p", projection);
	flags.mainShader->set("v_inv", v_inv);
	flags.mainShader->set("cameraPosition", cam->position());

	//for (auto& [transform, inverted, mesh] : meshes) {
	for (auto& mesh : meshes) {
		if (!flags.shadowmap) {
			rctx->setIrradianceProbe(
				nearest_irradiance_probe(mesh.center),
				flags.mainShader);
		}

		drawMesh(flags, nullptr, flags.mainShader, mesh.transform, mesh.inverted, mesh.data);
		drawnMeshes++;
	}

	meshes.clear();
	lights.clear();
	probes.clear();
	skinnedMeshes.clear();
	instancedMeshes.clear();
	billboardMeshes.clear();

	return drawnMeshes;
}

unsigned renderQueue::flush(renderFramebuffer::ptr fb,
                            renderContext::ptr rctx,
                            renderFlags& flags)
{
	DO_ERROR_CHECK();
	unsigned drawnMeshes = 0;

	if (flags.sort) { sort(); }

	if (flags.cull_faces) {
		enable(GL_CULL_FACE);
		glFrontFace(GL_CCW);
	}

	if (flags.stencil) {
		enable(GL_STENCIL_TEST);
		glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
	}

	if (flags.depthTest) {
		enable(GL_DEPTH_TEST);
	} else {
		disable(GL_DEPTH_TEST);
	}

	glDepthMask(flags.depthMask? GL_TRUE : GL_FALSE);

	DO_ERROR_CHECK();
	assert(fb != nullptr);
	//fb->framebuffer->bind();
	fb->bind();

	disable(GL_SCISSOR_TEST);
	glViewport(0, 0, fb->width, fb->height);
	glScissor(0, 0, fb->width, fb->height);

	cam->setViewport(fb->width/rctx->settings.scaleX, fb->height/rctx->settings.scaleY);
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

		for (auto& mesh : drawinfo) {
			skin->sync(flags.skinnedShader);

			if (!flags.shadowmap) {
				rctx->setIrradianceProbe(
					nearest_irradiance_probe(mesh.center),
					flags.skinnedShader);
			}
			drawMesh(flags, fb, flags.skinnedShader, mesh.transform, mesh.inverted, mesh.data);
			drawnMeshes++;
		}

		// TODO: check whether this skin is already synced
		DO_ERROR_CHECK();
	}

	flags.mainShader->bind();
	flags.mainShader->set("v", view);
	flags.mainShader->set("p", projection);
	flags.mainShader->set("v_inv", v_inv);
	flags.mainShader->set("cameraPosition", cam->position());
	shaderSync(flags.mainShader, rctx);

	//for (auto& [transform, inverted, mesh] : meshes) {
	for (auto& mesh : meshes) {
		if (!flags.shadowmap) {
			rctx->setIrradianceProbe(
				nearest_irradiance_probe(mesh.center),
				flags.mainShader);
		}

		drawMesh(flags, fb, flags.mainShader, mesh.transform, mesh.inverted, mesh.data);
		drawnMeshes++;
	}

	flags.instancedShader->bind();
	flags.instancedShader->set("v", view);
	flags.instancedShader->set("p", projection);
	flags.instancedShader->set("v_inv", v_inv);
	flags.instancedShader->set("cameraPosition", cam->position());
	shaderSync(flags.instancedShader, rctx);

	for (auto& [transform, inverted, particleSystem, mesh] : instancedMeshes) {
		if (!flags.shadowmap) {
			rctx->setIrradianceProbe(
				nearest_irradiance_probe(applyTransform(transform)),
				flags.instancedShader);
		}

		drawMeshInstanced(flags, fb, flags.instancedShader,
		                  transform, inverted, particleSystem, mesh);
		drawnMeshes += particleSystem->activeInstances;
	}

	glDepthMask(GL_FALSE);

	flags.billboardShader->bind();
	DO_ERROR_CHECK();
	flags.billboardShader->set("v", view);
	flags.billboardShader->set("p", projection);
	flags.billboardShader->set("v_inv", v_inv);
	flags.billboardShader->set("cameraPosition", cam->position());
	DO_ERROR_CHECK();
	shaderSync(flags.billboardShader, rctx);
	DO_ERROR_CHECK();

	for (auto& [transform, inverted, particleSystem, mesh] : billboardMeshes) {
		if (!flags.shadowmap) {
			rctx->setIrradianceProbe(
				nearest_irradiance_probe(applyTransform(transform)),
				flags.billboardShader);
		}

		drawBillboards(flags, fb, flags.billboardShader,
		               transform, inverted, particleSystem, mesh);
		drawnMeshes += particleSystem->activeInstances;
		DO_ERROR_CHECK();
	}

	// TODO: clear()
	meshes.clear();
	lights.clear();
	probes.clear();
	skinnedMeshes.clear();
	instancedMeshes.clear();
	billboardMeshes.clear();

	return drawnMeshes;
}

typedef std::vector<std::pair<glm::mat4&, gameLightPoint::ptr>>       pointList;
typedef std::vector<std::pair<glm::mat4&, gameLightSpot::ptr>>        spotList;
typedef std::vector<std::pair<glm::mat4&, gameLightDirectional::ptr>> dirList;
typedef std::tuple<pointList, spotList, dirList> lightLists;

static void syncPlainUniforms(Program::ptr program,
	                          renderContext::ptr rctx,
	                          pointList& points,
	                          spotList& spots,
	                          dirList&  directionals)
{
	size_t pactive = min(MAX_LIGHTS, points.size());
	size_t sactive = min(MAX_LIGHTS, spots.size());
	size_t dactive = min(MAX_LIGHTS, directionals.size());

	program->set("active_point_lights",       (GLint)pactive);
	program->set("active_spot_lights",        (GLint)sactive);
	program->set("active_directional_lights", (GLint)dactive);

	SDL_Log(">>> syncing plain uniforms: %lu point, %lu spot, %lu directional",
		pactive, sactive, dactive);

	for (size_t i = 0; i < pactive; i++) {
		//gameLightPoint::ptr light = points[i];
		auto& [trans, light] = points[i];
		glm::vec3 pos = applyTransform(trans, light->getTransformTRS().position);

		// TODO: also updating all these uniforms can't be very efficient for a lot of
		//       moving point lights... maybe look into how uniform buffer objects work
		std::string locstr = "point_lights[" + std::to_string(i) + "]";

		program->set(locstr + ".position",      glm::vec4(pos, 1.0));
		DO_ERROR_CHECK();
		program->set(locstr + ".diffuse",       glm::vec4(light->diffuse));
		DO_ERROR_CHECK();
		program->set(locstr + ".intensity",     light->intensity);
		DO_ERROR_CHECK();
		program->set(locstr + ".radius",        light->radius);
		DO_ERROR_CHECK();
		program->set(locstr + ".casts_shadows", light->casts_shadows);
		DO_ERROR_CHECK();

		if (light->casts_shadows) {
			for (unsigned k = 0; k < 6; k++) {
				std::string sloc = locstr + ".shadowmap[" + std::to_string(k) + "]";
				auto vec = rctx->atlases.shadows->tex_vector(light->shadowmap[k]);
				program->set(sloc, glm::vec4(vec, 0.0));
				DO_ERROR_CHECK();
			}
		}
	}

	for (size_t i = 0; i < sactive; i++) {
		//gameLightSpot::ptr light = spots[i];
		auto& [trans, light] = spots[i];

		// TODO: also updating all these uniforms can't be very efficient for a lot of
		//       moving point lights... maybe look into how uniform buffer objects work
		TRS transform = light->getTransformTRS();
		std::string locstr = "spot_lights[" + std::to_string(i) + "]";
		// light points toward +X by default
		glm::vec3 rotvec = glm::mat3_cast(transform.rotation) * glm::vec3(1, 0, 0);

		program->set(locstr + ".position",      glm::vec4(transform.position, 1.0));
		DO_ERROR_CHECK();
		program->set(locstr + ".diffuse",       glm::vec4(light->diffuse));
		DO_ERROR_CHECK();
		program->set(locstr + ".direction",     glm::vec4(rotvec, 0.0));
		DO_ERROR_CHECK();
		program->set(locstr + ".intensity",     light->intensity);
		DO_ERROR_CHECK();
		program->set(locstr + ".radius",        light->radius);
		DO_ERROR_CHECK();
		program->set(locstr + ".angle",         light->angle);
		DO_ERROR_CHECK();
		program->set(locstr + ".casts_shadows", light->casts_shadows);
		DO_ERROR_CHECK();

		if (light->casts_shadows) {
			std::string smap  = locstr + ".shadowmap";
			std::string sproj = locstr + ".shadowproj";

			auto vec = rctx->atlases.shadows->tex_vector(light->shadowmap);
			program->set(smap, glm::vec4(vec, 0.0));
			program->set(sproj, light->shadowproj);
			DO_ERROR_CHECK();
		}
	}

	for (size_t i = 0; i < dactive; i++) {
		//gameLightDirectional::ptr light = directionals[i];
		auto& [trans, light] = directionals[i];

		// TODO: also updating all these uniforms can't be very efficient for a lot of
		//       moving point lights... maybe look into how uniform buffer objects work
		TRS transform = light->getTransformTRS();
		std::string locstr = "directional_lights[" + std::to_string(i) + "]";
		// light points toward +X by default
		glm::vec3 rotvec = glm::mat3_cast(transform.rotation) * glm::vec3(1, 0, 0);

		// position is ignored in the shader, but might as well set it anyway
		// TODO: probably remove position member from directional lights
		program->set(locstr + ".position",      glm::vec4(transform.position, 0.0));
		DO_ERROR_CHECK();
		program->set(locstr + ".diffuse",       glm::vec4(light->diffuse));
		DO_ERROR_CHECK();
		program->set(locstr + ".direction",     glm::vec4(rotvec, 0.0));
		DO_ERROR_CHECK();
		program->set(locstr + ".intensity",     light->intensity);
		DO_ERROR_CHECK();
		program->set(locstr + ".casts_shadows", light->casts_shadows);
		DO_ERROR_CHECK();

		if (light->casts_shadows) {
			std::string smap  = locstr + ".shadowmap";
			std::string sproj = locstr + ".shadowproj";

			auto vec = rctx->atlases.shadows->tex_vector(light->shadowmap);
			program->set(smap, glm::vec4(vec, 0.0));
			program->set(sproj, light->shadowproj);
			DO_ERROR_CHECK();
		}
	}
}

static inline float rectScale(float R, float d) {
	return 1.f / cos(d/R);
}

void grendx::buildTilemap(renderQueue& queue, renderContext::ptr rctx) {
	switch (rctx->lightingMode) {
		case renderContext::lightingModes::Clustered:
			// TODO:
			break;

		case renderContext::lightingModes::Tiled:
			buildTilemapTiled(queue, rctx);
			break;

		case renderContext::lightingModes::PlainArray:
		default:
			// nothing to do
			break;
	}
}

// TODO: possibly move this to it's own file
#include <grend/plane.hpp>
void grendx::buildTilemapTiled(renderQueue& queue, renderContext::ptr rctx) {
	lights_std140&      lightbuf = rctx->lightBufferCtx;
	light_tiles_std140& pointbuf = rctx->pointTilesCtx;
	light_tiles_std140& spotbuf  = rctx->spotTilesCtx;

	camera::ptr cam = queue.cam;
	float rx = glm::radians(cam->fovx());
	float ry = glm::radians(cam->fovy());

	size_t activePoints = 0;
	size_t activeSpots = 0;
	size_t activeDirs = 0;

	// TODO: put arrays in one big struct so that this can be just one memset()
	memset(&lightbuf.upoint_lights, 0, sizeof(lightbuf.upoint_lights));
	memset(&lightbuf.uspot_lights, 0, sizeof(lightbuf.uspot_lights));
	memset(&lightbuf.udirectional_lights, 0, sizeof(lightbuf.udirectional_lights));
	memset(&pointbuf, 0, sizeof(pointbuf));
	memset(&spotbuf,  0, sizeof(spotbuf));

	plane planes[16*9*6];
	glm::vec3 nearpos = cam->position() + cam->direction()*cam->near();
	glm::vec3 cpos = cam->position();

	unsigned x = 0;
	for (float tx = rx/2.0; x < 16; tx -= rx/16.0, x++) {
		unsigned y = 0;
		for (float ty = ry/2.0; y < 9; ty -= ry/9.0, y++) {
			unsigned cluster = x + y*16;
			plane *p = planes + cluster*6;

			// left
			float txp = tx - rx/16.f;
			p[0].n = glm::rotate(cam->right(), txp/rectScale(rx, txp), cam->up());
			p[0].d = -glm::dot(p[0].n, cpos);

			// right
			p[1].n = -glm::rotate(cam->right(), tx/rectScale(rx, tx), cam->up());
			p[1].d = -glm::dot(p[1].n, cpos);

			// bottom
			p[2].n = glm::rotate(cam->up(), ty/rectScale(ry, ty), cam->right());
			p[2].d = -glm::dot(p[2].n, cpos);

			// top
			float typ = ty - ry/9.f;
			p[3].n = -glm::rotate(cam->up(), typ/rectScale(ry, typ), cam->right());
			p[3].d = -glm::dot(p[3].n, cpos);

			// near
			p[4].n = cam->direction();
			p[4].d = -glm::dot(p[4].n, nearpos);

			// far
			p[5].n = -cam->direction();
			p[5].d = -glm::dot(p[5].n, cam->position() + cam->direction()*cam->far());
		}
	}

	// TODO: parallelism
	auto buildTiles = [&] (gameLight::ptr lit, glm::mat4& trans, unsigned idx) {
		glm::vec3 lightpos = applyTransform(trans);
		float ext = lit->extent(rctx->lightThreshold);

		for (unsigned cluster = 0; cluster < 16*9; cluster++) {
			unsigned in = 0;
			plane *p = planes + cluster*6;

			for (unsigned i = 0; i < 6; i++) {
				in += p[i].inPlane(lightpos, ext);
			}

			if (in == 6) {
				unsigned clusidx = MAX_LIGHTS * cluster;

				if (lit->lightType == gameLight::lightTypes::Point) {
					unsigned cur  = pointbuf.indexes[clusidx];
					unsigned next = cur + 1;

					if (next < MAX_LIGHTS) {
						pointbuf.indexes[clusidx]        = next;
						pointbuf.indexes[clusidx + next] = idx;
					}

				} else if (lit->lightType == gameLight::lightTypes::Spot) {
					unsigned cur  = spotbuf.indexes[clusidx];
					unsigned next = cur + 1;

					if (next < MAX_LIGHTS) {
						spotbuf.indexes[clusidx]        = next;
						spotbuf.indexes[clusidx + next] = idx;
					}
				}
			}
		}
	};

	//for (auto& [trans, _, lit] : queue.lights) {
	for (auto& lit : queue.lights) {
		if (activePoints < MAX_POINT_LIGHT_OBJECTS_TILED &&
		    lit.data->lightType == gameLight::lightTypes::Point)
		{
			gameLightPoint::ptr plit =
				std::dynamic_pointer_cast<gameLightPoint>(lit.data);

			packLight(plit,
					  lightbuf.upoint_lights + activePoints,
					  rctx, lit.transform);
			buildTiles(lit.data, lit.transform, activePoints);
			activePoints++;

		} else if (activeSpots < MAX_SPOT_LIGHT_OBJECTS_TILED
		           && lit.data->lightType == gameLight::lightTypes::Spot)
		{
			gameLightSpot::ptr slit =
				std::dynamic_pointer_cast<gameLightSpot>(lit.data);

			packLight(slit,
					  lightbuf.uspot_lights + activeSpots,
					  rctx, lit.transform);
			buildTiles(lit.data, lit.transform, activeSpots);
			activeSpots++;

		} else if (activeDirs < MAX_DIRECTIONAL_LIGHT_OBJECTS_TILED
		          && lit.data->lightType == gameLight::lightTypes::Directional)
		{
			gameLightDirectional::ptr dlit =
				std::dynamic_pointer_cast<gameLightDirectional>(lit.data);

			packLight(dlit,
					  lightbuf.udirectional_lights + activeDirs,
					  rctx, lit.transform);
			activeDirs++;
		}
	}

	lightbuf.uactive_point_lights       = activePoints;
	lightbuf.uactive_spot_lights        = activeSpots;
	lightbuf.uactive_directional_lights = activeDirs;

	/*
	SDL_Log("updating buffers: lights: %u, points: %u, spots: %u",
	        sizeof(lightbuf), sizeof(pointbuf), sizeof(spotbuf));
			*/
	rctx->lightBuffer->update(&lightbuf, 0, sizeof(lightbuf));
	rctx->pointTiles->update(&pointbuf,  0, sizeof(pointbuf));
	rctx->spotTiles->update(&spotbuf,    0, sizeof(spotbuf));
}

void renderQueue::updateReflectionProbe(renderContext::ptr rctx) {
	auto refprobe = nearest_reflection_probe(cam->position());

	// TODO: set the real transform
	glm::mat4 xxx(1);
	packRefprobe(refprobe, &rctx->lightBufferCtx, rctx, xxx);
	//rctx->lightBuffer->update(&rctx->lightBufferCtx, 16, 528);
	rctx->lightBuffer->update(&rctx->lightBufferCtx, 0, sizeof(rctx->lightBufferCtx));
}

void renderQueue::shaderSync(Program::ptr program, renderContext::ptr rctx) {
	if (rctx->lightingMode == renderContext::lightingModes::Clustered) {
		// TODO: implement
		// TODO: function to set mode
		SDL_Log("TODO: clustered light array");
	}

	else if (rctx->lightingMode == renderContext::lightingModes::Tiled) {
		program->setUniformBlock("lights", rctx->lightBuffer, UBO_LIGHT_INFO);
#if !defined(USE_SINGLE_UBO)
		program->setUniformBlock("point_light_tiles", rctx->pointTiles,
								 UBO_POINT_LIGHT_TILES);
		program->setUniformBlock("spot_light_tiles", rctx->spotTiles,
								 UBO_SPOT_LIGHT_TILES);
#endif
		DO_ERROR_CHECK();
	}

	else if (rctx->lightingMode == renderContext::lightingModes::PlainArray) {
		pointList point_lights;
		spotList  spot_lights;
		dirList   directional_lights;

		//for (auto& [trans, _, light] : lights) {
		for (auto& light : lights) {
			switch (light.data->lightType) {
				case gameLight::lightTypes::Point:
					{
						gameLightPoint::ptr plit =
							std::dynamic_pointer_cast<gameLightPoint>(light.data);
						point_lights.push_back({light.transform, plit});
					}
					break;

				case gameLight::lightTypes::Spot:
					{
						gameLightSpot::ptr slit =
							std::dynamic_pointer_cast<gameLightSpot>(light.data);
						spot_lights.push_back({light.transform, slit});
					}
					break;

				case gameLight::lightTypes::Directional:
					{
						gameLightDirectional::ptr dlit =
							std::dynamic_pointer_cast<gameLightDirectional>(light.data);
						directional_lights.push_back({light.transform, dlit});
					}
					break;

				default:
					break;
			}
		}

		syncPlainUniforms(program, rctx, point_lights,
						  spot_lights, directional_lights);

		//set_reflection_probe(refprobe, program, rctx->atlases);
		DO_ERROR_CHECK();
	}

	program->set("renderWidth",  (float)rctx->framebuffer->width);
	program->set("renderHeight", (float)rctx->framebuffer->height);
	program->set("lightThreshold", rctx->lightThreshold);

	glActiveTexture(TEX_GL_REFLECTIONS);
	rctx->atlases.reflections->color_tex->bind();
	program->set("reflection_atlas", TEXU_REFLECTIONS);

	glActiveTexture(TEX_GL_SHADOWS);
	rctx->atlases.shadows->depth_tex->bind();
	program->set("shadowmap_atlas", TEXU_SHADOWS);
	DO_ERROR_CHECK();

	glActiveTexture(TEX_GL_IRRADIANCE);
	rctx->atlases.irradiance->color_tex->bind();
	program->set("irradiance_atlas", TEXU_IRRADIANCE);
	DO_ERROR_CHECK();
}

gameReflectionProbe::ptr renderQueue::nearest_reflection_probe(glm::vec3 pos) {
	gameReflectionProbe::ptr ret = nullptr;
	float mindist = HUGE_VALF;

	// TODO: optimize this, O(N) is meh
	for (auto& p : probes) {
		float dist = glm::distance(pos, p.data->getTransformTRS().position);
		if (!ret || dist < mindist) {
			mindist = dist;
			ret = p.data;
		}
	}

	return ret;
}

gameIrradianceProbe::ptr renderQueue::nearest_irradiance_probe(glm::vec3 pos) {
	gameIrradianceProbe::ptr ret = nullptr;
	float mindist = HUGE_VALF;

	// TODO: optimize this, O(N) is meh
	for (auto& p : irradProbes) {
		float dist = glm::distance(pos, p.data->getTransformTRS().position);
		if (!ret || dist < mindist) {
			mindist = dist;
			ret = p.data;
		}
	}

	return ret;
}
