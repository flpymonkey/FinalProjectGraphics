#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <ctime>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>

// OpenGL library includes
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "debuggl.h"

// Classes
#include "floor.h"
#include "menger.h"
#include "camera.h"
#include "controller.h"
#include "render_pass.h"
#include "lights.h"
#include "filesystem.h"

#include "loader.h"
#include "mesh.h"
#include "material.h"
#include "filesystem.h"
#include "object.h"

//gui
#include "gui.h"

// game state booleans:
bool showGeometry = true;
bool showGui = false;
// ====================

struct MatrixPointers {
	const float *projection, *model, *view;
};

glm::mat4 projection_matrix;
glm::mat4 view_matrix;
glm::mat4 model_matrix;

float aspect = 0.0f;

int window_width = 1080;
int window_height = 720;

// Used to brighten hdr exposure shader as described in this tutorial:
// https://learnopengl.com/Advanced-Lighting/HDR
float exposure = 0.8f;
bool showMeshes = true;
bool lensEffects = false;
bool captureImage = false;

// Initialize static member of class Box
int Object::object_count = 0;

Floor* g_floor;
Menger *g_menger;
Camera* g_camera;
Controller* g_controller;

// Ass importer
// Assimp::Importer importer;

// error while loading shared libraries: libassimp.so.3:
// cannot open shared object file: No such file or directory

// VBO and VAO descriptors.
enum { kVertexBuffer, kNormalBuffer, kIndexBuffer, kNumVbos };

// These are our VAOs.
enum { kGeometryVao, kFloorVao, kScreenVao, kNumVaos };

GLuint g_array_objects[kNumVaos];  // This will store the VAO descriptors.
GLuint g_buffer_objects[kNumVaos][kNumVbos];  // These will store VBO descriptors.

// Shaders
const char* vertex_shader =
#include "shaders/default.vert"
;

const char* fragment_shader =
#include "shaders/default.frag"
;

const char* floor_fragment_shader =
#include "shaders/floor.frag"
;

const char* screen_vertex_shader =
#include "shaders/screen_default.vert"
;

const char* screen_fragment_shader =
#include "shaders/screen_effects.frag"
;

const char* screen_downsample_shader =
#include "shaders/screen_downsample.frag"
;

const char* screen_lensflare_shader =
#include "shaders/screen_lensflare.frag"
;

const char* screen_blur_shader =
#include "shaders/screen_blur.frag"
;

const char* screen_blur2_shader =
#include "shaders/screen_blur2.frag"
;

const char* screen_hdr_shader =
#include "shaders/screen_hdr.frag"
;

const char* screen_brightness_shader =
#include "shaders/screen_brightness.frag"
;

const char* object_vertex_shader =
#include "shaders/object.vert"
;

const char* object_fragment_shader =
#include "shaders/object.frag"
;



void
ErrorCallback(int error, const char* description)
{
	std::cerr << "GLFW Error: " << description << "\n";
}

void
printVec3(const char* name, glm::vec3 data)
{
    printf("%s: (%f, %f, %f)\n", name, data.x, data.y, data.z);
}

void
printUvec3(const char* name, glm::uvec3 data)
{
    printf("%s: (%d, %d, %d)\n", name, data.x, data.y, data.z);
}

void
printVec4(const char* name, glm::vec4 data)
{
    printf("%s: (%f, %f, %f, %f)\n", name, data.x, data.y, data.z, data.w);
}

int main(int argc, char* argv[])
{
	std::string window_title = "LENSTIME";
	if (!glfwInit()) exit(EXIT_FAILURE);

	// Setup
	g_floor = new Floor();
	g_menger = new Menger();
	g_camera = new Camera();

	glfwSetErrorCallback(ErrorCallback);

	// Ask an OpenGL 3.3 core profile context
	// It is required on OSX and non-NVIDIA Linux
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	GLFWwindow* window = glfwCreateWindow(window_width, window_height,
			&window_title[0], nullptr, nullptr);
	CHECK_SUCCESS(window != nullptr);
	glfwMakeContextCurrent(window);
	glewExperimental = GL_TRUE;

	// Controller
	g_controller = new Controller(window, g_camera, g_menger, &exposure, &showMeshes, &lensEffects, &captureImage);

	CHECK_SUCCESS(glewInit() == GLEW_OK);
	glGetError();  // clear GLEW's error for it

	glfwSwapInterval(1);
	const GLubyte* renderer = glGetString(GL_RENDERER);  // get renderer string
	const GLubyte* version = glGetString(GL_VERSION);    // version as a string
	std::cout << "Renderer: " << renderer << "\n";
	std::cout << "OpenGL version supported:" << version << "\n";

	glEnable(GL_CULL_FACE); // Added to see faces are correct.

	// <<<Lights>>>
	std::vector<DirectionalLight> directionalLights;
	DirectionalLight directionalLight = DirectionalLight(glm::vec3(-1.0f, -1.0f, -1.0f));
	directionalLights.push_back(directionalLight);

	// FIXME: THESE POINT LIGHT POSITIONS ARE SET TO SAME POSITIONS AS CUBE LIGHTS
	std::vector<PointLight> pointLights;
	PointLight pointLight = PointLight(glm::vec3(0.0f, 0.0f, -5.0f));
	pointLights.push_back(pointLight);
	pointLight = PointLight(glm::vec3(2.0f, 3.0f, -8.0f));
	pointLights.push_back(pointLight);

	std::vector<SpotLight> spotLights;
	SpotLight spotLight = SpotLight(glm::vec3(0.0f, 5.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f));
	//spotLights.push_back(spotLight);
	// <<<Lights>>>

	// <<<Renderpass Setup>>>
 	projection_matrix = glm::perspective(glm::radians(45.0f), aspect, 0.0001f, 1000.0f);
	view_matrix = g_camera->get_view_matrix();
	model_matrix = glm::mat4(1.0f);

	glm::vec4 light_position = glm::vec4(5.0f, 5.0f, 5.0f, 1.0f);

	MatrixPointers mats; // Define MatrixPointers here for lambda to capture
	mats.projection = &projection_matrix[0][0];
	mats.model= &model_matrix[0][0];
	mats.view = &view_matrix[0][0];

	/*
	 * In the following we are going to define several lambda functions to bind Uniforms.
	 *
	 * Introduction about lambda functions:
	 *      http://en.cppreference.com/w/cpp/language/lambda
	 *      http://www.stroustrup.com/C++11FAQ.html#lambda
	 */
	auto matrix_binder = [](int loc, const void* data) {
		glUniformMatrix4fv(loc, 1, GL_FALSE, (const GLfloat*)data);
	};

	auto vector_binder = [](int loc, const void* data) {
		glUniform4fv(loc, 1, (const GLfloat*)data);
	};

	glm::mat4 menger_model_matrix = glm::mat4(1.0f);
	auto menger_model_data = [&menger_model_matrix]() -> const void* {
		return &menger_model_matrix[0][0];
	}; // This return model matrix for the menger.

	glm::mat4 floor_model_matrix = glm::mat4(1.0f);
	auto floor_model_data = [&floor_model_matrix]() -> const void* {
		return &floor_model_matrix[0][0];
	}; // This return model matrix for the floor.

    auto std_model_data = [&mats]() -> const void* {
		return mats.model;
	};

	auto std_view_data = [&mats]() -> const void* {
		return mats.view;
	};

	auto std_proj_data = [&mats]() -> const void* {
		return mats.projection;
	};

	auto std_light_data = [&light_position]() -> const void* {
		return &light_position[0];
	};

	glm::vec4 eye_position = glm::vec4(g_camera->getPosition(), 1.0f);

	auto std_view_position_data = [&eye_position]() -> const void* {
		return &eye_position[0];
	};

    ShaderUniform menger_model = { "model", matrix_binder, menger_model_data};
	ShaderUniform floor_model = { "model", matrix_binder, floor_model_data};
	ShaderUniform std_view = { "view", matrix_binder, std_view_data };
    ShaderUniform std_model = { "model", matrix_binder, std_model_data };
	ShaderUniform std_proj = { "projection", matrix_binder, std_proj_data };
	ShaderUniform std_light = { "light_position", vector_binder, std_light_data };
	ShaderUniform std_view_position = { "view_position", vector_binder, std_view_position_data };
	// <<<RenderPass Setup>>>

  // <<<Menger Data>>>
  std::vector<glm::vec4> menger_vertices;
  std::vector<glm::vec4> menger_normals;
  std::vector<glm::uvec3> menger_faces;

  glm::vec4 menger_pos = glm::vec4(0.0f, 0.0f, -5.0f, 1.0f);

	g_menger->set_nesting_level(1);
	g_menger->generate_geometry(menger_vertices, menger_normals, menger_faces, menger_pos);
	g_menger->set_clean();

	glm::vec4 min_bounds = glm::vec4(std::numeric_limits<float>::max());
	glm::vec4 max_bounds = glm::vec4(-std::numeric_limits<float>::max());
	for (int i = 0; i < menger_vertices.size(); ++i) {
		min_bounds = glm::min(menger_vertices[i], min_bounds);
		max_bounds = glm::max(menger_vertices[i], max_bounds);
	}
	std::cout << "min_bounds = " << glm::to_string(min_bounds) << "\n";
	std::cout << "max_bounds = " << glm::to_string(max_bounds) << "\n";
	// <<<Menger Data>>>


	// <<<Menger2 Data>>>
	Menger *menger2 = new Menger();

	std::vector<glm::vec4> menger2_vertices;
	std::vector<glm::vec4> menger2_normals;
	std::vector<glm::uvec3> menger2_faces;

	glm::vec4 menger2_pos = glm::vec4(2.0f, 3.0f, -8.0f, 1.0f);

	menger2->set_nesting_level(1);
	menger2->generate_geometry(menger2_vertices, menger2_normals, menger2_faces, menger2_pos);
	menger2->set_clean();

	glm::vec4 min_bounds2 = glm::vec4(std::numeric_limits<float>::max());
	glm::vec4 max_bounds2 = glm::vec4(-std::numeric_limits<float>::max());
	for (int i = 0; i < menger2_vertices.size(); ++i) {
		min_bounds2 = glm::min(menger2_vertices[i], min_bounds2);
		max_bounds2 = glm::max(menger2_vertices[i], max_bounds2);
	}
	std::cout << "min_bounds = " << glm::to_string(min_bounds2) << "\n";
	std::cout << "max_bounds = " << glm::to_string(max_bounds2) << "\n";
	// <<<Menger2 Data>>>

	// <<<Menger3 Data>>>
	Menger *menger3 = new Menger();

	std::vector<glm::vec4> menger3_vertices;
	std::vector<glm::vec4> menger3_normals;
	std::vector<glm::uvec3> menger3_faces;

	glm::vec4 menger3_pos = glm::vec4(-5.0f, 1.5f, -4.0f, 1.0f);

	menger3->set_nesting_level(1);
	menger3->generate_geometry(menger3_vertices, menger3_normals, menger3_faces, menger3_pos);
	menger3->set_clean();

	glm::vec4 min_bounds3 = glm::vec4(std::numeric_limits<float>::max());
	glm::vec4 max_bounds3 = glm::vec4(-std::numeric_limits<float>::max());
	for (int i = 0; i < menger3_vertices.size(); ++i) {
		min_bounds3 = glm::min(menger3_vertices[i], min_bounds3);
		max_bounds3 = glm::max(menger3_vertices[i], max_bounds3);
	}
	std::cout << "min_bounds = " << glm::to_string(min_bounds3) << "\n";
	std::cout << "max_bounds = " << glm::to_string(max_bounds3) << "\n";
	// <<<Menger3 Data>>>


    // <<<Floor Data>>>
    std::vector<glm::vec4> floor_vertices;
    std::vector<glm::vec4> floor_normals;
    std::vector<glm::uvec3> floor_faces;

    g_floor->create_floor(floor_vertices, floor_normals, floor_faces);
    // <<<Floor Data>>>

    // <<<Floor Renderpass>>>
	RenderDataInput floor_pass_input;
	floor_pass_input.assign(0, "vertex_position", floor_vertices.data(), floor_vertices.size(), 4, GL_FLOAT);
	floor_pass_input.assign(1, "normal", floor_normals.data(), floor_normals.size(), 4, GL_FLOAT);
	floor_pass_input.assign_index(floor_faces.data(), floor_faces.size(), 3);
	RenderPass floor_pass(-1,
			floor_pass_input,
			{ vertex_shader, NULL, floor_fragment_shader},
			{ floor_model, std_view, std_proj, std_light },
			{ "fragment_color" }
			);
    // <<<Floor Renderpass>>>

    // <<<Cat>>>
    Object* cat = new Object();
    cat->load("/src/assets/animals/cat/cat.obj");

    glm::mat4 cat_model_matrix = glm::mat4(1.0f);

    //cat_model_matrix = cat->translate(cat_model_matrix, glm::vec3(0.0f, 1.0f, 0.0f));
    //cat_model_matrix = cat->rotate(cat_model_matrix, -1.5708f, glm::vec3(1.0f, 0.0f, 0.0f));
    //cat_model_matrix = cat->scale(cat_model_matrix, glm::vec3(0.001f, 0.001f, 0.001f));

    auto cat_model_data = [&cat_model_matrix]() -> const void* {
			return &cat_model_matrix[0][0];
		};

    ShaderUniform cat_model = {"model", matrix_binder, cat_model_data};

    cat->shaders(object_vertex_shader, NULL, object_fragment_shader);
    cat->uniforms(cat_model, std_view, std_proj, std_light, std_view_position);
    cat->lights(directionalLights, pointLights, spotLights);

    cat->setup(0);
     //for (unsigned int i = 1; i < cat->meshes.size(); i++) {
        //cat->setup(i);
        //cat->render(i);
    //}
    // <<<Cat>>>

    // <<<Dog>>>
    Object* dog = new Object();
    dog->load("/src/assets/animals/dog/dog.obj");

    glm::mat4 dog_model_matrix = glm::mat4(1.0f);

    dog_model_matrix = dog->translate(dog_model_matrix, glm::vec3(1.5f, 0.0f, 0.0f));
    dog_model_matrix = dog->rotate(dog_model_matrix, -1.5708f, glm::vec3(1.0f, 0.0f, 0.0f));
    dog_model_matrix = dog->scale(dog_model_matrix, glm::vec3(0.01f, 0.01f, 0.01f));

    auto dog_model_data = [&dog_model_matrix]() -> const void* {
			return &dog_model_matrix[0][0];
		};

    ShaderUniform dog_model = {"model", matrix_binder, dog_model_data};

    dog->shaders(object_vertex_shader, NULL, object_fragment_shader);
    dog->uniforms(dog_model, std_view, std_proj, std_light, std_view_position);
    dog->lights(directionalLights, pointLights, spotLights);

    dog->setup(0);

    //for (unsigned int i = 1; i < dog->meshes.size(); i++) {
        //dog->setup(i);
        //dog->render(i);
    //}
    // <<<Dog>>>

		// <<<Dog2>>>
    Object* dog2 = new Object();
    dog2->load("/src/assets/animals/dog/dog.obj");

    glm::mat4 dog2_model_matrix = glm::mat4(1.0f);

    dog2_model_matrix = dog2->translate(dog2_model_matrix, glm::vec3(-1.5f, 0.0f, 0.0f));
    dog2_model_matrix = dog->rotate(dog2_model_matrix, -1.5708f, glm::vec3(1.0f, 0.0f, 0.0f));
    dog2_model_matrix = dog->scale(dog2_model_matrix, glm::vec3(0.01f, 0.01f, 0.01f));

    auto dog2_model_data = [&dog2_model_matrix]() -> const void* {
			return &dog2_model_matrix[0][0];
		};

    ShaderUniform dog2_model = {"model", matrix_binder, dog2_model_data};

    dog2->shaders(object_vertex_shader, NULL, object_fragment_shader);
    dog2->uniforms(dog2_model, std_view, std_proj, std_light, std_view_position);
    dog2->lights(directionalLights, pointLights, spotLights);

    dog2->setup(0);

    //for (unsigned int i = 1; i < dog->meshes.size(); i++) {
        //dog->setup(i);
        //dog->render(i);
    //}
    // <<<Dog2>>>

	float theta = 0.0f;

	// screen quad VAO, for displaying game as a texture
	float quadVertices[] = { // vertex attributes for a quad that fills the entire screen in Normalized Device Coordinates.
			// positions   // texCoords
			-1.0f,  1.0f,  0.0f, 1.0f,
			-1.0f, -1.0f,  0.0f, 0.0f,
			 1.0f, -1.0f,  1.0f, 0.0f,

			-1.0f,  1.0f,  0.0f, 1.0f,
			 1.0f, -1.0f,  1.0f, 0.0f,
			 1.0f,  1.0f,  1.0f, 1.0f
	};

	unsigned int quadVAO, quadVBO;
	glGenVertexArrays(1, &quadVAO);
	glGenBuffers(1, &quadVBO);
	glBindVertexArray(quadVAO);
	glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

	// Create basic quad program
	// Setup vertex shader for the quad.
	GLuint screen_vertex_shader_id = 0;
	const char* screen_vertex_source_pointer = screen_vertex_shader;
	CHECK_GL_ERROR(screen_vertex_shader_id = glCreateShader(GL_VERTEX_SHADER));
	CHECK_GL_ERROR(glShaderSource(screen_vertex_shader_id, 1, &screen_vertex_source_pointer, nullptr));
	glCompileShader(screen_vertex_shader_id);
	CHECK_GL_SHADER_ERROR(screen_vertex_shader_id);

	// Setup default fragment shader for the quad ====================
	GLuint screen_fragment_shader_id = 0;
	const char* screen_fragment_source_pointer = screen_fragment_shader;
	CHECK_GL_ERROR(screen_fragment_shader_id = glCreateShader(GL_FRAGMENT_SHADER));
	CHECK_GL_ERROR(glShaderSource(screen_fragment_shader_id, 1,
				&screen_fragment_source_pointer, nullptr));
	glCompileShader(screen_fragment_shader_id);
	CHECK_GL_SHADER_ERROR(screen_fragment_shader_id);

	// Setup the program
	GLuint screen_default_program_id = 0;
  CHECK_GL_ERROR(screen_default_program_id = glCreateProgram());
	CHECK_GL_ERROR(glAttachShader(screen_default_program_id, screen_vertex_shader_id));
	CHECK_GL_ERROR(glAttachShader(screen_default_program_id, screen_fragment_shader_id));

	CHECK_GL_ERROR(glBindBuffer(GL_ARRAY_BUFFER, g_buffer_objects[kScreenVao][kVertexBuffer]));

	// Bind attributes.
	CHECK_GL_ERROR(glBindAttribLocation(screen_default_program_id, 0, "vertex_position"));
	CHECK_GL_ERROR(glBindAttribLocation(screen_default_program_id, 1, "aTexCoords"));

	CHECK_GL_ERROR(glBindFragDataLocation(screen_default_program_id, 0, "fragment_color"));
	glLinkProgram(screen_default_program_id);
	CHECK_GL_PROGRAM_ERROR(screen_default_program_id);

	CHECK_GL_ERROR(glUseProgram(screen_default_program_id));
	// Get the uniform locations.
	GLint screen_projection_matrix_location = 0;
	CHECK_GL_ERROR(screen_projection_matrix_location =
			glGetUniformLocation(screen_default_program_id, "screenTexture"));
	glUniform1i(screen_projection_matrix_location, 0);

	GLint screen_effect_projection_matrix_location = 0;
	CHECK_GL_ERROR(screen_effect_projection_matrix_location =
			glGetUniformLocation(screen_default_program_id, "lensEffect"));
	glUniform1i(screen_effect_projection_matrix_location, 1);

	GLint screen_effect2_projection_matrix_location = 0;
	CHECK_GL_ERROR(screen_effect2_projection_matrix_location =
			glGetUniformLocation(screen_default_program_id, "lensEffect2"));
	glUniform1i(screen_effect2_projection_matrix_location, 2);
	// ===========================================================

	// Setup the program
	GLuint screen_single_program_id = 0;
  CHECK_GL_ERROR(screen_single_program_id = glCreateProgram());
	CHECK_GL_ERROR(glAttachShader(screen_single_program_id, screen_vertex_shader_id));
	CHECK_GL_ERROR(glAttachShader(screen_single_program_id, screen_fragment_shader_id));

	CHECK_GL_ERROR(glBindBuffer(GL_ARRAY_BUFFER, g_buffer_objects[kScreenVao][kVertexBuffer]));

	// Bind attributes.
	CHECK_GL_ERROR(glBindAttribLocation(screen_single_program_id, 0, "vertex_position"));
	CHECK_GL_ERROR(glBindAttribLocation(screen_single_program_id, 1, "aTexCoords"));

	CHECK_GL_ERROR(glBindFragDataLocation(screen_single_program_id, 0, "fragment_color"));
	glLinkProgram(screen_single_program_id);
	CHECK_GL_PROGRAM_ERROR(screen_single_program_id);

	CHECK_GL_ERROR(glUseProgram(screen_single_program_id));
	// Get the uniform locations.
	GLint screen_texture_matrix_location = 0;
	CHECK_GL_ERROR(screen_texture_matrix_location =
			glGetUniformLocation(screen_single_program_id, "screenTexture"));
	glUniform1i(screen_texture_matrix_location, 0);
	// ===========================================================

	// configure alternate geometry_framebuffer
	unsigned int id_tracking_framebuffer;
	glGenFramebuffers(1, &id_tracking_framebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, id_tracking_framebuffer);

	// create a color attachment texture
	unsigned int id_tracking_textureColorBuffer;
	glGenTextures(1, &id_tracking_textureColorBuffer);
	glBindTexture(GL_TEXTURE_2D, id_tracking_textureColorBuffer);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, window_width, window_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, id_tracking_textureColorBuffer, 0);

	unsigned int id_tracking_rbo;
	glGenRenderbuffers(1, &id_tracking_rbo);
	glBindRenderbuffer(GL_RENDERBUFFER, id_tracking_rbo);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, window_width, window_height); // use a single renderbuffer object for both a depth AND stencil buffer.
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, id_tracking_rbo); // now actually attach it

	// Check that framebuffer is set up correctly
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE){
		std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
	}

  // Setup hdr fragment shader for the quad ====================
	GLuint screen_hdr_shader_id = 0;
	const char* screen_hdr_source_pointer = screen_hdr_shader;
	CHECK_GL_ERROR(screen_hdr_shader_id = glCreateShader(GL_FRAGMENT_SHADER));
	CHECK_GL_ERROR(glShaderSource(screen_hdr_shader_id, 1,
				&screen_hdr_source_pointer, nullptr));
	glCompileShader(screen_hdr_shader_id);
	CHECK_GL_SHADER_ERROR(screen_hdr_shader_id);

	// Setup the program
	GLuint screen_hdr_program_id = 0;
	CHECK_GL_ERROR(screen_hdr_program_id = glCreateProgram());
	CHECK_GL_ERROR(glAttachShader(screen_hdr_program_id, screen_vertex_shader_id));
	CHECK_GL_ERROR(glAttachShader(screen_hdr_program_id, screen_hdr_shader_id));

	CHECK_GL_ERROR(glBindBuffer(GL_ARRAY_BUFFER, g_buffer_objects[kScreenVao][kVertexBuffer]));

	// Bind attributes.
	CHECK_GL_ERROR(glBindAttribLocation(screen_hdr_program_id, 0, "vertex_position"));
	CHECK_GL_ERROR(glBindAttribLocation(screen_hdr_program_id, 1, "aTexCoords"));

	CHECK_GL_ERROR(glBindFragDataLocation(screen_hdr_program_id, 0, "fragment_color"));
	glLinkProgram(screen_hdr_program_id);
	CHECK_GL_PROGRAM_ERROR(screen_hdr_program_id);

	// Get the uniform locations.
	GLint screen_hdr_projection_matrix_location = 0;
	CHECK_GL_ERROR(screen_hdr_projection_matrix_location =
	glGetUniformLocation(screen_hdr_program_id, "screenTexture"));

	glUseProgram(screen_hdr_program_id);
	glUniform1f(glGetUniformLocation(screen_hdr_program_id, "exposure"), exposure);
	// ===========================================================

	// configure hdr_framebuffer
	unsigned int hdr_framebuffer;
	glGenFramebuffers(1, &hdr_framebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, hdr_framebuffer);

	// create a color attachment texture
	unsigned int hdr_textureColorBuffer;
	glGenTextures(1, &hdr_textureColorBuffer);
	glBindTexture(GL_TEXTURE_2D, hdr_textureColorBuffer);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, window_width, window_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, hdr_textureColorBuffer, 0);

	unsigned int hdr_rbo;
	glGenRenderbuffers(1, &hdr_rbo);
	glBindRenderbuffer(GL_RENDERBUFFER, hdr_rbo);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, window_width, window_height); // use a single renderbuffer object for both a depth AND stencil buffer.
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, hdr_rbo); // now actually attach it

	// Check that framebuffer is set up correctly
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE){
		std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	// ===========================================================

	// Setup brightness fragment shader for the quad ====================
	GLuint screen_brightness_shader_id = 0;
	const char* screen_brightness_source_pointer = screen_brightness_shader;
	CHECK_GL_ERROR(screen_brightness_shader_id = glCreateShader(GL_FRAGMENT_SHADER));
	CHECK_GL_ERROR(glShaderSource(screen_brightness_shader_id, 1,
				&screen_brightness_source_pointer, nullptr));
	glCompileShader(screen_brightness_shader_id);
	CHECK_GL_SHADER_ERROR(screen_brightness_shader_id);

	// Setup the program
	GLuint screen_brightness_program_id = 0;
	CHECK_GL_ERROR(screen_brightness_program_id = glCreateProgram());
	CHECK_GL_ERROR(glAttachShader(screen_brightness_program_id, screen_vertex_shader_id));
	CHECK_GL_ERROR(glAttachShader(screen_brightness_program_id, screen_brightness_shader_id));

	CHECK_GL_ERROR(glBindBuffer(GL_ARRAY_BUFFER, g_buffer_objects[kScreenVao][kVertexBuffer]));

	// Bind attributes.
	CHECK_GL_ERROR(glBindAttribLocation(screen_brightness_program_id, 0, "vertex_position"));
	CHECK_GL_ERROR(glBindAttribLocation(screen_brightness_program_id, 1, "aTexCoords"));

	CHECK_GL_ERROR(glBindFragDataLocation(screen_brightness_program_id, 0, "fragment_color"));
	CHECK_GL_ERROR(glBindFragDataLocation(screen_brightness_program_id, 1, "bright_color"));
	glLinkProgram(screen_brightness_program_id);
	CHECK_GL_PROGRAM_ERROR(screen_brightness_program_id);

	// Get the uniform locations.
	GLint screen_brightness_projection_matrix_location = 0;
	CHECK_GL_ERROR(screen_brightness_projection_matrix_location =
	glGetUniformLocation(screen_brightness_program_id, "screenTexture"));
	// ===========================================================

	// ===========================================================

	// configure brightness_framebuffer
	unsigned int brightness_framebuffer;
	glGenFramebuffers(1, &brightness_framebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, brightness_framebuffer);

	// FIXME: create 2 floating point color buffers (1 for normal rendering, other for brightness treshold values)
  unsigned int brightness_colorBuffers[2];
  glGenTextures(2, brightness_colorBuffers);
  for (unsigned int i = 0; i < 2; i++)
  {
      glBindTexture(GL_TEXTURE_2D, brightness_colorBuffers[i]);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, window_width, window_height, 0, GL_RGB, GL_FLOAT, NULL);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);  // we clamp to the edge as the blur filter would otherwise sample repeated texture values!
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
      // attach texture to framebuffer
      glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, brightness_colorBuffers[i], 0);
  }

	unsigned int brightness_rbo;
	glGenRenderbuffers(1, &brightness_rbo);
	glBindRenderbuffer(GL_RENDERBUFFER, brightness_rbo);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, window_width, window_height); // use a single renderbuffer object for both a depth AND stencil buffer.
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, brightness_rbo); // now actually attach it

	// FIXME: tell OpenGL which color attachments we'll use (of this framebuffer) for rendering
  unsigned int attachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
  glDrawBuffers(2, attachments);

	// Check that framebuffer is set up correctly
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE){
		std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	// ===========================================================

	// Setup downsample shader for the quad ====================
	GLuint screen_downsample_shader_id = 0;
	const char* screen_downsample_source_pointer = screen_downsample_shader;
	CHECK_GL_ERROR(screen_downsample_shader_id = glCreateShader(GL_FRAGMENT_SHADER));
	CHECK_GL_ERROR(glShaderSource(screen_downsample_shader_id, 1,
				&screen_downsample_source_pointer, nullptr));
	glCompileShader(screen_downsample_shader_id);
	CHECK_GL_SHADER_ERROR(screen_downsample_shader_id);

	// Setup the program
	GLuint screen_downsample_program_id = 0;
  CHECK_GL_ERROR(screen_downsample_program_id = glCreateProgram());
	CHECK_GL_ERROR(glAttachShader(screen_downsample_program_id, screen_vertex_shader_id));
	CHECK_GL_ERROR(glAttachShader(screen_downsample_program_id, screen_downsample_shader_id));

	CHECK_GL_ERROR(glBindBuffer(GL_ARRAY_BUFFER, g_buffer_objects[kScreenVao][kVertexBuffer]));

	// Bind attributes.
	CHECK_GL_ERROR(glBindAttribLocation(screen_downsample_program_id, 0, "vertex_position"));
	CHECK_GL_ERROR(glBindAttribLocation(screen_downsample_program_id, 1, "aTexCoords"));

	CHECK_GL_ERROR(glBindFragDataLocation(screen_downsample_program_id, 0, "fragment_color"));
	glLinkProgram(screen_downsample_program_id);
	CHECK_GL_PROGRAM_ERROR(screen_downsample_program_id);

	// Get the uniform locations.
	GLint screen_downsample_projection_matrix_location = 0;
	CHECK_GL_ERROR(screen_downsample_projection_matrix_location =
	glGetUniformLocation(screen_downsample_program_id, "screenTexture"));

  int uScaleLocation = glGetUniformLocation(screen_downsample_program_id, "uScale");
  int uBiasLocation = glGetUniformLocation(screen_downsample_program_id, "uBias");

  glUseProgram(screen_downsample_program_id);

  // Adjust uScale and uBias
  glUniform4f(uScaleLocation, 0.1f, 0.1f, 0.1f, 1.0f);
  glUniform4f(uBiasLocation, -0.7f, -0.7f, -0.7f, 1.0f);
	// ===========================================================

	// configure downsample_framebuffer
	unsigned int downsample_framebuffer;
	glGenFramebuffers(1, &downsample_framebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, downsample_framebuffer);

	// create a color attachment texture
	unsigned int downsample_textureColorBuffer;
	glGenTextures(1, &downsample_textureColorBuffer);
	glBindTexture(GL_TEXTURE_2D, downsample_textureColorBuffer);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, window_width, window_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, downsample_textureColorBuffer, 0);

	unsigned int downsample_rbo;
	glGenRenderbuffers(1, &downsample_rbo);
	glBindRenderbuffer(GL_RENDERBUFFER, downsample_rbo);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, window_width, window_height); // use a single renderbuffer object for both a depth AND stencil buffer.
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, downsample_rbo); // now actually attach it

	// Check that framebuffer is set up correctly
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE){
		std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	// ===========================================================

	// Setup lensflare shader for the quad ====================
	GLuint screen_lensflare_shader_id = 0;
	const char* screen_lensflare_source_pointer = screen_lensflare_shader;
	CHECK_GL_ERROR(screen_lensflare_shader_id = glCreateShader(GL_FRAGMENT_SHADER));
	CHECK_GL_ERROR(glShaderSource(screen_lensflare_shader_id, 1,
				&screen_lensflare_source_pointer, nullptr));
	glCompileShader(screen_lensflare_shader_id);
	CHECK_GL_SHADER_ERROR(screen_lensflare_shader_id);

	// Setup the program
	GLuint screen_lensflare_program_id = 0;
	CHECK_GL_ERROR(screen_lensflare_program_id = glCreateProgram());
	CHECK_GL_ERROR(glAttachShader(screen_lensflare_program_id, screen_vertex_shader_id));
	CHECK_GL_ERROR(glAttachShader(screen_lensflare_program_id, screen_lensflare_shader_id));

	CHECK_GL_ERROR(glBindBuffer(GL_ARRAY_BUFFER, g_buffer_objects[kScreenVao][kVertexBuffer]));

	// Bind attributes.
	CHECK_GL_ERROR(glBindAttribLocation(screen_lensflare_program_id, 0, "vertex_position"));
	CHECK_GL_ERROR(glBindAttribLocation(screen_lensflare_program_id, 1, "aTexCoords"));

	CHECK_GL_ERROR(glBindFragDataLocation(screen_lensflare_program_id, 0, "fragment_color"));
	glLinkProgram(screen_lensflare_program_id);
	CHECK_GL_PROGRAM_ERROR(screen_lensflare_program_id);

	// Get the uniform locations.
	GLint screen_lensflare_projection_matrix_location = 0;
	CHECK_GL_ERROR(screen_lensflare_projection_matrix_location =
	glGetUniformLocation(screen_lensflare_program_id, "screenTexture"));
	glUniform1i(screen_lensflare_projection_matrix_location, 0);

	std::string lenscolor_image_path = "/assets/lenscolor.png";
	std::string full_path_to_image = cwd() + lenscolor_image_path;
	int width, height, nrChannels;
	unsigned char *image_data = loadImg(full_path_to_image, width, height, nrChannels);
	unsigned int lens_color_texture;

	glGenTextures(1, &lens_color_texture);
	glBindTexture(GL_TEXTURE_1D, lens_color_texture);
	glTexImage1D(GL_TEXTURE_1D, 0, GL_RGB, width, 0, GL_RGB, GL_UNSIGNED_BYTE, image_data);
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	CHECK_GL_ERROR(glUseProgram(screen_lensflare_program_id));
	GLint lens_color_texture_location = 0;
	CHECK_GL_ERROR(lens_color_texture_location =
	glGetUniformLocation(screen_lensflare_program_id, "uLensColor"));
	glUniform1i(lens_color_texture_location, 1);

  float ghostDispersal = 0.5f;
  glUniform1f(glGetUniformLocation(screen_lensflare_program_id, "uGhostDispersal"), ghostDispersal);

	float numberOfGhosts = 3.0f;
	glUniform1f(glGetUniformLocation(screen_lensflare_program_id, "uGhosts"), numberOfGhosts);

	float haloWidth = 0.5f;
	glUniform1f(glGetUniformLocation(screen_lensflare_program_id, "uHaloWidth"), haloWidth);

	float uDistortion = 0.5f;
	glUniform1f(glGetUniformLocation(screen_lensflare_program_id, "uDistortion"), haloWidth);
	// ===========================================================

	// configure lensflare_framebuffer
	unsigned int lensflare_framebuffer;
	glGenFramebuffers(1, &lensflare_framebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, lensflare_framebuffer);

	// create a color attachment texture
	unsigned int lensflare_textureColorBuffer;
	glGenTextures(1, &lensflare_textureColorBuffer);
	glBindTexture(GL_TEXTURE_2D, lensflare_textureColorBuffer);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, window_width, window_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, lensflare_textureColorBuffer, 0);

	unsigned int lensflare_rbo;
	glGenRenderbuffers(1, &lensflare_rbo);
	glBindRenderbuffer(GL_RENDERBUFFER, lensflare_rbo);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, window_width, window_height); // use a single renderbuffer object for both a depth AND stencil buffer.
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, lensflare_rbo); // now actually attach it

	// Check that framebuffer is set up correctly
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE){
		std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	// ===========================================================

	// Setup blur shader for the quad ====================
	GLuint screen_blur_shader_id = 0;
	const char* screen_blur_source_pointer = screen_blur_shader;
	CHECK_GL_ERROR(screen_blur_shader_id = glCreateShader(GL_FRAGMENT_SHADER));
	CHECK_GL_ERROR(glShaderSource(screen_blur_shader_id, 1,
				&screen_blur_source_pointer, nullptr));
	glCompileShader(screen_blur_shader_id);
	CHECK_GL_SHADER_ERROR(screen_blur_shader_id);

	// Setup the program
	GLuint screen_blur_program_id = 0;
	CHECK_GL_ERROR(screen_blur_program_id = glCreateProgram());
	CHECK_GL_ERROR(glAttachShader(screen_blur_program_id, screen_vertex_shader_id));
	CHECK_GL_ERROR(glAttachShader(screen_blur_program_id, screen_blur_shader_id));

	CHECK_GL_ERROR(glBindBuffer(GL_ARRAY_BUFFER, g_buffer_objects[kScreenVao][kVertexBuffer]));

	// Bind attributes.
	CHECK_GL_ERROR(glBindAttribLocation(screen_blur_program_id, 0, "vertex_position"));
	CHECK_GL_ERROR(glBindAttribLocation(screen_blur_program_id, 1, "aTexCoords"));

	CHECK_GL_ERROR(glBindFragDataLocation(screen_blur_program_id, 0, "fragment_color"));
	glLinkProgram(screen_blur_program_id);
	CHECK_GL_PROGRAM_ERROR(screen_blur_program_id);

	// Get the uniform locations.
	GLint screen_blur_projection_matrix_location = 0;
	CHECK_GL_ERROR(screen_blur_projection_matrix_location =
	glGetUniformLocation(screen_blur_program_id, "screenTexture"));
	// ===========================================================

	// configure blur_framebuffer
	unsigned int blur_framebuffer;
	glGenFramebuffers(1, &blur_framebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, blur_framebuffer);

	// create a color attachment texture
	unsigned int blur_textureColorBuffer;
	glGenTextures(1, &blur_textureColorBuffer);
	glBindTexture(GL_TEXTURE_2D, blur_textureColorBuffer);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, window_width, window_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, blur_textureColorBuffer, 0);

	unsigned int blur_rbo;
	glGenRenderbuffers(1, &blur_rbo);
	glBindRenderbuffer(GL_RENDERBUFFER, blur_rbo);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, window_width, window_height); // use a single renderbuffer object for both a depth AND stencil buffer.
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, blur_rbo); // now actually attach it

	// Check that framebuffer is set up correctly
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE){
		std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	// ===========================================================

	// Setup blur2 shader for the quad ====================
	GLuint screen_blur2_shader_id = 0;
	const char* screen_blur2_source_pointer = screen_blur2_shader;
	CHECK_GL_ERROR(screen_blur2_shader_id = glCreateShader(GL_FRAGMENT_SHADER));
	CHECK_GL_ERROR(glShaderSource(screen_blur2_shader_id, 1,
				&screen_blur2_source_pointer, nullptr));
	glCompileShader(screen_blur2_shader_id);
	CHECK_GL_SHADER_ERROR(screen_blur2_shader_id);

	// Setup the program
	GLuint screen_blur2_program_id = 0;
	CHECK_GL_ERROR(screen_blur2_program_id = glCreateProgram());
	CHECK_GL_ERROR(glAttachShader(screen_blur2_program_id, screen_vertex_shader_id));
	CHECK_GL_ERROR(glAttachShader(screen_blur2_program_id, screen_blur2_shader_id));

	CHECK_GL_ERROR(glBindBuffer(GL_ARRAY_BUFFER, g_buffer_objects[kScreenVao][kVertexBuffer]));

	// Bind attributes.
	CHECK_GL_ERROR(glBindAttribLocation(screen_blur2_program_id, 0, "vertex_position"));
	CHECK_GL_ERROR(glBindAttribLocation(screen_blur2_program_id, 1, "aTexCoords"));

	CHECK_GL_ERROR(glBindFragDataLocation(screen_blur2_program_id, 0, "fragment_color"));
	glLinkProgram(screen_blur2_program_id);
	CHECK_GL_PROGRAM_ERROR(screen_blur2_program_id);

	// Get the uniform locations.
	GLint screen_blur2_projection_matrix_location = 0;
	CHECK_GL_ERROR(screen_blur2_projection_matrix_location =
	glGetUniformLocation(screen_blur2_program_id, "screenTexture"));

	CHECK_GL_ERROR(glUseProgram(screen_blur2_program_id));
	// ===========================================================

	// configure blur2_framebuffer, FIXME A DOUBLE BUFFER, WHAT THE HELL
	unsigned int pingpongFBO[2];
	unsigned int pingpongBuffer[2];
	glGenFramebuffers(2, pingpongFBO);
	glGenTextures(2, pingpongBuffer);
	for (unsigned int i = 0; i < 2; i++)
	{
	    glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[i]);
	    glBindTexture(GL_TEXTURE_2D, pingpongBuffer[i]);
	    glTexImage2D(
	        GL_TEXTURE_2D, 0, GL_RGB16F, window_width, window_height, 0, GL_RGB, GL_FLOAT, NULL
	    );
	    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	    glFramebufferTexture2D(
	        GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pingpongBuffer[i], 0
	    );
	}
	// // ===========================================================

	// configure alternate geometry_framebuffer
	unsigned int geometry_framebuffer;
	glGenFramebuffers(1, &geometry_framebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, geometry_framebuffer);

	// create a color attachment texture
	unsigned int geometry_textureColorBuffer;
	glGenTextures(1, &geometry_textureColorBuffer);
	glBindTexture(GL_TEXTURE_2D, geometry_textureColorBuffer);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, window_width, window_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, geometry_textureColorBuffer, 0);

	unsigned int geometry_rbo;
	glGenRenderbuffers(1, &geometry_rbo);
	glBindRenderbuffer(GL_RENDERBUFFER, geometry_rbo);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, window_width, window_height); // use a single renderbuffer object for both a depth AND stencil buffer.
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, geometry_rbo); // now actually attach it

	// Check that framebuffer is set up correctly
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE){
		std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);


	// Setup GUI
	BasicGUI* gui = new BasicGUI(window);

	clock_t last_frame_time = clock();
	while (!glfwWindowShouldClose(window)) {
		if(showGeometry){
			// render
			// ------
			// bind to geometry_framebuffer and draw scene as we normally would to color texture
			glBindFramebuffer(GL_FRAMEBUFFER, geometry_framebuffer);
			glEnable(GL_DEPTH_TEST); // enable depth testing (is disabled for rendering screen-space quad)

			// make sure we clear the geometry_framebuffer's content
			glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


			if (g_menger && g_menger->is_dirty()) {
			  	g_menger->generate_geometry(menger_vertices, menger_normals, menger_faces, menger_pos);
				g_menger->set_clean();
			}

			// <<<Render Menger>>>
			RenderDataInput menger_pass_input;
			menger_pass_input.assign(0, "vertex_position", menger_vertices.data(), menger_vertices.size(), 4, GL_FLOAT);
			menger_pass_input.assign(1, "normal", menger_normals.data(), menger_normals.size(), 4, GL_FLOAT);
			menger_pass_input.assign_index(menger_faces.data(), menger_faces.size(), 3);
			RenderPass menger_pass(-1,
					menger_pass_input,
					{ vertex_shader, NULL, fragment_shader},
					{ menger_model, std_view, std_proj, std_light, std_view_position },
					{ "fragment_color" }
					);

			menger_pass.loadLights(directionalLights, pointLights, spotLights);
			menger_pass.loadLightColor(glm::vec4(3.0f, 3.0f, 3.0f, 1.0f));

			menger_pass.setup();
			CHECK_GL_ERROR(glDrawElements(GL_TRIANGLES, menger_faces.size() * 3, GL_UNSIGNED_INT, 0));
			// <<<Render Menger>>>

			// <<<Render Menger2>>>
			RenderDataInput menger2_pass_input;
			menger2_pass_input.assign(0, "vertex_position", menger2_vertices.data(), menger2_vertices.size(), 4, GL_FLOAT);
			menger2_pass_input.assign(1, "normal", menger2_normals.data(), menger2_normals.size(), 4, GL_FLOAT);
			menger2_pass_input.assign_index(menger2_faces.data(), menger2_faces.size(), 3);
			RenderPass menger2_pass(-1,
				menger2_pass_input,
				{ vertex_shader, NULL, fragment_shader },
				{ menger_model, std_view, std_proj, std_light, std_view_position },
				{ "fragment_color" }
			);

			menger2_pass.loadLights(directionalLights, pointLights, spotLights);
			menger2_pass.loadLightColor(glm::vec4(3.0f, 3.0f, 3.0f, 1.0f));

			menger2_pass.setup();
			CHECK_GL_ERROR(glDrawElements(GL_TRIANGLES, menger2_faces.size() * 3, GL_UNSIGNED_INT, 0));
			// <<<Render Menger2>>>

			// <<<Render Menger3>>>
			RenderDataInput menger3_pass_input;
			menger3_pass_input.assign(0, "vertex_position", menger3_vertices.data(), menger3_vertices.size(), 4, GL_FLOAT);
			menger3_pass_input.assign(1, "normal", menger3_normals.data(), menger3_normals.size(), 4, GL_FLOAT);
			menger3_pass_input.assign_index(menger3_faces.data(), menger3_faces.size(), 3);
			RenderPass menger3_pass(-1,
				menger3_pass_input,
				{ vertex_shader, NULL, fragment_shader },
				{ menger_model, std_view, std_proj, std_light, std_view_position },
				{ "fragment_color" }
			);

			menger3_pass.loadLights(directionalLights, pointLights, spotLights);
			menger3_pass.loadLightColor(glm::vec4(4.0f, 1.5f, 1.5f, 1.0f));

			menger3_pass.setup();
			CHECK_GL_ERROR(glDrawElements(GL_TRIANGLES, menger3_faces.size() * 3, GL_UNSIGNED_INT, 0));
			// <<<Render Menger3>>>

			// <<<Render Floor>>>
			floor_pass.setup();
			CHECK_GL_ERROR(glDrawElements(GL_TRIANGLES, floor_faces.size() * 3, GL_UNSIGNED_INT, 0));
			// <<<Render Floor>>>

	    if (showMeshes){

	      cat->render(0);
	      dog->render(0);
				dog2->render(0);
	    }

	    // End of geometry pass ====================================================

			// Use our post processing programs ========================================
			// downsampling pass for lensflair effect
			glBindFramebuffer(GL_FRAMEBUFFER, downsample_framebuffer);
			glDisable(GL_DEPTH_TEST); // disable depth test so screen-space quad isn't discarded due to depth test.
			// clear all relevant buffers
			glClearColor(1.0f, 1.0f, 1.0f, 1.0f); // set clear color to white (not really necessery actually, since we won't be able to see behind the quad anyways)
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			CHECK_GL_ERROR(glUseProgram(screen_downsample_program_id));
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, geometry_textureColorBuffer);	// use the color attachment texture as the texture of the quad plane

			glBindVertexArray(quadVAO);
	    glDrawArrays(GL_TRIANGLES, 0, 6);
			glBindFramebuffer(GL_FRAMEBUFFER, 0);


			// lensflair effect pass
	    glBindFramebuffer(GL_FRAMEBUFFER, lensflare_framebuffer);
	    glDisable(GL_DEPTH_TEST); // disable depth test so screen-space quad isn't discarded due to depth test.
	    // clear all relevant buffers
	    glClearColor(1.0f, 1.0f, 1.0f, 1.0f); // set clear color to white (not really necessery actually, since we won't be able to see behind the quad anyways)
	    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			CHECK_GL_ERROR(glUseProgram(screen_lensflare_program_id));
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, downsample_textureColorBuffer);	// use the color attachment texture as the texture of the quad plane
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_1D, lens_color_texture);

	    glBindVertexArray(quadVAO);
	    glDrawArrays(GL_TRIANGLES, 0, 6);


			// post lensflair blur pass
			glBindFramebuffer(GL_FRAMEBUFFER, blur_framebuffer);
			glDisable(GL_DEPTH_TEST); // disable depth test so screen-space quad isn't discarded due to depth test.
			// clear all relevant buffers
			glClearColor(1.0f, 1.0f, 1.0f, 1.0f); // set clear color to white (not really necessery actually, since we won't be able to see behind the quad anyways)
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			CHECK_GL_ERROR(glUseProgram(screen_blur_program_id));
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, lensflare_textureColorBuffer);	// use the color attachment texture as the texture of the quad plane

			glBindVertexArray(quadVAO);
			glDrawArrays(GL_TRIANGLES, 0, 6);


	    // HDR pass between geometry pass and final pass, (not dependent on lensflair passes)
	    glBindFramebuffer(GL_FRAMEBUFFER, hdr_framebuffer);
			glDisable(GL_DEPTH_TEST); // disable depth test so screen-space quad isn't discarded due to depth test.
			// clear all relevant buffers
			glClearColor(1.0f, 1.0f, 1.0f, 1.0f); // set clear color to white (not really necessery actually, since we won't be able to see behind the quad anyways)
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			CHECK_GL_ERROR(glUseProgram(screen_hdr_program_id));

			// Adjust exposure for controls
			glUniform1f(glGetUniformLocation(screen_hdr_program_id, "exposure"), exposure);

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, geometry_textureColorBuffer);	// use the color attachment texture as the texture of the quad plane

			glBindVertexArray(quadVAO);
			glDrawArrays(GL_TRIANGLES, 0, 6);

			// brightness pass which extracts brightness and color into two outputs
	    glBindFramebuffer(GL_FRAMEBUFFER, brightness_framebuffer);
			glDisable(GL_DEPTH_TEST); // disable depth test so screen-space quad isn't discarded due to depth test.
			// clear all relevant buffers
			glClearColor(1.0f, 1.0f, 1.0f, 1.0f); // set clear color to white (not really necessery actually, since we won't be able to see behind the quad anyways)
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			CHECK_GL_ERROR(glUseProgram(screen_brightness_program_id));

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, hdr_textureColorBuffer);	// use the color attachment texture as the texture of the quad plane

			glBindVertexArray(quadVAO);
			glDrawArrays(GL_TRIANGLES, 0, 6);

			// blur 2 multi pass
			// 2. blur bright fragments with two-pass Gaussian Blur
	    // --------------------------------------------------
	    bool horizontal = true, first_iteration = true;
	    unsigned int amount = 20;
	    CHECK_GL_ERROR(glUseProgram(screen_blur2_program_id));
	    for (unsigned int i = 0; i < amount; i++)
	    {
	        glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[horizontal]);
	        glUniform1i(glGetUniformLocation(screen_blur2_program_id, "horizontal"), horizontal);
					glActiveTexture(GL_TEXTURE0);
					glBindTexture(
				 		GL_TEXTURE_2D, first_iteration ? brightness_colorBuffers[1] : pingpongBuffer[!horizontal]
		 			);
					//glBindTexture(GL_TEXTURE_2D, brightness_colorBuffers[1]);	// use the color attachment texture as the texture of the quad plane
					glBindVertexArray(quadVAO);
					glDrawArrays(GL_TRIANGLES, 0, 6);
	        horizontal = !horizontal;
	        if (first_iteration){
						first_iteration = false;
					}
	    }

			// Final default pass to create final screen quad
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
	    glDisable(GL_DEPTH_TEST); // disable depth test so screen-space quad isn't discarded due to depth test.
	    // clear all relevant buffers
	    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			CHECK_GL_ERROR(glUseProgram(screen_default_program_id));
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, brightness_colorBuffers[0]);	// use the color attachment texture as the texture of the quad plane
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, pingpongBuffer[0]);	// use the color attachment texture as the texture of the quad plane
			glActiveTexture(GL_TEXTURE2);
	    if (lensEffects){
	      	glBindTexture(GL_TEXTURE_2D, blur_textureColorBuffer);	// use the color attachment texture as the texture of the quad plane
	    } else {
	        glBindTexture(GL_TEXTURE_2D, geometry_textureColorBuffer);	// use the color attachment texture as the texture of the quad plane
	    }

	    glBindVertexArray(quadVAO);
	    glDrawArrays(GL_TRIANGLES, 0, 6);
		}

		if (captureImage && !showGeometry){
      //  color id pass
    	glBindFramebuffer(GL_FRAMEBUFFER, id_tracking_framebuffer);
  		glEnable(GL_DEPTH_TEST); // enable depth testing (is disabled for rendering screen-space quad)

  		// make sure we clear the id_tracking_framebuffer's content
  		glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
  		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

			// every object with an id needs to be rendered here
			if (showMeshes){
	      cat->render_id(0);
	      dog->render_id(0);
				dog2->render_id(0);
			}

      glBindFramebuffer(GL_FRAMEBUFFER, 0);
	    glDisable(GL_DEPTH_TEST); // disable depth test so screen-space quad isn't discarded due to depth test.
	    // clear all relevant buffers
	    glClearColor(1.0f, 1.0f, 1.0f, 1.0f); // set clear color to white (not really necessery actually, since we won't be able to see behind the quad anyways)
	    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			// draw to the quad:
			CHECK_GL_ERROR(glUseProgram(screen_single_program_id));
			glBindTexture(GL_TEXTURE_2D, id_tracking_textureColorBuffer);	// use the color attachment texture as the texture of the quad plane

			glBindVertexArray(quadVAO);
			glDrawArrays(GL_TRIANGLES, 0, 6);

      // Wait until all the pending drawing commands are really done.
      // Ultra-mega-over slow !
      // There are usually a long time between glDrawElements() and
      // all the fragments completely rasterized.
      glFlush();
      glFinish();

      glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

      // Read the pixel at the center of the screen.
      // You can also use glfwGetMousePos().
      // Ultra-mega-over slow too, even for 1 pixel,
      // because the framebuffer is on the GPU.
      unsigned char data[4];
      glReadPixels(window_width/2, window_height/2,1,1, GL_RGBA, GL_UNSIGNED_BYTE, data);

      // retrieve the picked id
      int pickedID =
        	data[0] +
        	data[1] * 256 +
        	data[2] * 256*256;

			// FIXME: ObjectID is calculated by a multiple of 3 for some reason???
			// FIXME: Added /3 to hack a fix, may break math above 255 objects
      printf("%d\n", pickedID / 3);

      //captureImage = false;
			// end of color id pass ===========================================================
    }

		// Compute the projection matrix.
		aspect = static_cast<float>(window_width) / window_height;
		projection_matrix =
			glm::perspective(glm::radians(45.0f), aspect, 0.0001f, 1000.0f);

		view_matrix = g_camera->get_view_matrix();

		// Render GUI
		if (showGui){
			gui->render();
		}

		// Poll and swap.
		glfwPollEvents();
		glfwSwapBuffers(window);
	}
	glfwDestroyWindow(window);
	glfwTerminate();
	exit(EXIT_SUCCESS);
}
