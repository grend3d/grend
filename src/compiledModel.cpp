#include <grend/compiledModel.hpp>
#include <grend/logger.hpp>

namespace grendx {

// TODO: should be stored in a context somewhere
static std::map<material*, compiledMaterial::weakptr> materialCache;

compiledMaterial::~compiledMaterial() {
	//LogInfo("Freeing a compiledMaterial");
}

compiledMesh::~compiledMesh() {
	//LogInfo("Freeing a compiledMesh");
}

compiledModel::~compiledModel() {
	//LogInfo("Freeing a compiledModel");
}


compiledMaterial::ptr matcache(material::ptr mat) {
	if (!mat) {
		return nullptr;
	}

	auto it = materialCache.find(mat.get());

	if (it != materialCache.end()) {
		if (auto observe = it->second.lock()) {
			return observe;
		}
	}

	compiledMaterial::ptr ret = std::make_shared<compiledMaterial>();

	ret->factors = mat->factors;
	auto& maps = mat->maps;

	if (maps.diffuse && maps.diffuse->loaded()) {
		ret->textures.diffuse = texcache(maps.diffuse, true);
	}

	if (maps.metalRoughness && maps.metalRoughness->loaded()) {
		ret->textures.metalRoughness = texcache(maps.metalRoughness);
	}

	if (maps.normal && maps.normal->loaded()) {
		ret->textures.normal = texcache(maps.normal);
	}

	if (maps.ambientOcclusion && maps.ambientOcclusion->loaded()) {
		ret->textures.ambientOcclusion = texcache(maps.ambientOcclusion);
	}

	if (maps.emissive && maps.emissive->loaded()) {
		ret->textures.emissive = texcache(maps.emissive, true);
	}

	if (maps.lightmap && maps.lightmap->loaded()) {
		ret->textures.lightmap = texcache(maps.lightmap, true);
	}

	materialCache[mat.get()] = ret;
	// XXX: once the material is cached no need to keep pointers around...
	//      need better way to do this
	maps.diffuse.reset();
	maps.metalRoughness.reset();
	maps.normal.reset();
	maps.ambientOcclusion.reset();
	maps.emissive.reset();
	maps.lightmap.reset();

	return ret;
}

compiledMesh::ptr compileMesh(sceneMesh::ptr& mesh) {
	compiledMesh::ptr foo = compiledMesh::ptr(new compiledMesh());

	if (mesh->faces.size() == 0) {
		LogInfo("Mesh has no indices!");
		return foo;
	}

	mesh->comped_mesh = foo;
	mesh->compiled = true;

	foo->elements = genBuffer(GL_ELEMENT_ARRAY_BUFFER);
	foo->elements->buffer(mesh->faces.data(),
						  mesh->faces.size() * sizeof(GLuint));

	// TODO: more consistent naming here
	foo->mat   = matcache(mesh->meshMaterial);
	foo->blend = foo->mat? foo->mat->factors.blend : material::blend_mode::Opaque;

	return foo;
}

compiledModel::ptr compileModel(std::string name, sceneModel::ptr model) {
	// TODO: might be able to clear vertex info after compiling here
	compiledModel::ptr obj = compiledModel::ptr(new compiledModel());
	model->comped_model = obj;
	model->compiled = true;

	obj->vertices = genBuffer(GL_ARRAY_BUFFER);
	obj->vertices->buffer(model->vertices.data(),
	                      model->vertices.size() * sizeof(sceneModel::vertex));

	if (model->haveJoints) {
		obj->haveJoints = true;
		obj->joints = genBuffer(GL_ARRAY_BUFFER);
		obj->joints->buffer(model->joints.data(),
		                    model->joints.size() * sizeof(sceneModel::jointWeights));
	}

	for (auto& [meshname, ptr] : model->nodes) {
		if (ptr->type == sceneNode::objType::Mesh) {
			auto wptr = std::dynamic_pointer_cast<sceneMesh>(ptr);
			obj->meshes[meshname] = compileMesh(wptr);
		}
	}

	DO_ERROR_CHECK();
	obj->vao = preloadModelVao(obj);

	return obj;
}

void compileModels(const modelMap& models) {
	for (const auto& x : models) {
		compileModel(x.first, x.second);
	}
}

Vao::ptr preloadMeshVao(compiledModel::ptr obj, compiledMesh::ptr mesh) {
	if (mesh == nullptr || !mesh->elements) {
		LogWarn("Have broken mesh...");
		return getCurrentVao();
	}

	Vao::ptr orig_vao = getCurrentVao();
	Vao::ptr ret = bindVao(genVao());

	mesh->elements->bind();
	glEnableVertexAttribArray(VAO_ELEMENTS);
	glVertexAttribPointer(VAO_ELEMENTS, 3, GL_UNSIGNED_INT, GL_FALSE, 0, 0);

	obj->vertices->bind();
	glEnableVertexAttribArray(VAO_VERTICES);
	SET_VAO_ENTRY(VAO_VERTICES, sceneModel::vertex, position);

	glEnableVertexAttribArray(VAO_NORMALS);
	SET_VAO_ENTRY(VAO_NORMALS, sceneModel::vertex, normal);

	glEnableVertexAttribArray(VAO_TANGENTS);
	SET_VAO_ENTRY(VAO_TANGENTS, sceneModel::vertex, tangent);

	glEnableVertexAttribArray(VAO_COLORS);
	SET_VAO_ENTRY(VAO_COLORS, sceneModel::vertex, color);

	glEnableVertexAttribArray(VAO_TEXCOORDS);
	SET_VAO_ENTRY(VAO_TEXCOORDS, sceneModel::vertex, uv);

	glEnableVertexAttribArray(VAO_LIGHTMAP);
	SET_VAO_ENTRY(VAO_LIGHTMAP, sceneModel::vertex, lightmap);

	if (obj->haveJoints) {
		obj->joints->bind();
		glEnableVertexAttribArray(VAO_JOINTS);
		SET_VAO_ENTRY(VAO_JOINTS, sceneModel::jointWeights, joints);

		glEnableVertexAttribArray(VAO_JOINT_WEIGHTS);
		SET_VAO_ENTRY(VAO_JOINT_WEIGHTS, sceneModel::jointWeights, weights);
	}

	bindVao(orig_vao);
	DO_ERROR_CHECK();
	return ret;
}

Vao::ptr preloadModelVao(compiledModel::ptr obj) {
	Vao::ptr orig_vao = getCurrentVao();

	for (auto [mesh_name, ptr] : obj->meshes) {
		ptr->vao = preloadMeshVao(obj, ptr);
	}

	return orig_vao;
}

void bindModel(sceneModel::ptr model) {
	if (model->comped_model == nullptr) {
		LogError("Trying to bind an uncompiled model");
	}

	model->comped_model->vao = preloadModelVao(model->comped_model);
}

// namespace grendx
}
