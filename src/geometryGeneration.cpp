#include <grend/geometryGeneration.hpp>
#include <grend/gameModel.hpp>

namespace grendx {

gameModel::ptr generateHeightmap(float width, float height, float unitsPerVert,
                                 float x, float y, heightFunction func)
{
	gameModel::ptr ret = gameModel::ptr(new gameModel());
	gameMesh::ptr mesh = gameMesh::ptr(new gameMesh());

	unsigned xverts = width / unitsPerVert + 1;
	unsigned yverts = height / unitsPerVert + 1;
	float xinc = unitsPerVert / width;
	float yinc = unitsPerVert / height;

	for (unsigned k = 0; k < xverts; k++) {
		for (unsigned j = 0; j < yverts; j++) {
			float vx = k * unitsPerVert;
			float vz = j * unitsPerVert;
			float height = func(x + vx, y + vz);

			ret->vertices.push_back(glm::vec3(vx, height, vz));
			ret->texcoords.push_back({vx / 12.0, vz / 12.0});
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

gameModel::ptr generate_grid(int sx, int sy, int ex, int ey, int spacing) {
	//model ret;
	gameModel::ptr ret = gameModel::ptr(new gameModel());
	gameMesh::ptr mesh = gameMesh::ptr(new gameMesh());

	unsigned i = 0;
	for (int y = sx; y <= ey; y += spacing) {
		for (int x = sx; x <= ex; x += spacing) {
			float y1 = 0.2*sin(y*0.3f);
			float y2 = 0.2*sin((y + spacing)*0.3f);
			float x1 = 0.2*sin((x)*0.2f);
			float x2 = 0.2*sin((x + spacing)*0.2f);
			//float foo = (x ^ y)*(1./ex);
			//float foo = 0;

			ret->vertices.push_back(glm::vec3(x - spacing, x1+y2, y));
			ret->vertices.push_back(glm::vec3(x,           x2+y2, y));
			ret->vertices.push_back(glm::vec3(x - spacing, x1+y1, y - spacing));

			ret->vertices.push_back(glm::vec3(x,           x2+y1, y - spacing));
			ret->vertices.push_back(glm::vec3(x - spacing, x1+y1, y - spacing));
			ret->vertices.push_back(glm::vec3(x,           x2+y2, y));

			ret->texcoords.push_back({0, 0});
			ret->texcoords.push_back({1, 0});
			ret->texcoords.push_back({0, 1});

			ret->texcoords.push_back({1, 1});
			ret->texcoords.push_back({0, 1});
			ret->texcoords.push_back({1, 0});

			for (unsigned k = 0; k < 6; k++) {
				mesh->faces.push_back(i++);
				ret->normals.push_back(glm::vec3(0, 1, 0));
			}
		}
	}

	setNode("mesh", ret, mesh);

	//ret->gen_normals();
	ret->genTangents();

	ret->haveNormals = true;
	ret->haveTexcoords = true;

	return ret;
}

gameModel::ptr generate_cuboid(float width, float height, float depth) {
	gameModel::ptr ret = gameModel::ptr(new gameModel());
	gameMesh::ptr mesh = gameMesh::ptr(new gameMesh());

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

	for (unsigned i = 0; i < 24; i += 4) {
		mesh->faces.push_back(i);
		mesh->faces.push_back(i+1);
		mesh->faces.push_back(i+2);

		mesh->faces.push_back(i+2);
		mesh->faces.push_back(i+3);
		mesh->faces.push_back(i);

		ret->texcoords.push_back({0, 0});
		ret->texcoords.push_back({1, 0});
		ret->texcoords.push_back({0, 1});
		ret->texcoords.push_back({1, 0});
	}

	setNode("mesh", ret, mesh);
	ret->genNormals();
	ret->genTangents();

	// TODO: these should be set in gen...()
	ret->haveNormals = ret->haveTexcoords = true;

	return ret;
}

// namespace grendx
}
