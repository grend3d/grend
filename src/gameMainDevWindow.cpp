#include <grend/gameMainDevWindow.hpp>

using namespace grendx;

void gameMainDevWindow::handleInput(void) {
	SDL_Event ev;

	while (SDL_PollEvent(&ev)) {
		if (ev.type == SDL_QUIT) {
			running = false;
		}

#if 0
		else if (ev.type == SDL_WINDOWEVENT) {
			switch (ev.window.event) {
				case SDL_WINDOWEVENT_RESIZED:
				case SDL_WINDOWEVENT_SIZE_CHANGED:
					{
						SDL_GetWindowSize(ctx.window, &screen_x, &screen_y);
						// TODO: maybe reallocate framebuffers
						//int win_x, win_y;
						//glViewport(0, 0, win_x, win_y);
						/*
						projection = glm::perspective(glm::radians(60.f),
						                              (1.f*win_x)/win_y, 0.1f, 100.f);
													  */
					}
					break;
			}
		}

		else if (ev.type == SDL_MOUSEBUTTONDOWN) {
			if (ev.button.button == SDL_BUTTON_LEFT) {
				GLbyte color[4];
				GLfloat depth;
				GLuint index;

				// TODO: move this into renderer, since this all depends
				//       on render state
				/*
				int x, y;
				int win_x, win_y;
				Uint32 buttons = SDL_GetMouseState(&x, &y);
				buttons = buttons; // XXX: make the compiler shut up about the unused variable
				SDL_GetWindowSize(ctx.window, &win_x, &win_y);

				// adjust coordinates for window/resolution scaling
				x = rend_x * ((1.*x)/(1.*win_x)) * dsr_scale_x;
				y = (rend_y - (rend_y * ((1.*y)/(1.*win_y))) - 1) * dsr_scale_y;

				rend_fb->bind();
				glReadPixels(x, y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, color);
				glReadPixels(x, y, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &depth);
				glReadPixels(x, y, 1, 1, GL_STENCIL_INDEX, GL_UNSIGNED_INT, &index);

				struct draw_class obj = rend.index(index);
				fprintf(stderr, "clicked %d, %d, class: %u, id: %u, color: #%02x%02x%02x:%u, depth: %f, index: %u\n",
						x, y,
						obj.class_id, obj.obj_id,
						color[0]&0xff, color[1]&0xff, color[2]&0xff, color[3]&0xff,
						depth, index);

				// XXX: TODO: not this
				switch (obj.class_id) {
					case DRAWATTR_CLASS_MAP:
						editor.selected_object = obj.obj_id;
						editor.selected_light = -1;
						editor.selected_refprobe = -1;
						break;

					case DRAWATTR_CLASS_PHYSICS:
						editor.selected_object = -(int)obj.obj_id - 0x8000;
						editor.selected_light = -1;
						editor.selected_refprobe = -1;
						break;

					case DRAWATTR_CLASS_UI_LIGHT:
						editor.selected_light = obj.obj_id;
						editor.selected_refprobe = -1;
						editor.selected_object = -1;
						break;

					case DRAWATTR_CLASS_UI_REFPROBE:
						editor.selected_refprobe = obj.obj_id;
						editor.selected_object = -1;
						editor.selected_light = -1;
						break;

					case DRAWATTR_CLASS_UI:
						{
							static const char *axis[] = {"X", "Y", "Z"};
							static const char *indicator[] = {
								"pointer", 
								"rotation spinner"
							};

							fprintf(stderr, "clicked %s axis %s\n",
								axis[obj.obj_id % 3], indicator[obj.obj_id / 3]);
						}
						break;

					default:
						editor.selected_object = -1;
						editor.selected_light = -1;
						editor.selected_refprobe = -1;
						break;
				}
				*/
			}
		}

		// TODO: these should be placed in their respective view classes
		/*
		if (editor.mode != game_editor::mode::Inactive) {
			editor.handle_editor_input(&rend, ctx, ev);
		} else {
			handle_player_input(ev);
		}
		*/
		handle_player_input(ev);
#endif
		view->handleInput(this, ev);
	}
}
