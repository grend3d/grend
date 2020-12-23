#include "boxSpawner.hpp"
#include <grend/geometryGeneration.hpp>

boxBullet::boxBullet(entityManager *manager, gameMain *game, glm::vec3 position)
	: projectile(manager, game, position)
{
	static gameObject::ptr model = nullptr;

	manager->registerComponent(this, "boxBullet", this);

	if (!model) {
#if LOCAL_BUILD
		model = loadScene("test-assets/obj/smoothcube.glb");
#else
		gameModel::ptr mod = generate_cuboid(0.3, 0.3, 0.3);
		model = std::make_shared<gameObject>();
		compileModel("boxProjectile", mod);
		setNode("model", model, mod);
#endif
		bindCookedMeshes();
	}

	setNode("model", node, model);
}

