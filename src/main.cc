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

// Include AssImp
#include <assimp/Importer.hpp>      // C++ importer interface
#include <assimp/scene.h>           // Output data structure
#include <assimp/postprocess.h>     // Post processing flags

#include "debuggl.h"

// Classes
#include "floor.h"
#include "menger.h"
#include "camera.h"
#include "controller.h"
#include "render_pass.h"
#include "lights.h"

// Image loading library
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

struct MatrixPointers {
	const float *projection, *model, *view;
};

glm::mat4 projection_matrix;
glm::mat4 view_matrix;
glm::mat4 model_matrix;

float aspect = 0.0f;

int window_width = 2000;
int window_height = 2000;

// Used to brighten hdr exposure shader as described in this tutorial:
// https://learnopengl.com/Advanced-Lighting/HDR
float exposure = 0.8f;

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
#include "shaders/screen_default.frag"
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

const char* screen_hdr_shader =
#include "shaders/screen_hdr.frag"
;

void
ErrorCallback(int error, const char* description)
{
	std::cerr << "GLFW Error: " << description << "\n";
}

int main(int argc, char* argv[])
{
	std::string window_title = "Menger";
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
	g_controller = new Controller(window, g_camera, g_menger, &exposure);

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

	std::vector<PointLight> pointLights;
	PointLight pointLight = PointLight(glm::vec3(5.0f, 5.0f, 5.0f));
	//pointLights.push_back(pointLight);
	pointLight = PointLight(glm::vec3(-5.0f, 5.0f, 5.0f));
	//pointLights.push_back(pointLight);

	std::vector<SpotLight> spotLights;
	SpotLight spotLight = SpotLight(glm::vec3(0.0f, 5.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f));
	//spotLights.push_back(spotLight);
	// <<<Lights>>>

	// <<<Renderpass Setup>>>
 	projection_matrix = glm::perspective(glm::radians(45.0f), aspect, 0.0001f, 1000.0f);
	view_matrix = g_camera->get_view_matrix();
	model_matrix = glm::mat4(1.0f);
	glm::vec4 light_position = glm::vec4(5.0f, 5.0f, 5.0f, 1.0f);

    //glm::vec4 light_position = glm::vec4(0.0f, 100.0f, 0.0f, 1.0f);
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
	// auto bone_matrix_binder = [&mesh](int loc, const void* data) {
	// 	auto nelem = mesh.getNumberOfBones();
	// 	glUniformMatrix4fv(loc, nelem, GL_FALSE, (const GLfloat*)data);
	// };
	auto vector_binder = [](int loc, const void* data) {
		glUniform4fv(loc, 1, (const GLfloat*)data);
	};
	// auto vector3_binder = [](int loc, const void* data) {
	// 	glUniform3fv(loc, 1, (const GLfloat*)data);
	// };
	// auto float_binder = [](int loc, const void* data) {
	// 	glUniform1fv(loc, 1, (const GLfloat*)data);
	// };

 //    auto std_model_data = [&mats]() -> const void* {
	// 	return mats.model;
	// }; // This returns point to model matrix

	glm::mat4 menger_model_matrix = glm::mat4(1.0f);
	auto menger_model_data = [&menger_model_matrix]() -> const void* {
		return &menger_model_matrix[0][0];
	}; // This return model matrix for the menger.

	glm::mat4 floor_model_matrix = glm::mat4(1.0f);
	auto floor_model_data = [&floor_model_matrix]() -> const void* {
		return &floor_model_matrix[0][0];
	}; // This return model matrix for the floor.


	auto std_view_data = [&mats]() -> const void* {
		return mats.view;
	};
	// auto std_camera_data  = [&gui]() -> const void* {
	// 	return &gui.getCamera()[0];
	// };
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
	// auto alpha_data  = [&gui]() -> const void* {
	// 	static const float transparet = 0.5; // Alpha constant goes here
	// 	static const float non_transparet = 1.0;
	// 	if (gui.isTransparent())
	// 		return &transparet;
	// 	else
	// 		return &non_transparet;
	// };

    //ShaderUniform std_model = { "model", matrix_binder, std_model_data };
  ShaderUniform menger_model = { "model", matrix_binder, menger_model_data};
	ShaderUniform floor_model = { "model", matrix_binder, floor_model_data};
	ShaderUniform std_view = { "view", matrix_binder, std_view_data };
	//ShaderUniform std_camera = { "camera_position", vector3_binder, std_camera_data };
	ShaderUniform std_proj = { "projection", matrix_binder, std_proj_data };
	ShaderUniform std_light = { "light_position", vector_binder, std_light_data };
	ShaderUniform std_view_position = { "view_position", vector_binder, std_view_position_data };
	//ShaderUniform object_alpha = { "alpha", float_binder, alpha_data };
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
	// ===========================================================

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

	int width, height, nrChannels;
	unsigned char *image_data = stbi_load("../../assets/lenscolor.png", &width, &height, &nrChannels, 0);
	unsigned int lens_color_texture;

	for (int i = 0; i < width; i++){
		printf("%d\n", image_data[i]);
	}

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

	// Setup downsample shader for the quad ====================
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

	clock_t last_frame_time = clock();
	while (!glfwWindowShouldClose(window)) {
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

		// Compute the projection matrix.
		aspect = static_cast<float>(window_width) / window_height;
		projection_matrix =
			glm::perspective(glm::radians(45.0f), aspect, 0.0001f, 1000.0f);

		view_matrix = g_camera->get_view_matrix();

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

		menger2_pass.setup();
		CHECK_GL_ERROR(glDrawElements(GL_TRIANGLES, menger_faces.size() * 3, GL_UNSIGNED_INT, 0));
		// <<<Render Menger2>>>

		// <<<Render Floor>>>
		floor_pass.setup();
		CHECK_GL_ERROR(glDrawElements(GL_TRIANGLES, floor_faces.size() * 3, GL_UNSIGNED_INT, 0));
		// <<<Render Floor>>>
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

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


		// Final default pass to create final screen quad
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDisable(GL_DEPTH_TEST); // disable depth test so screen-space quad isn't discarded due to depth test.
    // clear all relevant buffers
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f); // set clear color to white (not really necessery actually, since we won't be able to see behind the quad anyways)
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		CHECK_GL_ERROR(glUseProgram(screen_default_program_id));
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, hdr_textureColorBuffer);	// use the color attachment texture as the texture of the quad plane
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, blur_textureColorBuffer);	// use the color attachment texture as the texture of the quad plane

    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);

		// Poll and swap.
		glfwPollEvents();
		glfwSwapBuffers(window);
	}
	glfwDestroyWindow(window);
	glfwTerminate();
	exit(EXIT_SUCCESS);
}
