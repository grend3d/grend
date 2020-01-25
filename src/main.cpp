#include <grend/sdl-context.hpp>
#include <grend/model.hpp>
#include <grend/glm-includes.hpp>
#include <grend/geometry-gen.hpp>
#include <grend/grend.hpp>
#include <grend/utility.hpp>

#include <stdio.h>
#include <stdlib.h>

#include <tuple>
#include <vector>
#include <fstream>
#include <iostream>
#include <sstream>
#include <iterator>
#include <string>
#include <exception>
#include <memory>
#include <utility>
#include <map>

using namespace grendx;

std::map<std::string, material> default_materials = {
	{"(null)", {{0.75, 0.75, 0.75, 1}, {1, 1, 1, 1}, {0.5, 0.5, 0.5, 1}, 15}},
	{"Blk",    {{1, 0.8, 0.2, 1}, {1, 0.8, 0.2, 1}, {1, 1, 1, 1}, 50}},
	{"Grey",   {{0.1, 0.1, 0.1, 0.5}, {0.1, 0.1, 0.1, 0.2}, {0.0, 0.0, 0.0, 0.05}, 15}},
	{"Yellow", {{0.01, 0.01, 0.01, 1}, {0, 0, 0, 1}, {0.2, 0.2, 0.2, 0.2}, 20}},
	{"Steel",  {{1, 1, 1, 1}, {1, 1, 1, 1}, {1, 1, 1, 1}, 20}},
};

class testscene : public grend {
	public:
		testscene();
		virtual ~testscene();
		virtual void render(context& ctx);
		virtual void logic(context& ctx);
		virtual void physics(context& ctx);
		virtual void input(context& ctx);

		void save_map(std::string name="save.map");
		void load_map(std::string name="save.map");

		void draw_mesh(compiled_mesh& foo, glm::mat4 transform, material& mat);
		void draw_mesh_lines(compiled_mesh& foo, glm::mat4 transform);
		void draw_mesh_lines(compiled_mesh& foo, glm::mat4 transform, material& mat);
		void draw_model(compiled_model& obj, glm::mat4 transform);
		void draw_model_lines(compiled_model& obj, glm::mat4 transform);

		glm::mat4 projection, view;

		glm::vec3 view_position = glm::vec3(0, 6, 5);
		glm::vec3 view_velocity;

		glm::vec3 view_direction = glm::vec3(0, 0, -1);
		glm::vec3 view_up = glm::vec3(0, 1, 0);
		glm::vec3 view_right = glm::vec3(1, 0, 0);

		struct editor_entry {
			std::string name;
			glm::vec3   position;
			// TODO: full rotation
			glm::mat4   transform;
			bool        inverted;

			// TODO: might be a good idea to keep track of whether face culling is enabled for
			//       this model, although that might be better off in the model class itself...
		};

		// Map editing things
		bool in_select_mode = false;
		glm::vec3 select_position = glm::vec3(0, 0, 0);
		glm::mat4 select_transform = glm::mat4(1);
		float     select_distance = 5;
		// this keeps track of the current face order after flipping along an axis
		bool      select_inverted = false;

		std::vector<editor_entry> dynamic_models;
		model_map::iterator select_model;

		Uint32 last_frame;

		// models
		model_map models = {
			/*
			   {"ground",    model("assets/obj/Modular Terrain Hilly/Grass_Flat.obj")},
			   {"path",      model("assets/obj/Modular Terrain Hilly/Path_Center.obj")},
			   {"grass",     model("assets/obj/Modular Terrain Hilly/Prop_Grass_Clump_1.obj")},
			   {"stump",     model("assets/obj/Modular Terrain Hilly/Prop_Stump.obj")},
			   {"tree",      model("assets/obj/Modular Terrain Hilly/Prop_Tree_Oak_1.obj")},
			   {"person",    model("assets/obj/low-poly-character-rpg/boy.obj")},
			   */
			{"person",    model("assets/obj/low-poly-character-rpg/boy.obj")},

			// TODO: load whole directories (model libraries)
			{"Grass_Flat.obj", model("assets/obj/Modular Terrain Hilly/Grass_Flat.obj")},
			{"Hill_Corner_Inner_2x2.obj", model("assets/obj/Modular Terrain Hilly/Hill_Corner_Inner_2x2.obj")},
			{"Hill_Corner_Outer_2x2.obj", model("assets/obj/Modular Terrain Hilly/Hill_Corner_Outer_2x2.obj")},
			{"Hill_Side.obj", model("assets/obj/Modular Terrain Hilly/Hill_Side.obj")},
			{"Hill_Side_On_Side.obj", model("assets/obj/Modular Terrain Hilly/Hill_Side_On_Side.obj")},
			{"Hill_Side_Transition_From_Gentle.obj", model("assets/obj/Modular Terrain Hilly/Hill_Side_Transition_From_Gentle.obj")},
			{"Hill_Side_Transition_To_Gentle.obj", model("assets/obj/Modular Terrain Hilly/Hill_Side_Transition_To_Gentle.obj")},
			{"Path_Center.obj", model("assets/obj/Modular Terrain Hilly/Path_Center.obj")},
			{"Path_Corner_Inner_1x1.obj", model("assets/obj/Modular Terrain Hilly/Path_Corner_Inner_1x1.obj")},
			{"Path_Corner_Inner_2x2.obj", model("assets/obj/Modular Terrain Hilly/Path_Corner_Inner_2x2.obj")},
			{"Path_Corner_Outer_1x1.obj", model("assets/obj/Modular Terrain Hilly/Path_Corner_Outer_1x1.obj")},
			{"Path_Corner_Outer_2x2.obj", model("assets/obj/Modular Terrain Hilly/Path_Corner_Outer_2x2.obj")},
			{"Path_Corner_Outer_3x3.obj", model("assets/obj/Modular Terrain Hilly/Path_Corner_Outer_3x3.obj")},
			{"Path_Corner_Y_2x2.obj", model("assets/obj/Modular Terrain Hilly/Path_Corner_Y_2x2.obj")},
			{"Path_Corner_Y_3x3.obj", model("assets/obj/Modular Terrain Hilly/Path_Corner_Y_3x3.obj")},
			{"Path_Hill_Gentle_Center.obj", model("assets/obj/Modular Terrain Hilly/Path_Hill_Gentle_Center.obj")},
			{"Path_Hill_Gentle_Side.obj", model("assets/obj/Modular Terrain Hilly/Path_Hill_Gentle_Side.obj")},
			{"Path_Hill_Sharp_Center.obj", model("assets/obj/Modular Terrain Hilly/Path_Hill_Sharp_Center.obj")},
			{"Path_Hill_Sharp_Side.obj", model("assets/obj/Modular Terrain Hilly/Path_Hill_Sharp_Side.obj")},
			{"Path_Side.obj", model("assets/obj/Modular Terrain Hilly/Path_Side.obj")},
			{"Prop_Branch_1.obj", model("assets/obj/Modular Terrain Hilly/Prop_Branch_1.obj")},
			{"Prop_Branch_2.obj", model("assets/obj/Modular Terrain Hilly/Prop_Branch_2.obj")},
			{"Prop_Branch_3.obj", model("assets/obj/Modular Terrain Hilly/Prop_Branch_3.obj")},
			{"Prop_Bridge_Log_End_Edge.obj", model("assets/obj/Modular Terrain Hilly/Prop_Bridge_Log_End_Edge.obj")},
			{"Prop_Bridge_Log_End.obj", model("assets/obj/Modular Terrain Hilly/Prop_Bridge_Log_End.obj")},
			{"Prop_Bridge_Log_Middle_Edge.obj", model("assets/obj/Modular Terrain Hilly/Prop_Bridge_Log_Middle_Edge.obj")},
			{"Prop_Bridge_Log_Middle.obj", model("assets/obj/Modular Terrain Hilly/Prop_Bridge_Log_Middle.obj")},
			{"Prop_Bridge_Log_Post_Support.obj", model("assets/obj/Modular Terrain Hilly/Prop_Bridge_Log_Post_Support.obj")},
			{"Prop_Bridge_Log_Post_Top.obj", model("assets/obj/Modular Terrain Hilly/Prop_Bridge_Log_Post_Top.obj")},
			{"Prop_Bush_1.obj", model("assets/obj/Modular Terrain Hilly/Prop_Bush_1.obj")},
			{"Prop_Bush_2.obj", model("assets/obj/Modular Terrain Hilly/Prop_Bush_2.obj")},
			{"Prop_Bush_3.obj", model("assets/obj/Modular Terrain Hilly/Prop_Bush_3.obj")},
			{"Prop_Cattail_1.obj", model("assets/obj/Modular Terrain Hilly/Prop_Cattail_1.obj")},
			{"Prop_Cattail_2.obj", model("assets/obj/Modular Terrain Hilly/Prop_Cattail_2.obj")},
			{"Prop_Fence_Boards_1.obj", model("assets/obj/Modular Terrain Hilly/Prop_Fence_Boards_1.obj")},
			{"Prop_Fence_Boards_2.obj", model("assets/obj/Modular Terrain Hilly/Prop_Fence_Boards_2.obj")},
			{"Prop_Fence_Boards_3.obj", model("assets/obj/Modular Terrain Hilly/Prop_Fence_Boards_3.obj")},
			{"Prop_Fence_Boards_4.obj", model("assets/obj/Modular Terrain Hilly/Prop_Fence_Boards_4.obj")},
			{"Prop_Fence_Curve_1x1.obj", model("assets/obj/Modular Terrain Hilly/Prop_Fence_Curve_1x1.obj")},
			{"Prop_Fence_Curve_2x2.obj", model("assets/obj/Modular Terrain Hilly/Prop_Fence_Curve_2x2.obj")},
			{"Prop_Fence_Curve_3x3.obj", model("assets/obj/Modular Terrain Hilly/Prop_Fence_Curve_3x3.obj")},
			{"Prop_Fence_Gate_1.obj", model("assets/obj/Modular Terrain Hilly/Prop_Fence_Gate_1.obj")},
			{"Prop_Fence_Gate_2.obj", model("assets/obj/Modular Terrain Hilly/Prop_Fence_Gate_2.obj")},
			{"Prop_Fence_Hill_Gentle.obj", model("assets/obj/Modular Terrain Hilly/Prop_Fence_Hill_Gentle.obj")},
			{"Prop_Fence_Hill_Sharp.obj", model("assets/obj/Modular Terrain Hilly/Prop_Fence_Hill_Sharp.obj")},
			{"Prop_Fence_Post_1.obj", model("assets/obj/Modular Terrain Hilly/Prop_Fence_Post_1.obj")},
			{"Prop_Fence_Post_2.obj", model("assets/obj/Modular Terrain Hilly/Prop_Fence_Post_2.obj")},
			{"Prop_Fence_Post_3.obj", model("assets/obj/Modular Terrain Hilly/Prop_Fence_Post_3.obj")},
			{"Prop_Fence_Post_4.obj", model("assets/obj/Modular Terrain Hilly/Prop_Fence_Post_4.obj")},
			{"Prop_Flower_Daisy.obj", model("assets/obj/Modular Terrain Hilly/Prop_Flower_Daisy.obj")},
			{"Prop_Flower_Lily_Blue.obj", model("assets/obj/Modular Terrain Hilly/Prop_Flower_Lily_Blue.obj")},
			{"Prop_Flower_Lily_Pink.obj", model("assets/obj/Modular Terrain Hilly/Prop_Flower_Lily_Pink.obj")},
			{"Prop_Flower_Rose.obj", model("assets/obj/Modular Terrain Hilly/Prop_Flower_Rose.obj")},
			{"Prop_Flower_Sunflower.obj", model("assets/obj/Modular Terrain Hilly/Prop_Flower_Sunflower.obj")},
			{"Prop_Flower_Tulip.obj", model("assets/obj/Modular Terrain Hilly/Prop_Flower_Tulip.obj")},
			{"Prop_Grass_Clump_1.obj", model("assets/obj/Modular Terrain Hilly/Prop_Grass_Clump_1.obj")},
			{"Prop_Grass_Clump_2.obj", model("assets/obj/Modular Terrain Hilly/Prop_Grass_Clump_2.obj")},
			{"Prop_Grass_Clump_3.obj", model("assets/obj/Modular Terrain Hilly/Prop_Grass_Clump_3.obj")},
			{"Prop_Grass_Clump_4.obj", model("assets/obj/Modular Terrain Hilly/Prop_Grass_Clump_4.obj")},
			{"Prop_Hollow_Trunk.obj", model("assets/obj/Modular Terrain Hilly/Prop_Hollow_Trunk.obj")},
			{"Prop_Mushroom_1.obj", model("assets/obj/Modular Terrain Hilly/Prop_Mushroom_1.obj")},
			{"Prop_Mushroom_2.obj", model("assets/obj/Modular Terrain Hilly/Prop_Mushroom_2.obj")},
			{"Prop_Rock_1.obj", model("assets/obj/Modular Terrain Hilly/Prop_Rock_1.obj")},
			{"Prop_Rock_2.obj", model("assets/obj/Modular Terrain Hilly/Prop_Rock_2.obj")},
			{"Prop_Rock_3.obj", model("assets/obj/Modular Terrain Hilly/Prop_Rock_3.obj")},
			{"Prop_Rock_4.obj", model("assets/obj/Modular Terrain Hilly/Prop_Rock_4.obj")},
			{"Prop_Stump.obj", model("assets/obj/Modular Terrain Hilly/Prop_Stump.obj")},
			{"Prop_Tree_Cedar_1.obj", model("assets/obj/Modular Terrain Hilly/Prop_Tree_Cedar_1.obj")},
			{"Prop_Tree_Cedar_2.obj", model("assets/obj/Modular Terrain Hilly/Prop_Tree_Cedar_2.obj")},
			{"Prop_Tree_Oak_1.obj", model("assets/obj/Modular Terrain Hilly/Prop_Tree_Oak_1.obj")},
			{"Prop_Tree_Oak_2.obj", model("assets/obj/Modular Terrain Hilly/Prop_Tree_Oak_2.obj")},
			{"Prop_Tree_Oak_3.obj", model("assets/obj/Modular Terrain Hilly/Prop_Tree_Oak_3.obj")},
			{"Prop_Tree_Pine_1.obj", model("assets/obj/Modular Terrain Hilly/Prop_Tree_Pine_1.obj")},
			{"Prop_Tree_Pine_2.obj", model("assets/obj/Modular Terrain Hilly/Prop_Tree_Pine_2.obj")},
			{"Prop_Tree_Pine_3.obj", model("assets/obj/Modular Terrain Hilly/Prop_Tree_Pine_3.obj")},
			{"Water_Flat.obj", model("assets/obj/Modular Terrain Hilly/Water_Flat.obj")},
			{"Water_Slope.obj", model("assets/obj/Modular Terrain Hilly/Water_Slope.obj")},

			{"grid",      generate_grid(-32, -32, 32, 32, 4)},
			{"unit_cube", generate_cuboid(1, 1, 1)},
		};

		rhandle main_vao;
		rhandle vertex_shader, fragment_shader;
		rhandle textures[8] = {
			load_texture("assets/tex/white.png"),
		};

		rhandle shader_program;
		GLint uniform_mytexture;

		// cube info
		rhandle vertices, color, elements, texcoords, normals;

		// monkey info
		rhandle mk_vertices, mk_faces, mk_normals;

		// weapon info
		rhandle wp_vertices, wp_faces, wp_normals;
};

testscene::testscene() : grend() {
	projection = glm::perspective(glm::radians(60.f),
	                             (1.f*SCREEN_SIZE_X)/SCREEN_SIZE_Y, 0.1f, 100.f);
	view = glm::lookAt(glm::vec3(0.0, 1.0, 5.0),
	                   glm::vec3(0.0, 0.0, 0.0),
	                   glm::vec3(0.0, 1.0, 0.0));
	select_model = models.begin();

	// buffers
	main_vao = bind_vao(gen_vao());
	//compile_meshes(meshes);
	compile_models(models);
	bind_cooked_meshes();

	vertex_shader = load_shader("assets/shaders/vertex-shading.vert", GL_VERTEX_SHADER);
	fragment_shader = load_shader("assets/shaders/vertex-shading.frag", GL_FRAGMENT_SHADER);
	shader_program = gen_program();

	glAttachShader(shader_program.first, vertex_shader.first);
	glAttachShader(shader_program.first, fragment_shader.first);

	// monkey business
	fprintf(stderr, " # have %lu vertices\n", cooked_vertices.size());
	glBindAttribLocation(shader_program.first, cooked_vert_vbo.first, "in_Position");
	glBindAttribLocation(shader_program.first, cooked_texcoord_vbo.first, "texcoord");
	glBindAttribLocation(shader_program.first, cooked_normal_vbo.first, "v_normal");
	glLinkProgram(shader_program.first);

	int linked;

	glGetProgramiv(shader_program.first, GL_LINK_STATUS, &linked);
	if (!linked) {
		SDL_Die("couldn't link monkey shaders");
	}

	glUseProgram(shader_program.first);
	if ((uniform_mytexture = glGetUniformLocation(shader_program.first, "mytexture")) == -1) {
		SDL_Die("Couldn't bind mytexture");
	}
}

testscene::~testscene() {
	puts("got here");
}

static glm::mat4 model_to_world(glm::mat4 model) {
	glm::quat rotation = glm::quat_cast(model);
	//rotation = -glm::conjugate(rotation);

	return glm::mat4_cast(rotation);
}

void testscene::draw_mesh(compiled_mesh& foo, glm::mat4 transform, material& mat) {
	glm::mat3 m_3x3_inv_transp = glm::transpose(glm::inverse(model_to_world(transform)));

	glUniformMatrix4fv(glGetUniformLocation(shader_program.first, "m"),
			1, GL_FALSE, glm::value_ptr(transform));
	glUniformMatrix3fv(glGetUniformLocation(shader_program.first, "m_3x3_inv_transp"),
			1, GL_FALSE, glm::value_ptr(m_3x3_inv_transp));

	// TODO: bind light maps here

	glUniform4f(glGetUniformLocation(shader_program.first, "anmaterial.diffuse"),
			mat.diffuse.x, mat.diffuse.y, mat.diffuse.z, mat.diffuse.w);
	glUniform4f(glGetUniformLocation(shader_program.first, "anmaterial.ambient"),
			mat.ambient.x, mat.ambient.y, mat.ambient.z, mat.ambient.w);
	glUniform4f(glGetUniformLocation(shader_program.first, "anmaterial.specular"),
			mat.specular.x, mat.specular.y, mat.specular.z, mat.specular.w);
	glUniform1f(glGetUniformLocation(shader_program.first, "anmaterial.shininess"),
			mat.shininess);

	bind_vao(foo.vao);
	glDrawElements(GL_TRIANGLES, foo.elements_size, GL_UNSIGNED_SHORT, foo.elements_offset);
}

// TODO: overload of this that takes a material
void testscene::draw_mesh_lines(compiled_mesh& foo, glm::mat4 transform) {
	glm::mat3 m_3x3_inv_transp = glm::transpose(glm::inverse(model_to_world(transform)));

	glUniformMatrix4fv(glGetUniformLocation(shader_program.first, "m"),
			1, GL_FALSE, glm::value_ptr(transform));
	glUniformMatrix3fv(glGetUniformLocation(shader_program.first, "m_3x3_inv_transp"),
			1, GL_FALSE, glm::value_ptr(m_3x3_inv_transp));

	// TODO: bind light maps here

	bind_vao(foo.vao);
	//glDrawElements(GL_LINE_LOOP, foo.elements_size, GL_UNSIGNED_SHORT, foo.elements_offset);
	glDrawElements(GL_LINES, foo.elements_size, GL_UNSIGNED_SHORT, foo.elements_offset);
}

void testscene::draw_model(compiled_model& obj, glm::mat4 transform) {
	for (std::string& name : obj.meshes) {
		std::string mat_name = cooked_meshes[name].material;

		if (obj.materials.find(mat_name) == obj.materials.end()) {
			material& mat = default_materials["(null)"];

			glBindTexture(GL_TEXTURE_2D, textures[0].first);
			glUniform1i(uniform_mytexture, 0);

			draw_mesh(cooked_meshes[name], transform, mat);
		}

		else {
			material& mat = obj.materials[mat_name];

			if (!mat.diffuse_map.empty()) {
				glBindTexture(GL_TEXTURE_2D, obj.mat_textures[mat_name].first);
				glUniform1i(uniform_mytexture, 0);

			} else {
				glBindTexture(GL_TEXTURE_2D, textures[0].first);
				glUniform1i(uniform_mytexture, 0);
			}

			draw_mesh(cooked_meshes[name], transform, mat);
		}
	}
}

void testscene::draw_model_lines(compiled_model& obj, glm::mat4 transform) {
	// TODO: need a set_material() function to handle stuff
	//       and we need to explicitly set a material
	glBindTexture(GL_TEXTURE_2D, textures[0].first);
	glUniform1i(uniform_mytexture, 0);

	for (std::string& name : obj.meshes) {
		draw_mesh_lines(cooked_meshes[name], transform);
	}
}

void testscene::render(context& ctx) {
	glClearColor(0.7, 0.9, 1, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, textures[0].first);
	glUniform1i(uniform_mytexture, 0);

#ifdef ENABLE_FACE_CULLING
	// TODO: toggle per-model
	glEnable(GL_CULL_FACE);
#endif

	glm::mat4 v_inv = glm::inverse(view);

	glUniformMatrix4fv(glGetUniformLocation(shader_program.first, "v"),
			1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(glGetUniformLocation(shader_program.first, "p"),
			1, GL_FALSE, glm::value_ptr(projection));

	glUniformMatrix4fv(glGetUniformLocation(shader_program.first, "v_inv"),
			1, GL_FALSE, glm::value_ptr(v_inv));

	srand(0x1337babe);
	const int dimension = 16;

	// TODO: this is really inefficient, especially combined with how inefficient
	//       draw_model is right now, need to join these meshes together to make a few
	//       bigger "world" meshes
	//       something like 16-unit cubes would be good probably
	//draw_model_lines(cooked_models["grid"], glm::mat4(1));
	for (int x = -dimension; x < dimension; x++) {
		for (int y = -dimension; y < dimension; y++) {
			glm::mat4 trans = glm::translate(glm::vec3(x, 0, y));
			if (abs(x) < 4 || abs(y) < 4) {
				draw_model(cooked_models["Path_Center.obj"], trans);

			} else {
				draw_model(cooked_models["Grass_Flat.obj"], trans);
				trans = glm::translate(glm::vec3(x, 0.2, y));

				if (abs(x) == 12 && abs(y) == 12) {
					draw_model(cooked_models["Prop_Tree_Oak_2.obj"], trans);

				} else if (rand() % 3 == 0) {
					draw_model(cooked_models["Prop_Grass_Clump_4.obj"], trans);

				} else if (rand() % 17 == 0) {
					draw_model(cooked_models["Prop_Stump.obj"], trans);
				}
			}
		}
	}

	glm::vec3 hpos = glm::vec3(view_direction.x*10 + view_position.x, 0, view_direction.z*10 + view_position.z);
	glm::mat4 bizz = glm::translate(glm::mat4(1), hpos);
	draw_model(cooked_models["person"], bizz);

	if (in_select_mode) {
		glm::mat4 trans = glm::translate(select_position) * select_transform;

		glFrontFace(select_inverted? GL_CW : GL_CCW);
		draw_model_lines(cooked_models[select_model->first], trans);
		draw_model(cooked_models[select_model->first], trans);
	}

	for (auto& v : dynamic_models) {
		glm::mat4 trans = glm::translate(v.position) * v.transform;

		glFrontFace(v.inverted? GL_CW : GL_CCW);
		draw_model(cooked_models[v.name], trans);
	}

	glFrontFace(GL_CCW);
	for (unsigned i = 0; i < 16; i++) {
		float time_component = i + M_PI*SDL_GetTicks()/1000.f;
		const float fpi = 3.1415926f;

		glm::mat4 model = glm::translate(glm::mat4(1),
			glm::vec3(-32.f + 4*i, 4 + sin(time_component), 0))
			* glm::rotate(fpi*cosf(0.025 * fpi * time_component), glm::vec3(0, 1, 0));

		compiled_model& foo =
		     (i%4 == 0)? cooked_models["Grass_Flat.obj"]
		   : (i%4 == 1)? cooked_models["Prop_Stump.obj"]
		   : (i%4 == 2)? cooked_models["Prop_Tree_Oak_2.obj"]
		   :             cooked_models["person"];

		draw_model(foo, model);
	}

	SDL_GL_SwapWindow(ctx.window);
}

void testscene::input(context& ctx) {
	Uint32 cur_ticks = SDL_GetTicks();
	Uint32 ticks_delta = cur_ticks - last_frame;

	float fticks = ticks_delta / 1000.0f;

	const float movement_speed = 10 /* units/s */;
	const float rotation_speed = 1;

	SDL_Event ev;

	while (SDL_PollEvent(&ev)) {
		if (ev.type == SDL_QUIT) {
			running = false;
		}

		else if (ev.type == SDL_KEYDOWN) {
			switch (ev.key.keysym.sym) {
				case SDLK_w: view_velocity.z =  movement_speed; break;
				case SDLK_s: view_velocity.z = -movement_speed; break;
				case SDLK_a: view_velocity.x = -movement_speed; break;
				case SDLK_d: view_velocity.x =  movement_speed; break;
				case SDLK_q: view_velocity.y =  movement_speed; break;
				case SDLK_e: view_velocity.y = -movement_speed; break;
				case SDLK_m: in_select_mode = !in_select_mode; break;

				case SDLK_ESCAPE: running = false; break;
			}
		}

		else if (ev.type == SDL_KEYUP) {
			switch (ev.key.keysym.sym) {
				case SDLK_w:
				case SDLK_s:
					view_velocity.z = 0;
					break;

				case SDLK_a:
				case SDLK_d:
					view_velocity.x = 0;
					break;

				case SDLK_q:
				case SDLK_e:
					view_velocity.y = 0;
					break;
			}
		}

		else if (ev.type == SDL_WINDOWEVENT) {
			switch (ev.window.event) {
				case SDL_WINDOWEVENT_RESIZED:
				case SDL_WINDOWEVENT_SIZE_CHANGED:
					{
						int win_x, win_y;
						SDL_GetWindowSize(ctx.window, &win_x, &win_y);
						std::cerr << " * resized window: " << win_x << ", " << win_y << std::endl;
						glViewport(0, 0, win_x, win_y);
						projection = glm::perspective(glm::radians(60.f),
						                              (1.f*win_x)/win_y, 0.1f, 100.f);

					}
					break;
			}
		}

		// keeping editing commands seperate from the main input handling
		if (in_select_mode) {
			if (ev.type == SDL_KEYDOWN) {
				switch (ev.key.keysym.sym) {
					case SDLK_i: load_map(); break;
					case SDLK_o: save_map(); break;

					case SDLK_z:
						//select_rotation -= M_PI/4;
						select_transform *= glm::rotate((float)-M_PI/4.f, glm::vec3(0, 1, 0));
						break;

					case SDLK_c:
						//select_rotation += M_PI/4;
						select_transform *= glm::rotate((float)M_PI/4.f, glm::vec3(0, 1, 0));
						break;

					case SDLK_x:
						// flip horizontally (along the X axis)
						select_transform *= glm::scale(glm::vec3(-1, 1, 1));
						select_inverted = !select_inverted;
						break;

					case SDLK_b:
						// flip horizontally (along the Z axis)
						select_transform *= glm::scale(glm::vec3( 1, 1, -1));
						select_inverted = !select_inverted;
						break;

					case SDLK_v:
						// flip vertically
						select_transform *= glm::scale(glm::vec3( 1, -1, 1));
						select_inverted = !select_inverted;
						break;

					case SDLK_r:
						if (select_model == models.begin()) {
							select_model = models.end();
						}
						select_model--;
						break;

					case SDLK_f:
						select_model++;
						if (select_model == models.end()) {
							select_model = models.begin();
						}
						break;

					case SDLK_DELETE:
						// undo, basically
						if (!dynamic_models.empty()) {
							dynamic_models.pop_back();
						}
						break;
				}
			}

			else if (ev.type == SDL_MOUSEWHEEL) {
				select_distance -= ev.wheel.y/10.f /* fidelity */;
			}

			else if (ev.type == SDL_MOUSEBUTTONDOWN) {
				if (ev.button.button == SDL_BUTTON_LEFT) {
					dynamic_models.push_back({
						select_model->first,
						select_position,
						select_transform,
						select_inverted,
					});
				}
			}
		}
	}

	int x, y;
	int win_x, win_y;
	Uint32 buttons = SDL_GetMouseState(&x, &y);
	SDL_GetWindowSize(ctx.window, &win_x, &win_y);

	x = (x > 0)? x : win_x/2;
	y = (x > 0)? y : win_y/2;

	// TODO: get window resolution
	float center_x = (float)win_x / 2;
	float center_y = (float)win_y / 2;

	float rel_x = ((float)x - center_x) / center_x;
	float rel_y = ((float)y - center_y) / center_y;

	// adjust view
	view_direction = glm::normalize(glm::vec3(
		rotation_speed * rel_x,
		rotation_speed *-rel_y,
		rotation_speed * -cos(rel_x*M_PI)
	));

	view_right = glm::normalize(glm::cross(glm::vec3(0, 1, 0), view_direction));
	view_up    = glm::normalize(glm::cross(view_direction, view_right));

	view_position += view_velocity.z * view_direction * fticks;
	view_position += view_velocity.y * view_up * fticks;
	view_position += view_velocity.x * glm::normalize(glm::cross(view_direction, view_up)) * fticks;

	// TODO: "fidelity" parameter to help align objects
	float fidelity = 10.f;
	auto align = [&] (float x) { return floor(x * fidelity)/fidelity; };

	select_position = glm::vec3(align(view_direction.x*select_distance + view_position.x),
	                            align(view_direction.y*select_distance + view_position.y),
	                            align(view_direction.z*select_distance + view_position.z));
}

void testscene::physics(context& ctx) {
	// nothing to do yet
}

void testscene::logic(context& ctx) {
	Uint32 cur_ticks = SDL_GetTicks();
	Uint32 ticks_delta = cur_ticks - last_frame;

	float fticks = ticks_delta / 1000.0f;
	float angle = fticks * glm::radians(-45.f);

	last_frame = cur_ticks;

	view = glm::lookAt(view_position, view_position + view_direction, view_up);
}

void testscene::save_map(std::string name) {
	std::ofstream foo(name);
	std::cerr << "saving map " << name << std::endl;

	if (!foo.good()) {
		std::cerr << "couldn't open save file" << name << std::endl;
		return;
	}

	foo << "### test scene save file" << std::endl;

	for (auto& v : dynamic_models) {
		foo << v.name << "\t"
			<< v.position.x << "," << v.position.y << "," << v.position.z << "\t";

		for (unsigned y = 0; y < 4; y++) {
			for (unsigned x = 0; x < 4; x++) {
				foo << v.transform[y][x] << ",";
			}
		}

		foo << "\t" << v.inverted;
		foo << std::endl;
	}
}

void testscene::load_map(std::string name) {
	std::ifstream foo(name);
	std::cerr << "loading map " << name << std::endl;

	if (!foo.good()) {
		std::cerr << "couldn't open save file" << name << std::endl;
		return;
	}

	std::string line;
	while (std::getline(foo, line)) {
		auto statement = split_string(line, '\t');
		if (line[0] == '#' || line[0] == '\n' || statement.size() < 4) {
			continue;
		}

		auto posvec = split_string(statement[1], ',');
		auto matvec = split_string(statement[2], ',');

		editor_entry v;
		v.name = statement[0];
		v.position = glm::vec3(std::stof(posvec[0]), std::stof(posvec[1]), std::stof(posvec[2]));
		v.inverted = std::stoi(statement[3]);

		for (unsigned i = 0; i < 16; i++) {
			v.transform[i/4][i%4] = std::stof(matvec[i]);
		}

		dynamic_models.push_back(v);
		std::cerr << "# loaded a " << v.name << std::endl;
	}
}

int main(int argc, char *argv[]) {
	context ctx("grend test");
	std::unique_ptr<grend> scene(new testscene());

	while (scene->running) {
		scene->input(ctx);

		if (scene->running) {
			scene->logic(ctx);
			scene->render(ctx);
		}
	}

	return 0;
}
