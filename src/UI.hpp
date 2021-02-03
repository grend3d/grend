#pragma once

#include "health.hpp"
#include "levelController.hpp"

#include <grend/ecs/ecs.hpp>
#include <grend/camera.hpp>
#include <grend/vecGUI.hpp>

using namespace grendx;

void drawPlayerHealthbar(entityManager *manager,
                         vecGUI&vgui,
                         health *playerHealth);
void renderHealthbars(entityManager *manager,
                      vecGUI& vgui,
                      camera::ptr cam);
void renderObjectives(entityManager *manager,
                      levelController *level,
                      vecGUI& vgui);
void renderControls(gameMain *game, vecGUI& vgui);
