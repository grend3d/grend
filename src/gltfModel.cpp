#include <grend/gameModel.hpp>
#include <grend/utility.hpp>

#include <stb/stb_image.h>
#include <stb/stb_image_write.h>

#include <vector>
#include <map>
#include <string>
#include <exception>
#include <iostream>
#include <fstream>
#include <sstream>
#include <optional>

#include <stdint.h>

// TODO: still need to clean this code up a bit

using namespace grendx;

template <typename T>
static inline bool check_index(std::vector<T> container, size_t idx) {
	if (idx >= container.size()) {
		// TODO: toggle exceptions somehow
		throw std::out_of_range("check_index()");
		return false;
	}

	return true;
}

static inline bool assert_type(int type, int expected) {
	if (type != expected) {
		throw std::invalid_argument(
			"assert_type(): have " + std::to_string(type)
			+ ", expected " + std::to_string(expected));
		return false;
	}

	return true;
}

static inline bool assert_nonzero(size_t x) {
	if (x == 0) {
		throw std::logic_error("assert_nonzero()");
		return false;
	}

	return true;
}

static tinygltf::Material& gltf_material(tinygltf::Model& gltf_model, size_t idx) {
	check_index(gltf_model.materials, idx);
	return gltf_model.materials[idx];
}

static tinygltf::Texture& gltf_texture(tinygltf::Model& gltf_model, size_t idx) {
	check_index(gltf_model.textures, idx);
	return gltf_model.textures[idx];
}

static tinygltf::Image& gltf_image(tinygltf::Model& gltf_model, size_t idx) {
	check_index(gltf_model.textures, idx);
	return gltf_model.images[idx];
}

static size_t gltf_buff_element_size(int component, int type) {
	size_t size_component = 0, size_type = 0;

	switch (type) {
		case TINYGLTF_TYPE_VECTOR:
		case TINYGLTF_TYPE_SCALAR: size_type = 1; break;
		case TINYGLTF_TYPE_VEC2: size_type = 2; break;
		case TINYGLTF_TYPE_VEC3: size_type = 3; break;
		case TINYGLTF_TYPE_VEC4: size_type = 4; break;
		case TINYGLTF_TYPE_MATRIX: size_type = 1; /* TODO: check this */ break;
		case TINYGLTF_TYPE_MAT2: size_type = 2*2; break;
		case TINYGLTF_TYPE_MAT3: size_type = 3*3; break;
		case TINYGLTF_TYPE_MAT4: size_type = 4*4; break;
	}

	switch (component) {
		case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
		case TINYGLTF_COMPONENT_TYPE_BYTE:
			size_component = sizeof(GLbyte);
			break;

		case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
		case TINYGLTF_COMPONENT_TYPE_SHORT:
			size_component = sizeof(GLushort);
			break;

		case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
		case TINYGLTF_COMPONENT_TYPE_INT:
			size_component = sizeof(GLint);
			break;

		case TINYGLTF_COMPONENT_TYPE_FLOAT:
			size_component = sizeof(GLfloat);
			break;

		case TINYGLTF_COMPONENT_TYPE_DOUBLE:
			size_component = sizeof(GLdouble); 
			break;
	}

	return size_component * size_type;
}

static void *gltf_load_accessor(tinygltf::Model& tgltf_model, int idx) {
	check_index(tgltf_model.accessors, idx);
	auto& acc = tgltf_model.accessors[idx];

	size_t elem_size =
		gltf_buff_element_size(acc.componentType, acc.type);
	size_t buff_size = acc.count * elem_size;
	assert_nonzero(elem_size);

	check_index(tgltf_model.bufferViews, acc.bufferView);
	auto& view = tgltf_model.bufferViews[acc.bufferView];
	check_index(tgltf_model.buffers, view.buffer);
	auto& buffer = tgltf_model.buffers[view.buffer];

	std::cerr << "        - acc @ " << idx << std::endl;
	std::cerr << "        - view @ " << acc.bufferView << std::endl;
	std::cerr << "        - buffer @ " << view.buffer << std::endl;

	check_index(buffer.data,
			view.byteOffset + acc.byteOffset + buff_size - 1);
	return static_cast<void*>(buffer.data.data() + (view.byteOffset + acc.byteOffset));
};

template<typename T>
static void gltf_unpack_buffer(tinygltf::Model& gltf_model,
                               int accessor,
                               std::vector<T>& vec)
{
	check_index(gltf_model.accessors, accessor);
	auto& acc = gltf_model.accessors[accessor];
	check_index(gltf_model.bufferViews, acc.bufferView);
	auto& view = gltf_model.bufferViews[acc.bufferView];

	uint8_t *datas = static_cast<uint8_t*>(gltf_load_accessor(gltf_model, accessor));
	size_t emsz = view.byteStride
		? view.byteStride
		: gltf_buff_element_size(acc.componentType, acc.type);

	for (unsigned i = 0; i < acc.count; i++) {
		T *em = reinterpret_cast<T*>(datas + i*emsz);
		vec.push_back(*em);
	}
}

static std::optional<grendx::AABB>
gltf_accessor_aabb(tinygltf::Accessor& acc) {
	bool have_aabb =
		!acc.minValues.empty() && !acc.maxValues.empty()
		&& acc.minValues.size() == 3
		&& acc.maxValues.size() == 3;

	if (have_aabb) {
		auto& min = acc.minValues;
		auto& max = acc.maxValues;

		return std::optional<grendx::AABB>({
			glm::vec3(min[0], min[1], min[2]),
			glm::vec3(max[0], max[1], max[2]),
		});

	} else {
		return {};
	}
}

static void gltf_unpack_ushort_to_uint(tinygltf::Model& gltf_model,
                                       int accessor,
                                       std::vector<GLuint>& vec)
{
	check_index(gltf_model.accessors, accessor);
	auto& acc = gltf_model.accessors[accessor];
	check_index(gltf_model.bufferViews, acc.bufferView);
	auto& view = gltf_model.bufferViews[acc.bufferView];

	uint8_t *datas = static_cast<uint8_t*>(gltf_load_accessor(gltf_model, accessor));
	size_t emsz = view.byteStride
		? view.byteStride
		: gltf_buff_element_size(acc.componentType, acc.type);

	for (unsigned i = 0; i < acc.count; i++) {
		//T *em = reinterpret_cast<T*>(datas + i*emsz);
		GLushort *em = reinterpret_cast<GLushort*>(datas + i*emsz);
		vec.push_back((GLuint)*em);
	}
}

static grendx::materialTexture::ptr
gltf_load_texture(tinygltf::Model& gltf_model, int tex_idx) {
	grendx::materialTexture::ptr ret = std::make_shared<grendx::materialTexture>();

	auto& tex = gltf_texture(gltf_model, tex_idx);
	std::cerr << "        + texture source: " << tex.source << std::endl;

	auto& img = gltf_image(gltf_model, tex.source);
	std::cerr << "        + texture image source: " << img.uri << ", "
		<< img.width << "x" << img.height << ":" << img.component << std::endl;


	ret->pixels.insert(ret->pixels.end(),
			img.image.begin(), img.image.end());
	ret->width = img.width;
	ret->height = img.height;
	ret->channels = img.component;
	ret->size = ret->width * ret->height * ret->channels;

	return ret;
}

static grendx::material::ptr
  gltf_load_material(tinygltf::Model& gltf_model, int material_idx)
{
	grendx::material::ptr ret = std::make_shared<grendx::material>();
	auto& mat = gltf_material(gltf_model, material_idx);

	/*
	assert(out_model->hasNode(temp_name));
	grendx::gameMesh::ptr mesh
		= std::dynamic_pointer_cast<grendx::gameMesh>(out_model->nodes[temp_name]);
	std::string matidxname = mat.name + ":" + std::to_string(material_idx);
	mesh->material = matidxname;
	*/
	std::string matidxname = ":" + std::to_string(material_idx);

	auto& pbr = mat.pbrMetallicRoughness;
	auto& base_color = pbr.baseColorFactor;
	glm::vec4 mat_diffuse(base_color[0], base_color[1],
			base_color[2], base_color[3]);

	std::cerr << "        + have material " << matidxname << std::endl;
	std::cerr << "        + base color: "
		<< mat_diffuse.x << ", " << mat_diffuse.y
		<< ", " << mat_diffuse.z << ", " << mat_diffuse.w
		<< std::endl;
	std::cerr << "        + alphaMode: " << mat.alphaMode << std::endl;
	std::cerr << "        + alphaCutoff: " << mat.alphaCutoff << std::endl;

	std::cerr << "        + pbr roughness: "
		<< pbr.roughnessFactor << std::endl;

	std::cerr << "        + pbr metallic: "
		<< pbr.metallicFactor << std::endl;

	std::cerr << "        + pbr base idx: "
		<< pbr.baseColorTexture.index << std::endl;
	std::cerr << "        + metal/roughness map idx: "
		<< pbr.metallicRoughnessTexture.index << std::endl;
	std::cerr << "        + normal map idx: "
		<< mat.normalTexture.index << std::endl;
	std::cerr << "        + occlusion map idx: "
		<< mat.occlusionTexture.index << std::endl;

	if (pbr.baseColorTexture.index >= 0) {
		ret->maps.diffuse =
			gltf_load_texture(gltf_model, pbr.baseColorTexture.index);
	}

	if (pbr.metallicRoughnessTexture.index >= 0) {
		ret->maps.metalRoughness =
			gltf_load_texture(gltf_model, pbr.metallicRoughnessTexture.index);
	}


	if (mat.normalTexture.index >= 0) {
		ret->maps.normal =
			gltf_load_texture(gltf_model, mat.normalTexture.index);
	}

	if (mat.occlusionTexture.index >= 0) {
		ret->maps.ambientOcclusion =
			gltf_load_texture(gltf_model, mat.occlusionTexture.index);
	}

	if (mat.emissiveTexture.index >= 0) {
		ret->maps.emissive =
			gltf_load_texture(gltf_model, mat.emissiveTexture.index);
	}

	if (mat.alphaMode == "BLEND") {
		// XXX:
		ret->factors.opacity = mat.alphaCutoff;
		ret->factors.blend = grendx::material::blend_mode::Blend;

	} else if (mat.alphaMode == "MASK") {
		// XXX:
		ret->factors.opacity = mat.alphaCutoff;
		ret->factors.blend = grendx::material::blend_mode::Mask;
	}

	auto& ev = mat.emissiveFactor;
	ret->factors.emissive  = glm::vec4(ev[0], ev[1], ev[2], 1.0);
	ret->factors.roughness = pbr.roughnessFactor;
	ret->factors.metalness = pbr.metallicFactor;
	ret->factors.diffuse   = mat_diffuse;

	return ret;
}

grendx::modelMap grendx::load_gltf_models(tinygltf::Model& tgltf_model) {
	modelMap ret;

	for (auto& mesh : tgltf_model.meshes) {
		grendx::gameModel::ptr curModel =
			grendx::gameModel::ptr(new grendx::gameModel());
		ret[mesh.name] = curModel;

		std::cerr << " GLTF > have mesh " << mesh.name << std::endl;
		std::cerr << "      > primitives: " << std::endl;

		for (size_t i = 0; i < mesh.primitives.size(); i++) {
			auto& prim = mesh.primitives[i];
			std::string suffix = ":[p"+std::to_string(i)+"]";
			std::string temp_name = mesh.name + suffix
				+ ":" + std::to_string(prim.material);

			std::cerr << "        primitive: " << temp_name << std::endl;
			std::cerr << "        material: " << prim.material << std::endl;
			std::cerr << "        indices: " << prim.indices << std::endl;
			std::cerr << "        mode: " << prim.mode << std::endl;

			grendx::gameMesh::ptr modmesh = grendx::gameMesh::ptr(new grendx::gameMesh());
			setNode(temp_name, curModel, modmesh);

			if (prim.material >= 0) {
				modmesh->meshMaterial = 
					gltf_load_material(tgltf_model, prim.material);

			} else {
				// XXX: a little wasteful
				modmesh->meshMaterial = std::make_shared<material>();
			}

			// accessor indices
			int elements = prim.indices;
			int normals = -1;
			int position = -1;
			int texcoord = -1;
			int joints = -1;
			int weights = -1;
			int tangents = -1;

			for (auto& attr : prim.attributes) {
				if (attr.first == "NORMAL")     normals = attr.second;
				if (attr.first == "POSITION")   position = attr.second;
				if (attr.first == "TEXCOORD_0") texcoord = attr.second;
				if (attr.first == "JOINTS_0")   joints = attr.second; 
				if (attr.first == "WEIGHTS_0")  weights = attr.second; 
				if (attr.first == "TANGENT")    tangents = attr.second;
			}

			if (position < 0) {
				throw std::logic_error(
					"load_gltf_models(): can't load a mesh primitive "
					"without position information");
			}

			if (elements >= 0) {
				check_index(tgltf_model.accessors, elements);

				auto& acc = tgltf_model.accessors[elements];
				assert_type(acc.type, TINYGLTF_TYPE_SCALAR);

				size_t vsize = curModel->vertices.size();
				auto& submesh = modmesh->faces;

				if (acc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
					gltf_unpack_ushort_to_uint(tgltf_model, elements, submesh);
				} else if (acc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) {
					gltf_unpack_buffer(tgltf_model, elements, submesh);
				} else {
					assert_type(acc.componentType, TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT);
				}

				// adjust element indices for vertices already in the model
				// (model element indices are per-model, seems gltf is per-primitive)
				for (auto& face : submesh) {
					face += vsize;
				}

			} else {
				// identity-mapped indices
				std::cerr << "        generating indices..." << std::endl;
				auto& acc = tgltf_model.accessors[position];
				auto& submesh = modmesh->faces;
				size_t vsize = curModel->vertices.size();

				for (size_t i = 0; i < acc.count; i++) {
					submesh.push_back(i + vsize);
				}
			}

			if (normals >= 0) {
				check_index(tgltf_model.accessors, normals);

				gltf_unpack_buffer(tgltf_model, normals, curModel->normals);
				curModel->haveNormals = true;
			}

			if (tangents >= 0) {
				check_index(tgltf_model.accessors, tangents);
				std::cerr << "        have tangents... " << tangents << std::endl;

				gltf_unpack_buffer(tgltf_model, tangents, curModel->tangents);
				curModel->haveTangents = true;
			}

			if (position >= 0) {
				check_index(tgltf_model.accessors, position);

				auto& acc = tgltf_model.accessors[position];
				assert_type(acc.type, TINYGLTF_TYPE_VEC3);
				assert_type(acc.componentType, 
					TINYGLTF_COMPONENT_TYPE_FLOAT);

				gltf_unpack_buffer(tgltf_model, position, curModel->vertices);

				if (auto box = gltf_accessor_aabb(acc)) {
					curModel->haveAABB = true;
					modmesh->boundingBox = *box;
				}
			}

			if (texcoord >= 0) {
				check_index(tgltf_model.accessors, texcoord);

				auto& acc = tgltf_model.accessors[texcoord];
				assert_type(acc.type, TINYGLTF_TYPE_VEC2);
				assert_type(acc.componentType, TINYGLTF_COMPONENT_TYPE_FLOAT);

				gltf_unpack_buffer(tgltf_model, texcoord, curModel->texcoords);
				curModel->haveTexcoords = true;
			}

			if (joints >= 0 && weights >= 0) {
				check_index(tgltf_model.accessors, joints);
				check_index(tgltf_model.accessors, weights);
				auto& jac = tgltf_model.accessors[joints];
				auto& wac = tgltf_model.accessors[weights];

				assert_type(jac.type, TINYGLTF_TYPE_VEC4);
				assert_type(wac.type, TINYGLTF_TYPE_VEC4);
				assert_type(jac.componentType,
					TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT);
				assert_type(wac.componentType,
					TINYGLTF_COMPONENT_TYPE_FLOAT);
				gltf_unpack_buffer(tgltf_model, joints, curModel->joints);
				gltf_unpack_buffer(tgltf_model, weights, curModel->weights);
				curModel->haveJoints = true;
				std::cerr << "        have joints: " << joints << std::endl;
			}
		}

		// generate anything not included
		if (!curModel->haveNormals) {
			curModel->genNormals();
		}

		if (!curModel->haveTexcoords) {
			curModel->genTexcoords();
		}

		if (!curModel->haveTangents) {
			curModel->genTangents();
		}

		if (!curModel->haveAABB) {
			curModel->genAABBs();
		}
	}

	return ret;
}

struct mapent {
	animationChannel::ptr channel;
	tinygltf::Animation anim;
};

typedef
std::map<int, std::vector<animationChannel::ptr>> animation_map;

static void assert_accessor_type(tinygltf::Model& mod, int accidx, int expected)
{
	check_index(mod.accessors, accidx);
	auto& acc = mod.accessors[accidx];
	assert_type(acc.type, expected);
}

static void assert_componentType(tinygltf::Model& mod, int accidx, int expected)
{
	check_index(mod.accessors, accidx);
	auto& acc = mod.accessors[accidx];
	assert_type(acc.componentType, expected);
}

static gameObject::ptr 
load_gltf_skin_node_top(tinygltf::Model& gmod,
                        animation_map& animations,
                        int nodeidx)
{
	if (nodeidx < 0) {
		return nullptr;
	}

	// TODO: range checko
	auto& node = gmod.nodes[nodeidx];
	gameObject::ptr ret = std::make_shared<gameObject>();

	auto it = animations.find(nodeidx); if (it != animations.end()) {
		//load_gltf_animation_channels(ret, nodeidx, gmod, it->second);
		ret->animations = it->second;
	}

	return ret;
}


static gameObject::ptr 
load_gltf_skin_nodes_rec(tinygltf::Model& gmod,
                         animation_map& animations,
                         int nodeidx)
{
	if (nodeidx < 0) {
		return nullptr;
	}

	// TODO: range checko
	auto& node = gmod.nodes[nodeidx];
	gameObject::ptr ret = std::make_shared<gameObject>();

	auto it = animations.find(nodeidx); if (it != animations.end()) {
		//load_gltf_animation_channels(ret, nodeidx, gmod, it->second);
		ret->animations = it->second;
	}

	for (auto& idx : node.children) {
		auto& cnode = gmod.nodes[idx];
		std::string name = "joint["+std::to_string(idx)+"]:" + cnode.name;
		setNode(name, ret, load_gltf_skin_nodes_rec(gmod, animations, idx));
	}

	return ret;
}

static gameSkin::ptr
load_gltf_skin_nodes(tinygltf::Model& gmod,
                     animation_map& animations,
                     int nodeidx)
{
	if (nodeidx < 0) {
		return nullptr;
	}

	// TODO: range checko
	auto& skin = gmod.skins[nodeidx];
	gameSkin::ptr   obj    = std::make_shared<gameSkin>();
	gameObject::ptr sub    = std::make_shared<gameObject>();
	gameObject::ptr joints = std::make_shared<gameObject>();
	std::string sname = "skin["+skin.name+"]";
	setNode(sname, obj, sub);
	setNode("joints", sub, joints);

	/*
	for (auto& idx : skin.joints) {
		auto& cnode = gmod.nodes[idx];
		gameObject::ptr jnt = load_gltf_skin_nodes_rec(gmod, animations, idx);
		obj->joints.push_back(jnt);
	}
	*/

	std::map<int, gameObject::ptr> jointidxs;

	for (auto& idx : skin.joints) {
		gameObject::ptr jnt = load_gltf_skin_node_top(gmod, animations, idx);
		obj->joints.push_back(jnt);
		jointidxs[idx] = jnt;
	}

	for (auto& [idx, ptr] : jointidxs) {
		auto& node = gmod.nodes[idx];
		for (auto& i : node.children) {
			assert(jointidxs.find(i) != jointidxs.end());
			std::string name = "joint["+std::to_string(i)+"]:" + node.name;
			setNode(name, ptr, jointidxs[i]);
		}
	}

	for (unsigned i = 0; i < obj->joints.size(); i++) {
		auto& ptr = obj->joints[i];
		if (ptr->parent.expired()) {
			std::string name = "rootjoint["+std::to_string(i)+"]";
			setNode(name, joints, ptr);
		}
	}

	/*
	if (skin.skeleton >= 0) {
		setNode("skeleton", sub,
			load_gltf_skin_nodes_rec(gmod, animations, skin.skeleton));
	}
	*/

	assert_accessor_type(gmod, skin.inverseBindMatrices, TINYGLTF_TYPE_MAT4);
	assert_componentType(gmod, skin.inverseBindMatrices,
	                      TINYGLTF_COMPONENT_TYPE_FLOAT);
	gltf_unpack_buffer(gmod, skin.inverseBindMatrices, obj->inverseBind);

	return obj;
}

static void
set_object_gltf_transform(gameObject::ptr ptr, tinygltf::Node& node) {
	glm::quat rotation(1, 0, 0, 0);
	glm::vec3 translation = {0, 0, 0};
	glm::vec3 scale = {1, 1, 1};

	if (node.rotation.size() == 4)
		rotation = glm::make_quat(node.rotation.data());
	if (node.translation.size() == 3)
		translation = glm::make_vec3(node.translation.data());

	if (node.scale.size() == 3) {
		scale = glm::make_vec3(node.scale.data());
	}

	/*
	// TODO: if no T/R/S specifiers, show an error/warning,
	//       extract info from the matrix
	if (node.matrix.size() == 16) {
	glm::mat4 mat;
	mat = glm::make_mat4(node.matrix.data());
	}
	*/

	ptr->transform.position = translation;
	ptr->transform.rotation = rotation;
	ptr->transform.scale = scale;
}

static gameObject::ptr
load_gltf_scene_nodes_rec(tinygltf::Model& gmod,
                          modelMap& models,
                          animation_map& anims,
                          int nodeidx)
{
	// TODO: range check
	auto& node = gmod.nodes[nodeidx];
	gameObject::ptr ret = std::make_shared<gameObject>();
	set_object_gltf_transform(ret, node);

	auto it = anims.find(nodeidx); if (it != anims.end()) {
		//load_gltf_animation_channels(ret, nodeidx, gmod, it->second);
		ret->animations = it->second;
	}

	if (node.mesh >= 0) {
		// TODO: range check
		auto& mesh = gmod.meshes[node.mesh];
		setNode("mesh", ret, models[mesh.name]);
		std::cerr << "GLTF node mesh: "
			<< node.mesh << " (" << mesh.name << ")"
			<< " @ " << models[mesh.name]
			<< std::endl;
	}

	if (node.skin >= 0) {
		// TODO: range check
		auto  obj = load_gltf_skin_nodes(gmod, anims, node.skin);
		setNode("skin", ret, obj);
	}

	std::string meh = "[";
	for (auto& x : node.children) {
		meh += std::to_string(x) + ", ";
	}
	meh += "]";
	std::cerr << "GLTF node children: "
		<< std::to_string(nodeidx) << " -> " << meh
		<< std::endl;

	for (auto& x : node.children) {
		std::string id = "node["+std::to_string(x)+"]";
		setNode(id, ret, load_gltf_scene_nodes_rec(gmod, models, anims, x));
	}

	return ret;
}

static void
load_gltf_animation_channels(animationChannel::ptr cookedchan,
                             tinygltf::Model& gmod,
                             tinygltf::Animation& anim,
                             tinygltf::AnimationChannel& chan)
{
	if (chan.target_path != "translation"
		&& chan.target_path != "rotation"
		&& chan.target_path != "scale")
	{
		// ignore weight targets
		return;
	}

	// TODO: range check
	auto& sampler = anim.samplers[chan.sampler];
	assert_accessor_type(gmod, sampler.input, TINYGLTF_TYPE_SCALAR);
	assert_componentType(gmod, sampler.input,
		TINYGLTF_COMPONENT_TYPE_FLOAT);

	animation::ptr chananim;

	if (chan.target_path == "translation") {
		std::cerr << "GLTF: loading translation animation" << std::endl;
		animationTranslation::ptr objanim =
			std::make_shared<animationTranslation>();
		chananim = objanim;

		assert_accessor_type(gmod, sampler.output, TINYGLTF_TYPE_VEC3);
		assert_componentType(gmod, sampler.output,
			TINYGLTF_COMPONENT_TYPE_FLOAT);
		gltf_unpack_buffer(gmod, sampler.output, objanim->translations);
		gltf_unpack_buffer(gmod, sampler.input,  objanim->frametimes);

	} else if (chan.target_path == "rotation") {
		std::cerr << "GLTF: loading rotation animation" << std::endl;
		animationRotation::ptr objanim =
			std::make_shared<animationRotation>();
		chananim = objanim;

		assert_accessor_type(gmod, sampler.output, TINYGLTF_TYPE_VEC4);
		assert_componentType(gmod, sampler.output,
			TINYGLTF_COMPONENT_TYPE_FLOAT);
		gltf_unpack_buffer(gmod, sampler.output, objanim->rotations);
		gltf_unpack_buffer(gmod, sampler.input,  objanim->frametimes);

	} else if (chan.target_path == "scale") {
		std::cerr << "GLTF: loading scale animation" << std::endl;
		animationScale::ptr objanim =
			std::make_shared<animationScale>();
		chananim = objanim;
		
		assert_accessor_type(gmod, sampler.output, TINYGLTF_TYPE_VEC3);
		assert_componentType(gmod, sampler.output,
			TINYGLTF_COMPONENT_TYPE_FLOAT);
		gltf_unpack_buffer(gmod, sampler.output, objanim->scales);
		gltf_unpack_buffer(gmod, sampler.input,  objanim->frametimes);
	}

	cookedchan->animations.push_back(chananim);
	cookedchan->group->endtime
		= max(cookedchan->group->endtime,
			  chananim->frametimes.back());
}

static animation_map collectAnimations(tinygltf::Model& gmod) {
	animation_map ret;
	auto collection = std::make_shared<animationCollection>();

	for (unsigned i = 0; i < gmod.animations.size(); i++) {
		auto& anim = gmod.animations[i];
		auto group = std::make_shared<animationGroup>();
		// TODO: put this in constructor
		group->name = anim.name.empty()
			? "(default "+std::to_string(i) + ")"
			: anim.name;
		group->collection = collection;
		collection->groups[group->name] = group;

		// avoid adding the same animation multiple times
		// when the animation has multiple channels 
		std::set<int> chanset;

		std::cerr << "GLTF: collectAnimations(): have animation '"
			<< anim.name
			<< "'" << std::endl;

		for (auto& chan : anim.channels) {
			auto chanptr = std::make_shared<animationChannel>();
			chanptr->group = group;

			//if (!chanset.count(chan.target_node)) {
				load_gltf_animation_channels(chanptr, gmod, anim, chan);
				ret[chan.target_node].push_back(chanptr);
				chanset.insert(chan.target_node);
			//}
		}
	}

	return ret;
}

static gameImport::ptr
load_gltf_scene_nodes(std::string filename,
                      tinygltf::Model& gmod,
                      modelMap& models)
{
	gameImport::ptr ret = std::make_shared<gameImport>(filename);
	auto anims = collectAnimations(gmod);

	for (auto& scene : gmod.scenes) {
		for (int nodeidx : scene.nodes) {
			std::string id = "scene-root["+std::to_string(nodeidx)+"]";
			setNode(id, ret,
				load_gltf_scene_nodes_rec(gmod, models, anims, nodeidx));
		}
	}

	return ret;
}

static tinygltf::Model open_gltf_model(std::string filename) {
	// TODO: parse extension, handle binary format
	tinygltf::TinyGLTF loader;
	tinygltf::Model tgltf_model;
	std::string ext = filename_extension(filename);

	std::string err;
	std::string warn;
	bool loaded;

	if (ext == ".glb") {
		loaded = loader.LoadBinaryFromFile(&tgltf_model, &err, &warn, filename);

	} else {
		// assume ascii otherwise
		loaded = loader.LoadASCIIFromFile(&tgltf_model, &err, &warn, filename);
	}

	if (!loaded) {
		throw std::logic_error(err);
	}

	if (!err.empty()) {
		std::cerr << "/!\\ ERROR: " << err << std::endl;
	}

	if (!warn.empty()) {
		std::cerr << "/!\\ WARNING: " << warn << std::endl;
	}

	return tgltf_model;
}

static void updateModelSources(grendx::modelMap& models, std::string filename) {
	for (auto& [name, model] : models) {
		model->sourceFile = filename;
	}
}

grendx::modelMap grendx::load_gltf_models(std::string filename) {
	tinygltf::Model gmod = open_gltf_model(filename);
	auto models = load_gltf_models(gmod);
	std::cerr << " GLTF > loaded a thing successfully" << std::endl;

	updateModelSources(models, filename);
	return models;
}

std::pair<grendx::gameImport::ptr, grendx::modelMap>
grendx::load_gltf_scene(std::string filename) {
	tinygltf::Model gmod = open_gltf_model(filename);
	grendx::modelMap models = load_gltf_models(gmod);
	gameImport::ptr ret = load_gltf_scene_nodes(filename, gmod, models);

	updateModelSources(models, filename);
	return {ret, models};
}
