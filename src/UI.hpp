#pragma once

#include "health.hpp"
#include <grend/ecs/ecs.hpp>
#include <grend/camera.hpp>
#include <grend/vecGUI.hpp>

void drawPlayerHealthbar(entityManager *manager,
                         vecGUI&vgui,
                         health *playerHealth);
void renderHealthbars(entityManager *manager,
                      vecGUI& vgui,
                      camera::ptr cam);
void renderControls(gameMain *game, vecGUI& vgui);
