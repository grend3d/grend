#include <grend/geometryGeneration.hpp>
#include <grend/sceneModel.hpp>

namespace grendx {

sceneModel::ptr generatePlaneMesh(int sx, int sy, int w, int h) {
	auto ecs = engine::Resolve<ecs::entityManager>();

	sceneModel::ptr ret  = ecs->construct<sceneModel>();
	sceneMesh::ptr  mesh = ecs->construct<sceneMesh>();

	setNode("mesh", ret, mesh);

	return ret;
}

sceneModel::ptr generateHeightmap(float width, float height, float unitsPerVert,
                                 float x, float y, heightFunction func)
{
	auto ecs = engine::Resolve<ecs::entityManager>();

	sceneModel::ptr ret  = ecs->construct<sceneModel>();
	sceneMesh::ptr  mesh = ecs->construct<sceneMesh>();

	unsigned xverts = width / unitsPerVert + 1;
	unsigned yverts = height / unitsPerVert + 1;

	for (unsigned k = 0; k < xverts; k++) {
		for (unsigned j = 0; j < yverts; j++) {
			float vx = k * unitsPerVert;
			float vz = j * unitsPerVert;
			float height = func(x + vx, y + vz);

			ret->vertices.push_back((sceneModel::vertex) {
				.position = {vx, height, vz},
				.uv       = {vx / 12.0, vz / 12.0},
			});
		}
	}

	for (unsigned k = 0; k + 1 < xverts; k++) {
		for (unsigned j = 0; j + 1 < yverts; j++) {
			GLuint vs[4] = {
				j*xverts + k,
				j*xverts + k + 1,
				(j + 1)*xverts + k,
				(j + 1)*xverts + k + 1,
			};

			mesh->faces.push_back(vs[0]);
			mesh->faces.push_back(vs[1]);
			mesh->faces.push_back(vs[2]);

			mesh->faces.push_back(vs[3]);
			mesh->faces.push_back(vs[2]);
			mesh->faces.push_back(vs[1]);
		}
	}

	setNode("mesh", ret, mesh);
	ret->genNormals();
	ret->genTangents();
	ret->genAABBs();

	ret->haveNormals = ret->haveTexcoords = ret->haveTangents = true;

	return ret;
}

sceneModel::ptr generate_grid(int sx, int sy, int ex, int ey, int spacing) {
	auto ecs = engine::Resolve<ecs::entityManager>();

	sceneModel::ptr ret = ecs->construct<sceneModel>();
	sceneMesh::ptr mesh = ecs->construct<sceneMesh>();

	unsigned i = 0;
	for (int y = sx; y <= ey; y += spacing) {
		for (int x = sx; x <= ex; x += spacing) {
			float y1 = 0.2*sin(y*0.3f);
			float y2 = 0.2*sin((y + spacing)*0.3f);
			float x1 = 0.2*sin((x)*0.2f);
			float x2 = 0.2*sin((x + spacing)*0.2f);

			ret->vertices.push_back((sceneModel::vertex) {
				.position = {x - spacing, x1+y2, y},
				.normal   = {0, 1, 0},
				.uv       = {0, 0},
			});

			ret->vertices.push_back((sceneModel::vertex) {
				.position = {x, x2+y2, y},
				.normal   = {0, 1, 0},
				.uv       = {1, 0},
			});

			ret->vertices.push_back((sceneModel::vertex) {
				.position = {x - spacing, x1+y1, y - spacing},
				.normal   = {0, 1, 0},
				.uv       = {0, 1},
			});

			ret->vertices.push_back((sceneModel::vertex) {
				.position = {x, x2+y1, y - spacing},
				.normal   = {0, 1, 0},
				.uv       = {1, 1},
			});

			ret->vertices.push_back((sceneModel::vertex) {
				.position = {x - spacing, x1+y1, y - spacing},
				.normal   = {0, 1, 0},
				.uv       = {0, 1},
			});

			ret->vertices.push_back((sceneModel::vertex) {
				.position = {x, x2+y2, y},
				.normal   = {0, 1, 0},
				.uv       = {1, 0},
			});

			for (unsigned k = 0; k < 6; k++) {
				mesh->faces.push_back(i++);
			}
		}
	}

	setNode("mesh", ret, mesh);

	ret->genTangents();

	ret->haveNormals = true;
	ret->haveTexcoords = true;

	return ret;
}

sceneModel::ptr generate_cuboid(float width, float height, float depth) {
	auto ecs = engine::Resolve<ecs::entityManager>();

	sceneModel::ptr ret  = ecs->construct<sceneModel>();
	sceneMesh::ptr  mesh = ecs->construct<sceneMesh>();

	float ax = width/2;
	float ay = height/2;
	float az = depth/2;

	// TODO: cube generation still borked after VBO refactor,
	//       leaving this here for reference
	/*
	float ax = width/2;
	float ay = height/2;
	float az = depth/2;

	// front
	ret->vertices.push_back(glm::vec3(-ax, -ay,  az));
	ret->vertices.push_back(glm::vec3( ax, -ay,  az));
	ret->vertices.push_back(glm::vec3( ax,  ay,  az));
	ret->vertices.push_back(glm::vec3(-ax,  ay,  az));

	// top
	ret->vertices.push_back(glm::vec3(-ax,  ay,  az));
	ret->vertices.push_back(glm::vec3( ax,  ay,  az));
	ret->vertices.push_back(glm::vec3( ax,  ay, -az));
	ret->vertices.push_back(glm::vec3(-ax,  ay, -az));

	// back
	ret->vertices.push_back(glm::vec3( ax, -ay, -az));
	ret->vertices.push_back(glm::vec3(-ax, -ay, -az));
	ret->vertices.push_back(glm::vec3(-ax,  ay, -az));
	ret->vertices.push_back(glm::vec3( ax,  ay, -az));

	// bottom
	ret->vertices.push_back(glm::vec3(-ax, -ay, -az));
	ret->vertices.push_back(glm::vec3( ax, -ay, -az));
	ret->vertices.push_back(glm::vec3( ax, -ay,  az));
	ret->vertices.push_back(glm::vec3(-ax, -ay,  az));

	// left
	ret->vertices.push_back(glm::vec3(-ax, -ay, -az));
	ret->vertices.push_back(glm::vec3(-ax, -ay,  az));
	ret->vertices.push_back(glm::vec3(-ax,  ay,  az));
	ret->vertices.push_back(glm::vec3(-ax,  ay, -az));

	// right
	ret->vertices.push_back(glm::vec3( ax, -ay,  az));
	ret->vertices.push_back(glm::vec3( ax, -ay, -az));
	ret->vertices.push_back(glm::vec3( ax,  ay, -az));
	ret->vertices.push_back(glm::vec3( ax,  ay,  az));
	*/

	/*
	unsigned generated = 0;
	for (unsigned i = 0; i < 8; i++) {
		generated |= 1 << i;

		for (unsigned k = 0; k < 3; k++) {
			unsigned n1 = i ^ (1 << k);
			unsigned n2 = i ^ (1 << ((k + 1) % 3));

			if (!(generated & n1) && !(generated & n1)) {
				glm::vec3 a = glm::vec3((i&1),  (i&2),  (i&4));
				glm::vec3 b = glm::vec3((n1&1), (n1&2), (n1&4));
				glm::vec3 c = glm::vec3((n2&1), (n2&2), (n2&4));

				for (auto& p : {a, b, c, a}) {
					ret->vertices.push_back((sceneModel::vertex) {
						.position = (p * dim) - dim/2.f,
						.uv       = {int(p.x)&1, int(p.y)&2},
					});
				}
			}
		}
	}
	*/

	/*
		ret->texcoords.push_back({0, 0});
		ret->texcoords.push_back({1, 0});
		ret->texcoords.push_back({0, 1});
		ret->texcoords.push_back({1, 0});
	*/

	glm::vec2 uvs[4] = {
		{0, 0}, {1, 0},
		{0, 1}, {1, 1},
	};

	glm::vec3 verts[24] = {
		// front
		{-ax, -ay,  az}, { ax, -ay,  az},
		{ ax,  ay,  az}, {-ax,  ay,  az},
		// top
		{-ax,  ay,  az}, { ax,  ay,  az},
		{ ax,  ay, -az}, {-ax,  ay, -az},
		// back
		{ ax, -ay, -az}, {-ax, -ay, -az},
		{-ax,  ay, -az}, { ax,  ay, -az},
		// bottom
		{-ax, -ay, -az}, { ax, -ay, -az},
		{ ax, -ay,  az}, {-ax, -ay,  az},
		// left
		{-ax, -ay, -az}, {-ax, -ay,  az},
		{-ax,  ay,  az}, {-ax,  ay, -az},
		// right
		{ ax, -ay,  az}, { ax, -ay, -az},
		{ ax,  ay, -az}, { ax,  ay,  az},
	};

	ret->vertices.resize(24);

	for (unsigned i = 0; i < 24; i += 4) {
		for (unsigned k = 0; k < 4; k++) {
			ret->vertices[i+k].position = verts[i+k];
			ret->vertices[i+k].uv       = uvs[k];
		}

		mesh->faces.push_back(i);
		mesh->faces.push_back(i+1);
		mesh->faces.push_back(i+2);

		mesh->faces.push_back(i+2);
		mesh->faces.push_back(i+3);
		mesh->faces.push_back(i);
	}

	setNode("mesh", ret, mesh);
	ret->genNormals();
	ret->genTangents();
	ret->genAABBs();

	// TODO: these should be set in gen...()
	ret->haveNormals = ret->haveTexcoords = true;

	return ret;
}

// namespace grendx
}
