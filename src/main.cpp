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

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <glm/ext.hpp>
#include <glm/gtc/matrix_transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>
//#include <glm/gtx/matrix_decompose.hpp>

//#define GL3_PROTOTYPES 1
//#include <GLES3/gl3.h>
#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

// premium HD resolution
#define SCREEN_SIZE_X 1366
#define SCREEN_SIZE_Y 768

#define ENABLE_MIPMAPS 1
#define ENABLE_FACE_CULLING 1

class context {
	public:
		context(const char *progname);
		~context();

		SDL_Window *window;
		SDL_GLContext glcontext;
		GLenum glew_status;
};

class material {
	public:
		material(){};
		material(glm::vec4 d, glm::vec4 a, glm::vec4 s, GLfloat shine) {
			diffuse = d, ambient = a, specular = s, shininess = shine;
		}

		glm::vec4 diffuse;
		glm::vec4 ambient;
		glm::vec4 specular;
		GLfloat   shininess = 10;

		// file names of textures
		// no ambient map, diffuse map serves as both
		std::string diffuse_map;
		std::string specular_map;
		std::string bump_map;
};

class model_submesh {
	public:
		std::string material = "(null)";
		std::vector<GLushort> faces;
};

class model {
	public:
		model(){};
		model(std::string filename);
		void load_object(std::string filename);
		void load_materials(std::string filename);
		void clear(void);

		void gen_all(void);
		void gen_normals(void);
		void gen_texcoords(void);

		std::vector<glm::vec3> vertices;
		std::vector<glm::vec3> normals;
		std::vector<GLfloat>  texcoords;

		std::map<std::string, model_submesh> meshes;
		std::map<std::string, material> materials;

		bool have_normals = false;
		bool have_texcoords = false;
};

std::map<std::string, material> default_materials = {
	{"(null)", {{0.75, 0.75, 0.75, 1}, {1, 1, 1, 1}, {0.5, 0.5, 0.5, 1}, 15}},
	{"Blk",    {{1, 0.8, 0.2, 1}, {1, 0.8, 0.2, 1}, {1, 1, 1, 1}, 50}},
	{"Grey",   {{0.1, 0.1, 0.1, 0.5}, {0.1, 0.1, 0.1, 0.2}, {0.0, 0.0, 0.0, 0.05}, 15}},
	{"Yellow", {{0.01, 0.01, 0.01, 1}, {0, 0, 0, 1}, {0.2, 0.2, 0.2, 0.2}, 20}},
	{"Steel",  {{1, 1, 1, 1}, {1, 1, 1, 1}, {1, 1, 1, 1}, 20}},
};

model generate_grid(int sx, int sy, int ex, int ey, int spacing) {
	model ret;

	unsigned i = 0;
	for (int y = sx; y <= ey; y += spacing) {
		for (int x = sx; x <= ex; x += spacing) {
			//float foo = sin(i*0.005f);
			//float foo = (x ^ y)*(1./ex);
			float foo = 0;

			ret.vertices.push_back(glm::vec3(x - spacing, foo, y - spacing));
			ret.vertices.push_back(glm::vec3(x - spacing, foo, y));
			ret.vertices.push_back(glm::vec3(x,           foo, y));

			ret.vertices.push_back(glm::vec3(x,           foo, y - spacing));
			ret.vertices.push_back(glm::vec3(x - spacing, foo, y - spacing));
			ret.vertices.push_back(glm::vec3(x,           foo, y));

			ret.texcoords.push_back(0); ret.texcoords.push_back(0);
			ret.texcoords.push_back(0); ret.texcoords.push_back(1);
			ret.texcoords.push_back(1); ret.texcoords.push_back(1);

			ret.texcoords.push_back(1); ret.texcoords.push_back(0);
			ret.texcoords.push_back(0); ret.texcoords.push_back(0);
			ret.texcoords.push_back(1); ret.texcoords.push_back(1);

			for (unsigned k = 0; k < 6; k++) {
				ret.meshes["default"].faces.push_back(i++);
				ret.normals.push_back(glm::vec3(0, 1, 0));
			}
		}
	}

	ret.have_normals = true;
	ret.have_texcoords = true;

	return ret;
}

model generate_cuboid(float width, float height, float depth) {
	model ret;

	float ax = width/2;
	float ay = height/2;
	float az = depth/2;

	// front
	ret.vertices.push_back(glm::vec3(-ax, -ay,  az));
	ret.vertices.push_back(glm::vec3( ax, -ay,  az));
	ret.vertices.push_back(glm::vec3( ax,  ay,  az));
	ret.vertices.push_back(glm::vec3(-ax,  ay,  az));

	// top
	ret.vertices.push_back(glm::vec3(-ax,  ay,  az));
	ret.vertices.push_back(glm::vec3( ax,  ay,  az));
	ret.vertices.push_back(glm::vec3( ax,  ay, -az));
	ret.vertices.push_back(glm::vec3(-ax,  ay, -az));

	// back
	ret.vertices.push_back(glm::vec3( ax, -ay, -az));
	ret.vertices.push_back(glm::vec3(-ax, -ay, -az));
	ret.vertices.push_back(glm::vec3(-ax,  ay, -az));
	ret.vertices.push_back(glm::vec3( ax,  ay, -az));

	// bottom
	ret.vertices.push_back(glm::vec3(-ax, -ay, -az));
	ret.vertices.push_back(glm::vec3( ax, -ay, -az));
	ret.vertices.push_back(glm::vec3( ax, -ay,  az));
	ret.vertices.push_back(glm::vec3(-ax, -ay,  az));

	// left
	ret.vertices.push_back(glm::vec3(-ax, -ay, -az));
	ret.vertices.push_back(glm::vec3(-ax, -ay,  az));
	ret.vertices.push_back(glm::vec3(-ax,  ay,  az));
	ret.vertices.push_back(glm::vec3(-ax,  ay, -az));

	// right
	ret.vertices.push_back(glm::vec3( ax, -ay,  az));
	ret.vertices.push_back(glm::vec3( ax, -ay, -az));
	ret.vertices.push_back(glm::vec3( ax,  ay, -az));
	ret.vertices.push_back(glm::vec3( ax,  ay,  az));

	for (unsigned i = 0; i < 24; i += 4) {
		ret.meshes["default"].faces.push_back(i);
		ret.meshes["default"].faces.push_back(i+1);
		ret.meshes["default"].faces.push_back(i+2);

		ret.meshes["default"].faces.push_back(i+2);
		ret.meshes["default"].faces.push_back(i+3);
		ret.meshes["default"].faces.push_back(i);

		ret.texcoords.push_back(0); ret.texcoords.push_back(0);
		ret.texcoords.push_back(1); ret.texcoords.push_back(0);
		ret.texcoords.push_back(1); ret.texcoords.push_back(1);
		ret.texcoords.push_back(0); ret.texcoords.push_back(1);
	}

	ret.gen_normals();

	return ret;
}

static std::vector<std::string> split_string(std::string s, char delim=' ') {
	std::vector<std::string> ret;
	std::size_t pos = std::string::npos, last = 0;

	for (pos = s.find(delim); pos != std::string::npos; pos = s.find(delim, pos + 1)) {
		ret.push_back(s.substr(last, pos - last));
		last = pos + 1;
	}

	ret.push_back(s.substr(last));
	return ret;
}

model::model(std::string filename) {
	load_object(filename);
}

void model::clear(void) {
	meshes.clear();
}

void model::gen_normals(void) {
	std::cerr << " > generating new normals... " << vertices.size() << std::endl;
	normals.resize(vertices.size(), glm::vec3(0));

	for (auto& thing : meshes) {
		for (unsigned i = 0; i < thing.second.faces.size(); i += 3) {
			GLushort elms[3] = {
				thing.second.faces[i],
				thing.second.faces[i+1],
				thing.second.faces[i+2]
			};

			glm::vec3 normal = glm::normalize(
					glm::cross(
						vertices[elms[1]] - vertices[elms[0]],
						vertices[elms[2]] - vertices[elms[0]]));

			normals[elms[0]] = normals[elms[1]] = normals[elms[2]] = normal;
		}
	}
}

void model::gen_texcoords(void) {
	std::cerr << " > generating new texcoords... " << vertices.size() << std::endl;
	texcoords.resize(2*vertices.size());

	for (unsigned i = 0; i < vertices.size(); i++) {
		glm::vec3& foo = vertices[i];
		texcoords[2*i]   = foo.x;
		texcoords[2*i+1] = foo.y;
	}
}

static std::string base_dir(std::string filename) {
	std::size_t found = filename.rfind("/");

	if (found != std::string::npos) {
		return filename.substr(0, found) + "/";
	}

	return filename;
}

void model::load_object(std::string filename) {
	std::ifstream input(filename);
	std::string line;
	std::string current_mesh = "default";

	std::vector<glm::vec3> vertbuf;
	std::vector<glm::vec3> normbuf;
	std::vector<GLfloat> texbuf;

	if (!input.good()) {
		// TODO: exception
		std::cerr << " ! couldn't load object from " << filename << std::endl;
		return;
	}

	// in case we're reloading over a previously-loaded object
	clear();

	while (std::getline(input, line)) {
		auto statement = split_string(line);

		if (statement.size() < 2)
			continue;

		if (statement[0] == "o") {
			std::cerr << " > have submesh " << statement[1] << std::endl;
			current_mesh = statement[1];
		}

		else if (statement[0] == "mtllib") {
			std::string temp = base_dir(filename) + statement[1];
			std::cerr << " > using material " << temp << std::endl;
			load_materials(temp);
		}

		else if (statement[0] == "usemtl") {
			std::cerr << " > using material " << statement[1] << std::endl;
			meshes[current_mesh].material = statement[1];
		}

		else if (statement[0] == "v") {
			glm::vec3 v = glm::vec3(std::stof(statement[1]),
			                        std::stof(statement[2]),
			                        std::stof(statement[3]));
			vertbuf.push_back(v);
		}

		else if (statement[0] == "f") {
			std::size_t end = statement.size();
			unsigned elements_added = 0;

			// XXX: we should be checking for buffer ranges here
			auto load_face_tri = [&] (std::string& statement) {
				auto spec = split_string(statement, '/');
				unsigned vert_index = std::stoi(spec[0]) - 1;

				vertices.push_back(vertbuf[vert_index]);
				meshes[current_mesh].faces.push_back(vertices.size() - 1);

				if (spec.size() > 1 && !spec[1].empty()) {
					unsigned buf_index = 2*(std::stoi(spec[1]) - 1);
					texcoords.push_back(texbuf[buf_index]);
					texcoords.push_back(texbuf[buf_index + 1]);
				}

				if (spec.size() > 2 && !spec[2].empty()) {
					normals.push_back(normbuf[std::stoi(spec[2]) - 1]);
				}
			};

			for (std::size_t cur = 1; cur + 2 < end; cur++) {
				load_face_tri(statement[1]);

				for (unsigned k = 1; k < 3; k++) {
					load_face_tri(statement[cur + k]);
				}
			}
		}

		else if (statement[0] == "vn") {
			glm::vec3 v = glm::vec3(std::stof(statement[1]),
			                        std::stof(statement[2]),
			                        std::stof(statement[3]));
			//normals.push_back(glm::normalize(v));
			normbuf.push_back(v);
			have_normals = true;
		}

		else if (statement[0] == "vt") {
			texbuf.push_back(std::stof(statement[1]));
			texbuf.push_back(std::stof(statement[2]));
			have_texcoords = true;
		}
	}

	// TODO: check that normals size == vertices size and fill in the difference
	if (!have_normals) {
		gen_normals();
	}

	if (!have_texcoords) {
		gen_texcoords();
	}

	if (normals.size() != vertices.size()) {
		std::cerr << " ? mismatched normals and vertices: "
			<< normals.size() << ", "
			<< vertices.size()
			<< std::endl;
		// TODO: should handle this
	}

	if (texcoords.size()/2 != vertices.size()) {
		std::cerr << " ? mismatched texcoords and vertices: "
			<< texcoords.size() << ", "
			<< vertices.size()
			<< std::endl;
		// TODO: should handle this
	}
}

void model::load_materials(std::string filename) {
	std::ifstream input(filename);
	std::string current_material = "default";
	std::string line;

	if (!input.good()) {
		// TODO: exception
		std::cerr << "Warning: couldn't load material library from "
			<< filename << std::endl;
		return;
	}

	while (std::getline(input, line)) {
		auto statement = split_string(line);

		if (statement.size() < 2) {
			continue;
		}

		if (statement[0] == "newmtl") {
			std::cerr << "   - new material: " << statement[1] << std::endl;
			current_material = statement[1];
		}

		else if (statement[0] == "Ka") {
			materials[current_material].ambient = glm::vec4(std::stof(statement[1]),
			                                                std::stof(statement[2]),
			                                                std::stof(statement[3]), 1);
		}

		else if (statement[0] == "Kd") {
			materials[current_material].diffuse = glm::vec4(std::stof(statement[1]),
			                                                std::stof(statement[2]),
			                                                std::stof(statement[3]), 1);

		}

		else if (statement[0] == "Ks") {
			materials[current_material].specular = glm::vec4(std::stof(statement[1]),
			                                                 std::stof(statement[2]),
			                                                 std::stof(statement[3]), 1);
		}

		else if (statement[0] == "Ns") {
			materials[current_material].shininess = std::stof(statement[1]);
		}

		else if (statement[0] == "map_Kd") {
			materials[current_material].diffuse_map = base_dir(filename) + statement[1];
		}

		// TODO: other light maps, attributes
	}
}

void SDL_Die(const char *message) {
	SDL_Quit();
	throw std::logic_error(message);
}

void display_color(SDL_Window *window, float r, float g, float b) {
	glClearColor(r, g, b, 1.0);
	glClear(GL_COLOR_BUFFER_BIT);
	SDL_GL_SwapWindow(window);
}

context::context(const char *progname) {
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		SDL_Die("Couldn't initialize video.");
	}

	if (IMG_Init(IMG_INIT_JPG | IMG_INIT_PNG) != (IMG_INIT_JPG | IMG_INIT_PNG)) {
		SDL_Die("Couldn't initialize sdl2_image");
	}

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);

	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);

	window = SDL_CreateWindow(progname, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
	                          SCREEN_SIZE_X, SCREEN_SIZE_Y,
	                          SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);

	SDL_ShowCursor(SDL_DISABLE);

	if (!window) {
		SDL_Die("Couldn't create a window");
	}

	glcontext = SDL_GL_CreateContext(window);
	SDL_GL_SetSwapInterval(1);

	if ((glew_status = glewInit()) != GLEW_OK) {
		SDL_Die("glewInit()");
	}
}

context::~context() {
	SDL_GL_DeleteContext(glcontext);
	SDL_DestroyWindow(window);
	IMG_Quit();
	SDL_Quit();
}

std::string load_file(const std::string filename) {
	std::ifstream ifs(filename);
	std::string content((std::istreambuf_iterator<char>(ifs)),
	                    (std::istreambuf_iterator<char>()));

	return content;
}

class grend {
	public:
		grend();
		~grend() { free_objects(); };
		bool running = true;

		virtual void render(context& ctx) = 0;
		virtual void logic(context& ctx) = 0;
		virtual void physics(context& ctx) = 0;
		virtual void input(context& ctx) = 0;

		// TODO: we could have a parent field so we can catch mismatched vbo->vao binds
		//       which could be neat
		typedef std::pair<GLuint, std::size_t> rhandle;
		typedef std::map<std::string, model_submesh> mesh_map;
		typedef std::map<std::string, model> model_map;

		class compiled_mesh {
			public:
				std::string material;
				rhandle vao;

				GLint elements_size;
				void *elements_offset;
		};
		typedef std::map<std::string, compiled_mesh> cooked_mesh_map;
		
		class compiled_model {
			public:
				rhandle vao;

				GLint vertices_size;
				GLint normals_size;
				GLint texcoords_size;

				std::vector<std::string> meshes;
				// NOTE: duplicating materials here because the model may not be valid
				//       for the lifetime of the compiled model, and there's some possible
				//       optimizations to be done in buffering all the material info
				//       to the shaders during initialization
				std::map<std::string, material> materials;
				std::map<std::string, rhandle> mat_textures;

				void *vertices_offset;
				void *normals_offset;
				void *texcoords_offset;
		};
		typedef std::map<std::string, compiled_model> cooked_model_map;

		void compile_meshes(std::string objname, const mesh_map& meshies);
		void compile_models(model_map& models);
		rhandle preload_mesh_vao(compiled_model& obj, compiled_mesh& mesh);
		rhandle preload_model_vao(compiled_model& mesh);
		void bind_cooked_meshes(void);

		cooked_mesh_map  cooked_meshes;
		cooked_model_map cooked_models;

		std::vector<glm::vec3> cooked_vertices;
		std::vector<GLushort> cooked_elements;
		std::vector<glm::vec3> cooked_normals;
		std::vector<GLfloat> cooked_texcoords;

		rhandle cooked_vert_vbo;
		rhandle cooked_element_vbo;
		rhandle cooked_normal_vbo;
		rhandle cooked_texcoord_vbo;

		rhandle gen_vao(void);
		rhandle gen_vbo(void);
		rhandle gen_texture(void);
		rhandle gen_shader(GLuint type);
		rhandle gen_program(void);

		rhandle bind_vao(const rhandle& handle);
		rhandle bind_vbo(const rhandle& handle, GLuint type);

		rhandle va_pointer(const rhandle& handle, GLuint width, GLuint type);
		rhandle enable_vbo(const rhandle& handle);

		rhandle buffer_vbo(const rhandle& handle, GLuint type, const std::vector<GLfloat>& vec);
		rhandle buffer_vbo(const rhandle& handle, GLuint type, const std::vector<GLushort>& vec);
		rhandle buffer_vbo(const rhandle& handle, GLuint type, const std::vector<glm::vec3>& vec);

		// XXX: yeah it's a bit obfuscated, but this turns a lot of boilerplate
		//      into just one function call
		// bind, load and enable vec3(float) attribute array buffer
		rhandle load_vec3f_ab_vattrib(const std::vector<GLfloat>& vec);
		rhandle load_vec3f_ab_vattrib(const std::vector<glm::vec3>& vec);
		// bind, load and enable vec3(unsigned short) attribute element array buffer
		rhandle load_vec3us_eab_vattrib(const std::vector<GLushort>& vec);

		void free_objects(void);

		// TODO: unload
		rhandle load_texture(std::string filename);
		rhandle load_shader(std::string filename, GLuint type);

		std::vector<GLuint> vaos;
		std::vector<GLuint> vbos;
		std::vector<GLuint> shaders;
		std::vector<GLuint> programs;
		std::vector<GLuint> textures;

		// frames rendered
		unsigned frames = 0;
		rhandle current_vao;
};

class testscene : public grend {
	public:
		testscene();
		virtual ~testscene();
		virtual void render(context& ctx);
		virtual void logic(context& ctx);
		virtual void physics(context& ctx);
		virtual void input(context& ctx);

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

		glm::vec3 select_position = glm::vec3(0, 0, 0);
		float     select_distance = 5;
		// TODO: obj rotation
		// TODO: model types
		std::vector<glm::vec3> dynamic_models;

		Uint32 last_frame;

		// models
		model_map models = {
			{"ground",    model("assets/obj/Modular Terrain Hilly/Grass_Flat.obj")},
			{"path",      model("assets/obj/Modular Terrain Hilly/Path_Center.obj")},
			{"grass",     model("assets/obj/Modular Terrain Hilly/Prop_Grass_Clump_1.obj")},
			{"stump",     model("assets/obj/Modular Terrain Hilly/Prop_Stump.obj")},
			{"tree",      model("assets/obj/Modular Terrain Hilly/Prop_Tree_Oak_1.obj")},
			{"person",    model("assets/obj/low-poly-character-rpg/boy.obj")},

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

grend::grend() {
	std::cerr << " # Got here, " << __func__ << std::endl;
	std::cerr << " # maximum texture units: " << GL_MAX_TEXTURE_UNITS << std::endl;
	std::cerr << " # maximum combined texture units: " << GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS << std::endl;

	cooked_vertices.clear();
	cooked_normals.clear();
	cooked_elements.clear();
	cooked_texcoords.clear();

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	/*
	// testing texcoord generation
	glEnable(GL_TEXTURE_GEN_S);
	glEnable(GL_TEXTURE_GEN_T);
	glEnable(GL_TEXTURE_GEN_R);
	glEnable(GL_TEXTURE_GEN_Q);
	*/
}

void grend::compile_meshes(std::string objname, const grend::mesh_map& meshies) {
	for (const auto& x : meshies) {
		compiled_mesh foo;
		std::string meshname = objname + "." + x.first;

		//foo.elements_size = x.second.faces.size() * sizeof(GLushort);
		foo.elements_size = x.second.faces.size();
		foo.elements_offset = reinterpret_cast<void*>(cooked_elements.size() * sizeof(GLushort));
		cooked_elements.insert(cooked_elements.end(), x.second.faces.begin(),
		                                              x.second.faces.end());

		// TODO: fix this to work with per-model material maps
		/*
		// check to make sure we have all the materials loaded
		if (materials.find(x.second.material) == materials.end()) {
			std::cerr
				<< "Warning: material \"" << x.second.material
				<< "\" for mesh " << meshname
				<< " isn't loaded!"
				<< std::endl;
			foo.material = "(null)";

		} else {
			foo.material = x.second.material;
		}
		*/

		foo.material = x.second.material;

		cooked_meshes[meshname] = foo;
	}
	fprintf(stderr, " > elements size %lu\n", cooked_elements.size());
}

void grend::compile_models(model_map& models) {
	for (const auto& x : models) {
		std::cerr << " >>> compiling " << x.first << std::endl;
		compiled_model obj;

		obj.vertices_size = x.second.vertices.size() * sizeof(glm::vec3);
		obj.vertices_offset = reinterpret_cast<void*>(cooked_vertices.size() * sizeof(glm::vec3));
		cooked_vertices.insert(cooked_vertices.end(), x.second.vertices.begin(),
		                                              x.second.vertices.end());
		fprintf(stderr, " > vertices size %lu\n", cooked_vertices.size());

		obj.normals_size = x.second.normals.size() * sizeof(glm::vec3);
		obj.normals_offset = reinterpret_cast<void*>(cooked_normals.size() * sizeof(glm::vec3));
		cooked_normals.insert(cooked_normals.end(), x.second.normals.begin(),
		                                            x.second.normals.end());
		fprintf(stderr, " > normals size %lu\n", cooked_normals.size());

		obj.texcoords_size = x.second.texcoords.size() * sizeof(GLfloat) * 2;
		obj.texcoords_offset = reinterpret_cast<void*>(cooked_texcoords.size() * sizeof(GLfloat));
		cooked_texcoords.insert(cooked_texcoords.end(),
				x.second.texcoords.begin(),
				x.second.texcoords.end());
		fprintf(stderr, " > texcoords size %lu\n", cooked_texcoords.size());

		compile_meshes(x.first, x.second.meshes);

		// copy mesh names
		for (const auto& m : x.second.meshes) {
			std::string asdf = x.first + "." + m.first;
			std::cerr << " > have cooked mesh " << asdf << std::endl;
			obj.meshes.push_back(asdf);
		}

		// copy materials
		for (const auto& mat : x.second.materials) {
			obj.materials[mat.first] = mat.second;

			if (!mat.second.diffuse_map.empty()) {
				obj.mat_textures[mat.first] = load_texture(mat.second.diffuse_map);
			}
		}

		cooked_models[x.first] = obj;
	}
}

grend::rhandle grend::preload_mesh_vao(compiled_model& obj, compiled_mesh& mesh) {
	rhandle orig_vao = current_vao;
	rhandle ret = bind_vao(gen_vao());

	bind_vbo(cooked_vert_vbo, GL_ARRAY_BUFFER);
	glVertexAttribPointer(cooked_vert_vbo.first, 3, GL_FLOAT, GL_FALSE, 0, obj.vertices_offset);
	enable_vbo(cooked_vert_vbo);

	bind_vbo(cooked_normal_vbo, GL_ARRAY_BUFFER);
	glVertexAttribPointer(cooked_normal_vbo.first, 3, GL_FLOAT, GL_FALSE, 0, obj.normals_offset);
	enable_vbo(cooked_normal_vbo);

	bind_vbo(cooked_texcoord_vbo, GL_ARRAY_BUFFER);
	glVertexAttribPointer(cooked_texcoord_vbo.first, 2, GL_FLOAT, GL_FALSE, 0, obj.texcoords_offset);
	enable_vbo(cooked_texcoord_vbo);

	bind_vbo(cooked_element_vbo, GL_ELEMENT_ARRAY_BUFFER);
	glVertexAttribPointer(cooked_element_vbo.first, 3, GL_UNSIGNED_SHORT, GL_FALSE, 0, mesh.elements_offset);
	enable_vbo(cooked_element_vbo);

	bind_vao(orig_vao);
	return ret;
}

grend::rhandle grend::preload_model_vao(compiled_model& obj) {
	rhandle orig_vao = current_vao;

	for (std::string& mesh_name : obj.meshes) {
		std::cerr << " # binding " << mesh_name << std::endl;
		cooked_meshes[mesh_name].vao = preload_mesh_vao(obj, cooked_meshes[mesh_name]);
	}

	return orig_vao;
}

void grend::bind_cooked_meshes(void) {
	cooked_vert_vbo = gen_vbo();
	cooked_element_vbo = gen_vbo();
	cooked_normal_vbo = gen_vbo();
	cooked_texcoord_vbo = gen_vbo();

	buffer_vbo(cooked_vert_vbo, GL_ARRAY_BUFFER, cooked_vertices);
	buffer_vbo(cooked_normal_vbo, GL_ARRAY_BUFFER, cooked_normals);
	buffer_vbo(cooked_texcoord_vbo, GL_ARRAY_BUFFER, cooked_texcoords);
	buffer_vbo(cooked_element_vbo, GL_ELEMENT_ARRAY_BUFFER, cooked_elements);

	for (auto& x : cooked_models) {
		std::cerr << " # binding " << x.first << std::endl;
		x.second.vao = preload_model_vao(x.second);
	}
}

void grend::free_objects() {
	std::cerr << " # Got here, " << __func__ << std::endl;
	glUseProgram(0);

	fprintf(stderr, "   - %lu shaders\n", shaders.size());
	/*
	for (auto& thing : shaders) {
		// XXX: should detach shaders here, need to keep track of
		//      what shaders are owned where
		// glDetachShader()
	}
	*/

	fprintf(stderr, "   - %lu programs\n", programs.size());
	for (auto& thing : programs) {
		glDeleteProgram(thing);
	}

	fprintf(stderr, "   - %lu VBOs\n", vbos.size());
	for (auto& thing : vbos) {
		glDeleteBuffers(1, &thing);
	}

	fprintf(stderr, "   - %lu VAOs\n", vaos.size());
	for (auto& thing : vaos) {
		glDeleteVertexArrays(1, &thing);
	}

	fprintf(stderr, "   - %lu textures\n", textures.size());
	for (auto& thing : textures) {
		glDeleteTextures(1, &thing);
	}

	std::cerr << " # done cleanup" << std::endl;
}

grend::rhandle grend::gen_vao(void) {
	GLuint temp;
	glGenVertexArrays(1, &temp);
	vaos.push_back(temp);
	return {temp, vaos.size() - 1};
}

grend::rhandle grend::gen_vbo(void) {
	GLuint temp;
	glGenBuffers(1, &temp);
	vbos.push_back(temp);

	return {temp, vbos.size() - 1};
}

grend::rhandle grend::gen_texture(void) {
	GLuint temp;
	glGenTextures(1, &temp);
	textures.push_back(temp);
	return {temp, textures.size() - 1};
}

grend::rhandle grend::gen_shader(GLuint type) {
	GLuint temp = glCreateShader(type);
	shaders.push_back(temp);
	return {temp, shaders.size() - 1};
}

grend::rhandle grend::gen_program(void) {
	GLuint temp = glCreateProgram();
	programs.push_back(temp);
	return {temp, programs.size() - 1};
}

grend::rhandle grend::bind_vao(const grend::rhandle& handle) {
	current_vao = handle;
	glBindVertexArray(handle.first);
	return handle;
}

grend::rhandle grend::bind_vbo(const grend::rhandle& handle, GLuint type) {
	glBindBuffer(type, handle.first);
	return handle;
}

grend::rhandle grend::va_pointer(const grend::rhandle& handle, GLuint width, GLuint type) {
	glVertexAttribPointer(handle.first, width, type, GL_FALSE, 0, 0);
	return handle;
}

grend::rhandle grend::enable_vbo(const grend::rhandle& handle) {
	glEnableVertexAttribArray(handle.first);
	return handle;
}

grend::rhandle grend::buffer_vbo(const grend::rhandle& handle,
                                 GLuint type,
                                 const std::vector<GLfloat>& vec)
{
	bind_vbo(handle, type);
	glBufferData(type, sizeof(GLfloat) * vec.size(), vec.data(), GL_STATIC_DRAW);
	return handle;
}

grend::rhandle grend::buffer_vbo(const grend::rhandle& handle,
                                 GLuint type,
                                 const std::vector<glm::vec3>& vec)
{
	bind_vbo(handle, type);
	glBufferData(type, sizeof(glm::vec3) * vec.size(), vec.data(), GL_STATIC_DRAW);
	return handle;
}

grend::rhandle grend::buffer_vbo(const grend::rhandle& handle,
                                 GLuint type,
                                 const std::vector<GLushort>& vec)
{
	bind_vbo(handle, type);
	glBufferData(type, sizeof(GLushort) * vec.size(), vec.data(), GL_STATIC_DRAW);
	return handle;
}

grend::rhandle grend::load_vec3f_ab_vattrib(const std::vector<GLfloat>& vec) {
	return va_pointer(enable_vbo(buffer_vbo(gen_vbo(), GL_ARRAY_BUFFER, vec)), 3, GL_FLOAT);
}

grend::rhandle grend::load_vec3f_ab_vattrib(const std::vector<glm::vec3>& vec) {
	return va_pointer(enable_vbo(buffer_vbo(gen_vbo(), GL_ARRAY_BUFFER, vec)), 3, GL_FLOAT);
}

grend::rhandle grend::load_vec3us_eab_vattrib(const std::vector<GLushort>& vec) {
	return va_pointer(enable_vbo(buffer_vbo(gen_vbo(), GL_ELEMENT_ARRAY_BUFFER, vec)), 3, GL_UNSIGNED_SHORT);
}

grend::rhandle grend::load_texture(std::string filename) {
	SDL_Surface *texture = IMG_Load(filename.c_str());

	if (!texture) {
		SDL_Die("Couldn't load texture");
	}

	fprintf(stderr, " > loaded image: w = %u, h = %u, pitch = %u, bytesperpixel: %u\n",
	        texture->w, texture->h, texture->pitch, texture->format->BytesPerPixel);

	GLenum texformat = GL_RGBA;
	switch (texture->format->BytesPerPixel) {
		case 1: texformat = GL_R; break;
		case 2: texformat = GL_RG; break;
		case 3: texformat = GL_RGB; break;
		case 4: texformat = GL_RGBA; break;
		default: break;
	}

	/*
	glGenTextures(1, &texture_id);
	glBindTexture(GL_TEXTURE_2D, texture_id);
	*/
	rhandle temp = gen_texture();
	glBindTexture(GL_TEXTURE_2D, temp.first);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D,
	             0, texformat, texture->w, texture->h, 0, texformat,
	             GL_UNSIGNED_BYTE, texture->pixels);

#if ENABLE_MIPMAPS
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glGenerateMipmap(GL_TEXTURE_2D);
#else
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
#endif

	SDL_FreeSurface(texture);

	return temp;
}

grend::rhandle grend::load_shader(const std::string filename, GLuint type) {
	std::string source = load_file(filename);
	const char *temp = source.c_str();
	int compiled;
	//GLuint ret = glCreateShader(type);
	rhandle ret = gen_shader(type);

	glShaderSource(ret.first, 1, (const GLchar**)&temp, 0);
	glCompileShader(ret.first);
	glGetShaderiv(ret.first, GL_COMPILE_STATUS, &compiled);

	if (!compiled) {
		int max_length;
		char *shader_log;

		glGetShaderiv(ret.first, GL_INFO_LOG_LENGTH, &max_length);
		shader_log = new char[max_length];
		glGetShaderInfoLog(ret.first, max_length, &max_length, shader_log);

		throw std::logic_error(filename + ": " + shader_log);
	}

	return ret;
}

testscene::testscene() : grend() {
	projection = glm::perspective(glm::radians(60.f),
	                             (1.f*SCREEN_SIZE_X)/SCREEN_SIZE_Y, 0.1f, 100.f);
	view = glm::lookAt(glm::vec3(0.0, 1.0, 5.0),
	                   glm::vec3(0.0, 0.0, 0.0),
	                   glm::vec3(0.0, 1.0, 0.0));

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
	for (std::string& name : obj.meshes) {
		draw_mesh_lines(cooked_meshes[name], transform);
	}
}

void testscene::render(context& ctx) {
	glClearColor(0.1, 0.1, 0.1, 1);
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
				draw_model(cooked_models["path"], trans);

			} else {
				draw_model(cooked_models["ground"], trans);
				trans = glm::translate(glm::vec3(x, 0.2, y));

				if (abs(x) == 12 && abs(y) == 12) {
					draw_model(cooked_models["tree"], trans);

				} else if (rand() % 3 == 0) {
					draw_model(cooked_models["grass"], trans);

				} else if (rand() % 17 == 0) {
					draw_model(cooked_models["stump"], trans);
				}
			}
		}
	}

	glm::vec3 hpos = glm::vec3(view_direction.x*10 + view_position.x, 0, view_direction.z*10 + view_position.z);
	glm::mat4 bizz = glm::translate(glm::mat4(1), hpos);
	draw_model(cooked_models["person"], bizz);

	draw_model(cooked_models["ground"], glm::translate(select_position));
	draw_model_lines(cooked_models["ground"], glm::translate(select_position));

	for (auto& v : dynamic_models) {
		draw_model(cooked_models["ground"], glm::translate(v));
	}

	for (unsigned i = 0; i < 16; i++) {
		float time_component = i + M_PI*SDL_GetTicks()/1000.f;
		const float fpi = 3.1415926f;

		glm::mat4 model = glm::translate(glm::mat4(1),
			glm::vec3(-32.f + 4*i, 4 + sin(time_component), 0))
			* glm::rotate(fpi*cosf(0.025 * fpi * time_component), glm::vec3(0, 1, 0));

		compiled_model& foo =
		     (i%4 == 0)? cooked_models["grass"]
		   : (i%4 == 1)? cooked_models["stump"]
		   : (i%4 == 2)? cooked_models["tree"]
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
				case SDLK_x: running = false; break;
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

		else if (ev.type == SDL_MOUSEWHEEL) {
			select_distance -= ev.wheel.y/10.f /* fidelity */;
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

	if (buttons & SDL_BUTTON(SDL_BUTTON_LEFT)) {
		// TODO: should check for button down, this will usually place multiple objects otherwise
		dynamic_models.push_back(select_position);
	}
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
