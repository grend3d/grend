#include <grend/gameModel.hpp>
#include <grend/utility.hpp>
#include <tinygltf/tiny_gltf.h>

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

class gltfModel {
	public:
		gltfModel(tinygltf::Model& mod, std::string fname)
			: data(mod), filename(fname) {};
		tinygltf::Model data;
		std::string filename;

		std::map<int, materialTexture::weakptr> texcache;
		std::map<int, material::weakptr>        matcache;
};

namespace grendx {
// XXX: ...
modelMap load_gltf_models(gltfModel& gltf);
}

template <class T>
class accessorIterator {
	public:
		uint8_t *buffer;
		size_t index = 0;
		size_t end;
		size_t elementSize;

		accessorIterator()
			: buffer(nullptr), end(0), elementSize(0) {};
		accessorIterator(uint8_t *_buffer, size_t _end, size_t _emSize)
			: buffer(_buffer), end(_end), elementSize(_emSize) {};

		accessorIterator& operator++(int) {
			if (index < end) {
				index++;
			}

			return *this;
		}

		bool atEnd(void) {
			return index >= end;
		}

		T& operator*() {
			return *reinterpret_cast<T*>(buffer + index*elementSize);
		}

};

template <typename T>
static inline bool check_index(const std::vector<T>& container, size_t idx) {
	if (idx >= container.size()) {
		// TODO: toggle exceptions somehow
		SDL_Log("EXCEPTION: check_index()");
		throw std::out_of_range("check_index()");
		return false;
	}

	return true;
}

static inline bool assert_type(int type, int expected) {
	if (type != expected) {
		SDL_Log("EXCEPTION: assert_type()");
		throw std::invalid_argument(
			"assert_type(): have " + std::to_string(type)
			+ ", expected " + std::to_string(expected));
		return false;
	}

	return true;
}

static inline bool assert_nonzero(size_t x) {
	if (x == 0) {
		SDL_Log("EXCEPTION: assert_nonzero()");
		throw std::logic_error("assert_nonzero()");
		return false;
	}

	return true;
}

static tinygltf::Material& gltf_material(gltfModel& gltf, size_t idx) {
	check_index(gltf.data.materials, idx);
	return gltf.data.materials[idx];
}

static tinygltf::Texture& gltf_texture(gltfModel& gltf, size_t idx) {
	check_index(gltf.data.textures, idx);
	return gltf.data.textures[idx];
}

static tinygltf::Image& gltf_image(gltfModel& gltf, size_t idx) {
	check_index(gltf.data.textures, idx);
	return gltf.data.images[idx];
}

static tinygltf::Sampler& gltf_sampler(gltfModel& gltf, size_t idx) {
	check_index(gltf.data.samplers, idx);
	return gltf.data.samplers[idx];
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

#ifdef GL_DOUBLE
		case TINYGLTF_COMPONENT_TYPE_DOUBLE:
			size_component = sizeof(GLdouble); 
			break;
#endif
	}

	return size_component * size_type;
}

static void *gltf_load_accessor(gltfModel& gltf, int idx) {
	check_index(gltf.data.accessors, idx);
	auto& acc = gltf.data.accessors[idx];

	size_t elem_size =
		gltf_buff_element_size(acc.componentType, acc.type);
	size_t buff_size = acc.count * elem_size;
	assert_nonzero(elem_size);

	check_index(gltf.data.bufferViews, acc.bufferView);
	auto& view = gltf.data.bufferViews[acc.bufferView];
	check_index(gltf.data.buffers, view.buffer);
	auto& buffer = gltf.data.buffers[view.buffer];

	check_index(buffer.data,
			view.byteOffset + acc.byteOffset + buff_size - 1);
	return static_cast<void*>(buffer.data.data() + (view.byteOffset + acc.byteOffset));
};

template<typename T>
static void gltf_unpack_buffer(gltfModel& gltf,
                               int accessor,
                               std::vector<T>& vec)
{
	check_index(gltf.data.accessors, accessor);
	auto& acc = gltf.data.accessors[accessor];
	check_index(gltf.data.bufferViews, acc.bufferView);
	auto& view = gltf.data.bufferViews[acc.bufferView];

	uint8_t *datas = static_cast<uint8_t*>(gltf_load_accessor(gltf, accessor));
	size_t emsz = view.byteStride
		? view.byteStride
		: gltf_buff_element_size(acc.componentType, acc.type);

	for (unsigned i = 0; i < acc.count; i++) {
		T *em = reinterpret_cast<T*>(datas + i*emsz);
		vec.push_back(*em);
	}
}

template<typename T>
static accessorIterator<T>
gltf_buffer_iterator(gltfModel& gltf, int accessor) {
	check_index(gltf.data.accessors, accessor);
	auto& acc = gltf.data.accessors[accessor];
	check_index(gltf.data.bufferViews, acc.bufferView);
	auto& view = gltf.data.bufferViews[acc.bufferView];

	uint8_t *datas = static_cast<uint8_t*>(gltf_load_accessor(gltf, accessor));
	size_t emsz = view.byteStride
		? view.byteStride
		: gltf_buff_element_size(acc.componentType, acc.type);


	return accessorIterator<T>(datas, acc.count, emsz);
}

static void gltf_unpack_ushort_to_uint(gltfModel& gltf,
                                       int accessor,
                                       std::vector<GLuint>& vec)
{
	check_index(gltf.data.accessors, accessor);
	auto& acc = gltf.data.accessors[accessor];
	check_index(gltf.data.bufferViews, acc.bufferView);
	auto& view = gltf.data.bufferViews[acc.bufferView];

	uint8_t *datas = static_cast<uint8_t*>(gltf_load_accessor(gltf, accessor));
	size_t emsz = view.byteStride
		? view.byteStride
		: gltf_buff_element_size(acc.componentType, acc.type);

	for (unsigned i = 0; i < acc.count; i++) {
		//T *em = reinterpret_cast<T*>(datas + i*emsz);
		GLushort *em = reinterpret_cast<GLushort*>(datas + i*emsz);
		vec.push_back((GLuint)*em);
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

// TODO: make this a generic function, .obj loader needs this too
void gltf_load_external(gltfModel& gltf, materialTexture::ptr tex, std::string uri)
{
	std::string dir  = dirnameStr(gltf.filename);
	std::string texname = dir + "/" + uri;
	std::string vecname = texname + ".vectex";
	// TODO: pkm, ktx, uuuuuuhhhhh.....
	std::string pkmname = texname + ".pkm";

	uint8_t *px = nullptr;

	// try vector texture first
	// TODO: might want to split these into multiple functions
	px = stbi_load(vecname.c_str(), &tex->width, &tex->height,
	               &tex->channels, 0);
	tex->type = materialTexture::imageType::VecTex;

	if (!px) {
		// otherwise try plain image
		px = stbi_load(texname.c_str(), &tex->width, &tex->height,
		               &tex->channels, 0);
		tex->type = materialTexture::imageType::Plain;
	}

	if (!px) {
		SDL_Log("Couldn't load texture: uri: %s, location: %s\n",
				uri.c_str(), texname.c_str());
		return;
	}

	size_t size = (tex->channels * tex->width) * tex->height;
	tex->pixels.insert(tex->pixels.end(), px, px + size);
	tex->size = size;

	SDL_Log("Loaded external texture: uri: %s, size: %lu, location: %s\n",
	        uri.c_str(), size, texname.c_str());

	stbi_image_free(px);
}

static materialTexture::ptr gltf_load_texture(gltfModel& gltf, int tex_idx) {
	materialTexture::ptr ret;

	if (gltf.texcache.count(tex_idx) && (ret = gltf.texcache[tex_idx].lock())) {
		return ret;
	}

	gltf.texcache[tex_idx] = ret = std::make_shared<materialTexture>();

	auto& tex = gltf_texture(gltf, tex_idx);
	//std::cerr << "        + texture source: " << tex.source << std::endl;

	if (tex.source >= 0) {
		auto& img = gltf_image(gltf, tex.source);
		std::cerr << "        + texture image source: " << img.uri << ", "
			<< img.width << "x" << img.height << ":" << img.component << std::endl;

		if (img.component < 0 || img.height < 0 || img.width < 0) {
			// external image, do checks for compressed formats, etc
			gltf_load_external(gltf, ret, img.uri);

		} else {
			ret->pixels.insert(ret->pixels.end(),
			                   img.image.begin(), img.image.end());

			ret->width    = img.width;
			ret->height   = img.height;
			ret->channels = img.component;
			ret->size     = ret->width * ret->height * ret->channels;
		}
	}

	if (tex.sampler >= 0) {
		auto& sampler = gltf_sampler(gltf, tex.sampler);

		switch (sampler.minFilter) {
			case TINYGLTF_TEXTURE_FILTER_NEAREST:
				ret->minFilter = materialTexture::filter::Nearest;
				break;
			case TINYGLTF_TEXTURE_FILTER_LINEAR:
				ret->minFilter = materialTexture::filter::Linear;
				break;
			case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST:
				ret->minFilter = materialTexture::filter::NearestMipmapNearest;
				break;
			case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_LINEAR:
				ret->minFilter = materialTexture::filter::NearestMipmapLinear;
				break;
			case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_NEAREST:
				ret->minFilter = materialTexture::filter::LinearMipmapNearest;
				break;
			case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR:
				ret->minFilter = materialTexture::filter::LinearMipmapLinear;
				break;
			default: break;
		}

		switch (sampler.magFilter) {
			case TINYGLTF_TEXTURE_FILTER_NEAREST:
				ret->magFilter = materialTexture::filter::Nearest;
				break;
			case TINYGLTF_TEXTURE_FILTER_LINEAR:
				ret->magFilter = materialTexture::filter::Linear;
				break;
			default: break;
		}

		// TODO: wrap
	}

	return ret;
}

static material::ptr gltf_load_material(gltfModel& gltf, int material_idx) {
	material::ptr ret;

	if (gltf.matcache.count(material_idx)
	    && (ret = gltf.matcache[material_idx].lock()))
	{
		return ret;

	} else {
		gltf.matcache[material_idx] = ret = std::make_shared<grendx::material>();
		auto& mat = gltf_material(gltf, material_idx);

		auto& pbr = mat.pbrMetallicRoughness;
		auto& base_color = pbr.baseColorFactor;
		glm::vec4 mat_diffuse(base_color[0], base_color[1],
				base_color[2], base_color[3]);

		if (pbr.baseColorTexture.index >= 0) {
			ret->maps.diffuse =
				gltf_load_texture(gltf, pbr.baseColorTexture.index);
		}

		if (pbr.metallicRoughnessTexture.index >= 0) {
			ret->maps.metalRoughness =
				gltf_load_texture(gltf, pbr.metallicRoughnessTexture.index);
		}


		if (mat.normalTexture.index >= 0) {
			ret->maps.normal =
				gltf_load_texture(gltf, mat.normalTexture.index);
		}

		if (mat.occlusionTexture.index >= 0) {
			ret->maps.ambientOcclusion =
				gltf_load_texture(gltf, mat.occlusionTexture.index);
		}

		if (mat.emissiveTexture.index >= 0) {
			ret->maps.emissive =
				gltf_load_texture(gltf, mat.emissiveTexture.index);
		}

		if (mat.alphaMode == "BLEND") {
			ret->factors.blend = grendx::material::blend_mode::Blend;

		} else if (mat.alphaMode == "MASK") {
			ret->factors.blend = grendx::material::blend_mode::Mask;
		}

		auto& ev = mat.emissiveFactor;
		ret->factors.emissive  = glm::vec4(ev[0], ev[1], ev[2], 1.0);
		ret->factors.roughness = pbr.roughnessFactor;
		ret->factors.metalness = pbr.metallicFactor;
		ret->factors.diffuse   = mat_diffuse;
		// XXX: is there really a point in having RGBA base color and an
		//      opacity field...
		ret->factors.opacity = mat_diffuse[3];
		// TODO: alpha cutoff field
		//ret->factors.opacity = mat.alphaCutoff;

		return ret;
	}
}

materialTexture::ptr load_gltf_lightmap(gltfModel& gltf) {
	for (auto& img : gltf.data.images) {
		if (img.name.find("grendLightmap") != std::string::npos) {
			std::cerr << " GLTF > have lightmap: "
				" name: " << img.name << ", "
				" uri: " << img.uri
				<< " (" << img.mimeType << ")"
				<< std::endl;

			materialTexture::ptr ret = std::make_shared<materialTexture>();
			ret->pixels.insert(ret->pixels.end(), img.image.begin(), img.image.end());
			ret->width = img.width;
			ret->height = img.height;
			ret->channels = img.component;
			ret->size = ret->width * ret->height * ret->channels;
			
			return ret;
		}
	}

	return nullptr;
}

grendx::modelMap grendx::load_gltf_models(gltfModel& gltf) {
	modelMap ret;
	materialTexture::ptr lightmap = load_gltf_lightmap(gltf);

	for (auto& mesh : gltf.data.meshes) {
		grendx::gameModel::ptr curModel =
			grendx::gameModel::ptr(new grendx::gameModel());
		ret[mesh.name] = curModel;

		/*
		std::cerr << " GLTF > have mesh " << mesh.name << std::endl;
		std::cerr << "      > primitives: " << std::endl;
		*/

		for (size_t i = 0; i < mesh.primitives.size(); i++) {
			auto& prim = mesh.primitives[i];
			std::string suffix = ":[p"+std::to_string(i)+"]";
			std::string temp_name = mesh.name + suffix
				+ ":" + std::to_string(prim.material);

			/*
			std::cerr << "        primitive: " << temp_name << std::endl;
			std::cerr << "        material: " << prim.material << std::endl;
			std::cerr << "        indices: " << prim.indices << std::endl;
			std::cerr << "        mode: " << prim.mode << std::endl;
			*/

			grendx::gameMesh::ptr modmesh = grendx::gameMesh::ptr(new grendx::gameMesh());
			setNode(temp_name, curModel, modmesh);

			// copy over extra property values
			for (const auto& name : mesh.extras.Keys()) {
				tinygltf::Value foo = mesh.extras.Get(name);

				if (foo.IsNumber()) {
					modmesh->extraProperties[name] = (float)foo.GetNumberAsDouble();
				}
			}

			if (prim.material >= 0) {
				modmesh->meshMaterial
					= gltf_load_material(gltf, prim.material);
			} else {
				// XXX: a little wasteful
				modmesh->meshMaterial = std::make_shared<material>();
			}

			modmesh->meshMaterial->maps.lightmap = lightmap;

			// accessor indices
			int elements = prim.indices;
			int normals = -1;
			int position = -1;
			int tangents = -1;
			int colors = -1;
			int texcoord = -1;
			int lightmap = -1;

			int joints = -1;
			int weights = -1;

			for (auto& attr : prim.attributes) {
				if (attr.first == "NORMAL")     normals = attr.second;
				if (attr.first == "POSITION")   position = attr.second;
				if (attr.first == "TANGENT")    tangents = attr.second;
				if (attr.first == "COLOR_0")    colors = attr.second;
				if (attr.first == "TEXCOORD_0") texcoord = attr.second;

				// XXX: if there's a texcoord 1, that's the material UV map,
				//      first is the lightmap UVs... quirk of how blender does
				//      it's automatic lightmap UV map generation, the generated
				//      UV map gets put into the first slot, the original is put
				//      in the second.
				//
				//      also assumes texcoord definitions are sequential,
				//      not sure if the spec requires that, would be good
				//      to handle out-of-order definitions anyway, TODO for
				//      another day
				if (attr.first == "TEXCOORD_1") {
					// throw an error just in case that assumption is wrong...
					assert_nonzero(texcoord != -1);
					lightmap = texcoord;
					texcoord = attr.second;
				}

				if (attr.first == "JOINTS_0")   joints = attr.second; 
				if (attr.first == "WEIGHTS_0")  weights = attr.second; 
			}

			if (position < 0) {
				SDL_Log("EXCEPTION: load_gltf_models() can't load a mesh primitive without position information");
				throw std::logic_error(
					"load_gltf_models(): can't load a mesh primitive "
					"without position information");
			}

			if (elements >= 0) {
				check_index(gltf.data.accessors, elements);

				auto& acc = gltf.data.accessors[elements];
				assert_type(acc.type, TINYGLTF_TYPE_SCALAR);

				size_t vsize = curModel->vertices.size();
				auto& submesh = modmesh->faces;

				if (acc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
					gltf_unpack_ushort_to_uint(gltf, elements, submesh);
				} else if (acc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) {
					gltf_unpack_buffer(gltf, elements, submesh);
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
				// std::cerr << "        generating indices..." << std::endl;
				auto& acc = gltf.data.accessors[position];
				auto& submesh = modmesh->faces;
				size_t vsize = curModel->vertices.size();

				for (size_t i = 0; i < acc.count; i++) {
					submesh.push_back(i + vsize);
				}
			}

			accessorIterator<usvec4>    colorIt;
			accessorIterator<glm::vec3> positionIt;
			accessorIterator<glm::vec3> normalIt;
			accessorIterator<glm::vec4> tangentIt;
			accessorIterator<glm::vec2> uvIt;
			accessorIterator<glm::vec2> litIt;

			if (normals >= 0) {
				check_index(gltf.data.accessors, normals);

				auto& acc = gltf.data.accessors[normals];
				assert_type(acc.type, TINYGLTF_TYPE_VEC3);
				assert_type(acc.componentType, TINYGLTF_COMPONENT_TYPE_FLOAT);

				//gltf_unpack_buffer(gltf, normals, curModel->normals);
				normalIt = gltf_buffer_iterator<glm::vec3>(gltf, normals);
				curModel->haveNormals = true;
			}

			if (tangents >= 0) {
				check_index(gltf.data.accessors, tangents);
				// std::cerr << "        have tangents... " << tangents << std::endl;

				auto& acc = gltf.data.accessors[tangents];
				assert_type(acc.type, TINYGLTF_TYPE_VEC4);
				assert_type(acc.componentType, TINYGLTF_COMPONENT_TYPE_FLOAT);

				//gltf_unpack_buffer(gltf, tangents, curModel->tangents);
				// TODO: tangent is vec4!
				tangentIt = gltf_buffer_iterator<glm::vec4>(gltf, tangents);
				curModel->haveTangents = true;
			}

			if (colors >= 0) {
				check_index(gltf.data.accessors, colors);
				// std::cerr << "        have colors... " << colors << std::endl;

				auto& acc = gltf.data.accessors[colors];
				// TODO: also vec3
				assert_type(acc.type, TINYGLTF_TYPE_VEC4);
				// TODO: need to handle float and byte types
				assert_type(acc.componentType, TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT);

				//gltf_unpack_buffer(gltf, colors, curModel->colors);
				colorIt = gltf_buffer_iterator<usvec4>(gltf, colors);
				curModel->haveColors = true;
			}

			if (position >= 0) {
				check_index(gltf.data.accessors, position);

				auto& acc = gltf.data.accessors[position];
				assert_type(acc.type, TINYGLTF_TYPE_VEC3);
				assert_type(acc.componentType, TINYGLTF_COMPONENT_TYPE_FLOAT);

				//gltf_unpack_buffer(gltf, position, curModel->vertices);
				positionIt = gltf_buffer_iterator<glm::vec3>(gltf, position);

				if (auto box = gltf_accessor_aabb(acc)) {
					curModel->haveAABB = true;
					modmesh->boundingBox = *box;
				}
			}

			if (texcoord >= 0) {
				check_index(gltf.data.accessors, texcoord);

				auto& acc = gltf.data.accessors[texcoord];
				assert_type(acc.type, TINYGLTF_TYPE_VEC2);
				assert_type(acc.componentType, TINYGLTF_COMPONENT_TYPE_FLOAT);

				//gltf_unpack_buffer(gltf, texcoord, curModel->texcoords);
				uvIt = gltf_buffer_iterator<glm::vec2>(gltf, texcoord);
				curModel->haveTexcoords = true;
			}

			if (lightmap >= 0) {
				check_index(gltf.data.accessors, lightmap);

				auto& acc = gltf.data.accessors[lightmap];
				assert_type(acc.type, TINYGLTF_TYPE_VEC2);
				assert_type(acc.componentType, TINYGLTF_COMPONENT_TYPE_FLOAT);

				//gltf_unpack_buffer(gltf, lightmap, curModel->lightmap);
				litIt = gltf_buffer_iterator<glm::vec2>(gltf, lightmap);
				curModel->haveLightmap = true;
			}

			for (; !positionIt.atEnd(); positionIt++) {
				gameModel::vertex vert;

				vert.position = *positionIt;
				if (!normalIt.atEnd())  {vert.normal   = *normalIt;  normalIt++;};
				if (!tangentIt.atEnd()) {vert.tangent  = *tangentIt; tangentIt++;};
				if (!colorIt.atEnd())   {
					// XXX
					auto it = *colorIt;
					glm::vec4 c(it.x, it.y, it.z, it.w);
					c = c / 65536.f;
					vert.color = c;
					colorIt++;
				} else {
					vert.color = glm::vec4(1.f);
				}
				if (!uvIt.atEnd())      {vert.uv       = *uvIt;      uvIt++;};
				if (!litIt.atEnd())     {vert.lightmap = *litIt;     litIt++;};

				curModel->vertices.push_back(vert);
			}

			accessorIterator<usvec4> jointItShort;
			accessorIterator<ubvec4> jointItByte;
			unsigned jointType = 0;

			accessorIterator<glm::vec4> weightIt;

			if (joints >= 0 && weights >= 0) {
				check_index(gltf.data.accessors, joints);
				check_index(gltf.data.accessors, weights);
				auto& jac = gltf.data.accessors[joints];
				auto& wac = gltf.data.accessors[weights];

				assert_type(jac.type, TINYGLTF_TYPE_VEC4);
				assert_type(wac.type, TINYGLTF_TYPE_VEC4);

				if (jac.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) {
					jointType = jac.componentType;
					jointItByte = gltf_buffer_iterator<ubvec4>(gltf, joints);
				} else if (jac.componentType
				           == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
				{
					jointType = jac.componentType;
					jointItShort = gltf_buffer_iterator<usvec4>(gltf, joints);
				}

				//assert_type(jac.componentType,
				//	TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT);

				assert_type(wac.componentType,
					TINYGLTF_COMPONENT_TYPE_FLOAT);

				weightIt = gltf_buffer_iterator<glm::vec4>(gltf, weights);
				curModel->haveJoints = true;
				// std::cerr << "        have joints: " << joints << std::endl;
			}

			for (; !weightIt.atEnd(); weightIt++) {
				gameModel::jointWeights joint;

				if (jointType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) {
					if (!jointItByte.atEnd()) {
						joint.joints = *jointItByte;
						jointItByte++;
					}

				} else if (jointType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
					if (!jointItShort.atEnd()) {
						joint.joints = *jointItShort;
						jointItShort++;
					}
				}

				joint.weights = *weightIt;
				curModel->joints.push_back(joint);
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

// TODO: camelcase?
typedef std::map<int, std::vector<animationChannel::ptr>> animation_map;
typedef std::map<std::string, gameObject::ptr> node_map;

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
load_gltf_skin_node_top(gltfModel& gltf,
                        animation_map& animations,
                        int nodeidx)
{
	if (nodeidx < 0) {
		return nullptr;
	}

	gameObject::ptr ret = std::make_shared<gameObject>();

	auto it = animations.find(nodeidx);
	if (it != animations.end()) {
		//load_gltf_animation_channels(ret, nodeidx, gltf, it->second);
		ret->animations = it->second;
	}

	return ret;
}


static gameObject::ptr 
load_gltf_skin_nodes_rec(gltfModel& gltf,
                         animation_map& animations,
                         int nodeidx)
{
	if (nodeidx < 0) {
		return nullptr;
	}

	// TODO: range checko
	auto& node = gltf.data.nodes[nodeidx];
	gameObject::ptr ret = std::make_shared<gameObject>();

	auto it = animations.find(nodeidx); if (it != animations.end()) {
		//load_gltf_animation_channels(ret, nodeidx, gltf, it->second);
		ret->animations = it->second;
	}

	for (auto& idx : node.children) {
		auto& cnode = gltf.data.nodes[idx];
		std::string name = "joint["+std::to_string(idx)+"]:" + cnode.name;
		setNode(name, ret, load_gltf_skin_nodes_rec(gltf, animations, idx));
	}

	return ret;
}

static gameSkin::ptr
load_gltf_skin_nodes(gltfModel& gltf,
                     animation_map& animations,
                     int nodeidx)
{
	if (nodeidx < 0) {
		return nullptr;
	}

	// TODO: range checko
	auto& skin = gltf.data.skins[nodeidx];
	gameSkin::ptr   obj    = std::make_shared<gameSkin>();
	gameObject::ptr sub    = std::make_shared<gameObject>();
	gameObject::ptr joints = std::make_shared<gameObject>();
	std::string sname = "skin["+skin.name+"]";
	setNode(sname, obj, sub);
	setNode("joints", sub, joints);

	/*
	for (auto& idx : skin.joints) {
		auto& cnode = gltf.data.nodes[idx];
		gameObject::ptr jnt = load_gltf_skin_nodes_rec(gltf, animations, idx);
		obj->joints.push_back(jnt);
	}
	*/

	std::map<int, gameObject::ptr> jointidxs;

	for (auto& idx : skin.joints) {
		gameObject::ptr jnt = load_gltf_skin_node_top(gltf, animations, idx);
		obj->joints.push_back(jnt);
		jointidxs[idx] = jnt;
	}

	for (auto& [idx, ptr] : jointidxs) {
		auto& node = gltf.data.nodes[idx];
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
			load_gltf_skin_nodes_rec(gltf, animations, skin.skeleton));
	}
	*/

	assert_accessor_type(gltf.data, skin.inverseBindMatrices, TINYGLTF_TYPE_MAT4);
	assert_componentType(gltf.data, skin.inverseBindMatrices,
	                      TINYGLTF_COMPONENT_TYPE_FLOAT);
	gltf_unpack_buffer(gltf, skin.inverseBindMatrices, obj->inverseBind);

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

	TRS newtrans;
	newtrans.position = translation;
	newtrans.rotation = rotation;
	newtrans.scale = scale;
	ptr->setTransform(newtrans);
}

static void load_gltf_node_light(gltfModel& gltf,
                                 tinygltf::Node& node,
                                 gameObject::ptr obj)
{
	auto it = node.extensions.find("KHR_lights_punctual");
	if (it != node.extensions.end()) {
		std::cerr << " GLTF > have punctual light!" << std::endl;

		if (!it->second.IsObject() || !it->second.Has("light")) {
			std::cerr << " GLTF > don't have light object" << std::endl;
			return;
		}

		auto val = it->second.Get("light");
		if (!val.IsInt()) {
			std::cerr << " GLTF > invalid light object index type" << std::endl;
			return;
		}

		int idx = val.GetNumberAsInt();

		if ((unsigned)idx >= gltf.data.lights.size()) {
			std::cerr << " GLTF > invalid light object index" << std::endl;
			return;
		}

		auto& light = gltf.data.lights[idx];
		std::cerr << " GLTF > node light! " << light.name << ": " << light.type << std::endl;

		if (light.type == "point") {
			gameLightPoint::ptr point = std::make_shared<gameLightPoint>();
			point->intensity = light.intensity;
			point->casts_shadows = true;
			// XXX: 0.25 seems to be the blender default, and the punctual lights
			//      gltf extension doesn't store this info...
			point->radius = 0.25;
			point->diffuse =
				glm::vec4(light.color[0], light.color[1], light.color[2], 1.0);

			setNode(light.name, obj, point);

		} else if (light.type == "spot") {
			gameLightSpot::ptr spot = std::make_shared<gameLightSpot>();
			spot->intensity = light.intensity;
			spot->casts_shadows = true;
			spot->radius = 0.25;
			spot->diffuse =
				glm::vec4(light.color[0], light.color[1], light.color[2], 1.0);
			// TODO: inner cone angle, blend(?)
			spot->angle = light.spot.outerConeAngle;

			setNode(light.name, obj, spot);

		} else {
			std::cerr << " GLTF > don't know how to handle node light of type "
				<< light.type << std::endl;
		}
	}
}

static gameObject::ptr
load_gltf_scene_nodes_rec(gltfModel& gltf,
                          modelMap& models,
                          animation_map& anims,
                          node_map& names,
                          int nodeidx)
{
	// TODO: range check
	auto& node = gltf.data.nodes[nodeidx];
	gameObject::ptr ret = std::make_shared<gameObject>();
	set_object_gltf_transform(ret, node);
	load_gltf_node_light(gltf, node, ret);

	auto it = anims.find(nodeidx); if (it != anims.end()) {
		//load_gltf_animation_channels(ret, nodeidx, gltf, it->second);
		ret->animations = it->second;
	}

	if (node.mesh >= 0 && (unsigned)node.mesh < gltf.data.meshes.size()) {
		auto& mesh = gltf.data.meshes[node.mesh];

		if (models.count(mesh.name)) {
			setNode("mesh", ret, models[mesh.name]);

		} else {
			SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
			            "Invalid model name '%s'!", mesh.name.c_str());
		}
	}

	if (node.skin >= 0) {
		// TODO: range check
		// TODO: get skin node names
		auto  obj = load_gltf_skin_nodes(gltf, anims, node.skin);
		setNode("skin", ret, obj);
	}

	std::string meh = "[";
	for (auto& x : node.children) {
		meh += std::to_string(x) + ", ";
	}
	meh += "]";

	for (auto& x : node.children) {
		std::string id = "node["+std::to_string(x)+"]";
		setNode(id, ret, load_gltf_scene_nodes_rec(gltf, models, anims, names, x));
	}

	if (!node.name.empty()) {
		if (!names.count(node.name)) {
			names[node.name] = ret;

		} else {
			SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
			            "Repeated name '%s', ignoring...", node.name.c_str());
		}
	}

	return ret;
}

static void
load_gltf_animation_channels(animationChannel::ptr cookedchan,
                             gltfModel& gltf,
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
	assert_accessor_type(gltf.data, sampler.input, TINYGLTF_TYPE_SCALAR);
	assert_componentType(gltf.data, sampler.input,
		TINYGLTF_COMPONENT_TYPE_FLOAT);

	animation::ptr chananim;

	if (chan.target_path == "translation") {
		// std::cerr << "GLTF: loading translation animation" << std::endl;
		animationTranslation::ptr objanim =
			std::make_shared<animationTranslation>();
		chananim = objanim;

		assert_accessor_type(gltf.data, sampler.output, TINYGLTF_TYPE_VEC3);
		assert_componentType(gltf.data, sampler.output,
			TINYGLTF_COMPONENT_TYPE_FLOAT);
		gltf_unpack_buffer(gltf, sampler.output, objanim->translations);
		gltf_unpack_buffer(gltf, sampler.input,  objanim->frametimes);

	} else if (chan.target_path == "rotation") {
		// std::cerr << "GLTF: loading rotation animation" << std::endl;
		animationRotation::ptr objanim =
			std::make_shared<animationRotation>();
		chananim = objanim;

		assert_accessor_type(gltf.data, sampler.output, TINYGLTF_TYPE_VEC4);
		assert_componentType(gltf.data, sampler.output,
			TINYGLTF_COMPONENT_TYPE_FLOAT);
		gltf_unpack_buffer(gltf, sampler.output, objanim->rotations);
		gltf_unpack_buffer(gltf, sampler.input,  objanim->frametimes);

	} else if (chan.target_path == "scale") {
		// std::cerr << "GLTF: loading scale animation" << std::endl;
		animationScale::ptr objanim =
			std::make_shared<animationScale>();
		chananim = objanim;
		
		assert_accessor_type(gltf.data, sampler.output, TINYGLTF_TYPE_VEC3);
		assert_componentType(gltf.data, sampler.output,
			TINYGLTF_COMPONENT_TYPE_FLOAT);
		gltf_unpack_buffer(gltf, sampler.output, objanim->scales);
		gltf_unpack_buffer(gltf, sampler.input,  objanim->frametimes);
	}

	cookedchan->animations.push_back(chananim);
	cookedchan->group->endtime
		= max(cookedchan->group->endtime,
			  chananim->frametimes.back());
}

static animation_map collectAnimations(gltfModel& gltf) {
	animation_map ret;
	auto collection = std::make_shared<animationCollection>();

	for (unsigned i = 0; i < gltf.data.animations.size(); i++) {
		auto& anim = gltf.data.animations[i];
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

		/*
		std::cerr << "GLTF: collectAnimations(): have animation '"
			<< anim.name
			<< "'" << std::endl;
			*/

		for (auto& chan : anim.channels) {
			auto chanptr = std::make_shared<animationChannel>();
			chanptr->group = group;

			//if (!chanset.count(chan.target_node)) {
				load_gltf_animation_channels(chanptr, gltf, anim, chan);
				ret[chan.target_node].push_back(chanptr);
				chanset.insert(chan.target_node);
			//}
		}
	}

	return ret;
}

static gameImport::ptr
load_gltf_scene_nodes(std::string filename,
                      gltfModel& gltf,
                      modelMap& models)
{
	gameImport::ptr ret = std::make_shared<gameImport>(filename);
	auto anims = collectAnimations(gltf);
	node_map names;

	gameImport::ptr sceneobj = std::make_shared<gameImport>(filename);
	for (auto& scene : gltf.data.scenes) {
		for (int nodeidx : scene.nodes) {
			std::string id = "scene-root["+std::to_string(nodeidx)+"]";
			setNode(id, sceneobj,
				load_gltf_scene_nodes_rec(gltf, models, anims, names, nodeidx));
		}
	}

	auto nameobj = std::make_shared<gameObject>();
	nameobj->visible = false;

	for (auto& [name, obj] : names) {
		setNode(name, nameobj, obj);
	}

	setNode("names", ret, nameobj);
	setNode("scene", ret, sceneobj);

	return ret;
}

static gltfModel open_gltf_model(std::string filename) {
	// TODO: parse extension, handle binary format
	tinygltf::TinyGLTF loader;
	tinygltf::Model gltf;
	std::string ext = filename_extension(filename);

	std::string err;
	std::string warn;
	bool loaded;

	if (ext == ".glb") {
		loaded = loader.LoadBinaryFromFile(&gltf, &err, &warn, filename);

	} else {
		// assume ascii otherwise
		loaded = loader.LoadASCIIFromFile(&gltf, &err, &warn, filename);
	}

	if (!loaded) {
		SDL_Log("EXCEPTION: %s", err.c_str());
		throw std::logic_error(err);
	}

	if (!err.empty()) {
		std::cerr << "/!\\ ERROR: " << err << std::endl;
	}

	if (!warn.empty()) {
		std::cerr << "/!\\ WARNING: " << warn << std::endl;
	}

	return gltfModel(gltf, filename);
}

static void updateModelSources(grendx::modelMap& models, std::string filename) {
	for (auto& [name, model] : models) {
		model->sourceFile = filename;
	}
}

grendx::modelMap grendx::load_gltf_models(std::string filename) {
	gltfModel gltf = open_gltf_model(filename);
	auto models = load_gltf_models(gltf);
	std::cerr << " GLTF > loaded a thing successfully" << std::endl;

	updateModelSources(models, filename);
	return models;
}

std::pair<grendx::gameImport::ptr, grendx::modelMap>
grendx::load_gltf_scene(std::string filename) {
	SDL_Log("Opening gltf scene %s...", filename.c_str());
	gltfModel gltf = open_gltf_model(filename);
	SDL_Log("Loading gltf scene %s...", filename.c_str());
	grendx::modelMap models = load_gltf_models(gltf);
	SDL_Log("Loading gltf scene nodes %s...", filename.c_str());
	gameImport::ptr ret = load_gltf_scene_nodes(filename, gltf, models);

	SDL_Log("updating sources %s...", filename.c_str());
	updateModelSources(models, filename);
	SDL_Log("done loading %s", filename.c_str());
	return {ret, models};
}
