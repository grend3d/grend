#pragma once

#include <grend/ecs/ref.hpp>
#include <grend/glManager.hpp>
#include <grend/materialTexture.hpp>
#include <grend/material.hpp>

namespace grendx {

// defined in sceneModel.hpp, included at end
class sceneMesh;
class sceneModel;

class compiledMaterial {
	public:
		typedef std::shared_ptr<compiledMaterial> ptr;
		typedef std::weak_ptr<compiledMaterial> weakptr;

		~compiledMaterial();

		material::materialFactors factors;
		struct loadedTextures {
			Texture::ptr diffuse;
			Texture::ptr metalRoughness;
			Texture::ptr normal;
			Texture::ptr ambientOcclusion;
			Texture::ptr emissive;
			Texture::ptr lightmap;
		} textures;
};

// TODO: camelCase
class compiledMesh {
	public:
		typedef std::shared_ptr<compiledMesh> ptr;
		typedef std::weak_ptr<compiledMesh> weakptr;

		~compiledMesh();

		Vao::ptr vao;
		Buffer::ptr elements;
		compiledMaterial::ptr mat;
		material::blend_mode blend;
};

// TODO: camelCase
class compiledModel {
	public:
		typedef std::shared_ptr<compiledModel> ptr;
		typedef std::weak_ptr<compiledModel> weakptr;

		~compiledModel();

		Vao::ptr vao;
		std::map<std::string, compiledMesh::ptr> meshes;
		Buffer::ptr vertices;

		bool haveJoints = false;
		Buffer::ptr joints;
};

compiledMaterial::ptr matcache(material::ptr mat);

compiledMesh::ptr compileMesh(ecs::ref<sceneMesh> mesh);
compiledModel::ptr compileModel(std::string name, ecs::ref<sceneModel> mod);
void compileModels(const std::map<std::string, ecs::ref<sceneModel>>& models);
Vao::ptr preloadMeshVao(compiledModel::ptr obj, compiledMesh::ptr mesh);
Vao::ptr preloadModelVao(compiledModel::ptr obj);
void bindModel(ecs::ref<sceneModel> model);

// namespace grendx
}
#include <grend/sceneModel.hpp>
