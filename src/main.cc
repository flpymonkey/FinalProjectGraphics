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
#include <debuggl.h>

// Classes
#include "floor.h"
#include "menger.h"
#include "camera.h"
#include "controller.h"
#include "render_pass.h"

// Lights Tutorial
#include "shader.h"

// assimp
// #include <assimp/Importer.hpp>
// #include <assimp/scene.h>
// #include <assimp/postprocess.h>

struct MatrixPointers {
	const float *projection, *model, *view;
};

glm::mat4 projection_matrix;
glm::mat4 view_matrix;
glm::mat4 model_matrix;

float aspect = 0.0f;

int window_width = 800;
int window_height = 600;

// Used to brighten hdr exposure shader as described in this tutorial:
// https://learnopengl.com/Advanced-Lighting/HDR
float exposure = 1.0f;

Floor* g_floor;
Menger* g_menger;
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

// hdr shader which uses Reinhard tone mapping for high dynamic range
const char* screen_fragment_shader =
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
	//ShaderUniform object_alpha = { "alpha", float_binder, alpha_data };
	// <<<RenderPass Setup>>>

    // <<<Menger Data>>>
    std::vector<glm::vec4> menger_vertices;
    std::vector<glm::vec4> menger_normals;
    std::vector<glm::uvec3> menger_faces;

	g_menger->set_nesting_level(1);
	g_menger->generate_geometry(menger_vertices, menger_normals, menger_faces);
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

	// Setup fragment shader for the quad
	GLuint screen_fragment_shader_id = 0;
	const char* screen_fragment_source_pointer = screen_fragment_shader;
	CHECK_GL_ERROR(screen_fragment_shader_id = glCreateShader(GL_FRAGMENT_SHADER));
	CHECK_GL_ERROR(glShaderSource(screen_fragment_shader_id, 1,
				&screen_fragment_source_pointer, nullptr));
	glCompileShader(screen_fragment_shader_id);
	CHECK_GL_SHADER_ERROR(screen_fragment_shader_id);

	// Setup the program
	GLuint screen_program_id = 0;
  CHECK_GL_ERROR(screen_program_id = glCreateProgram());
	CHECK_GL_ERROR(glAttachShader(screen_program_id, screen_vertex_shader_id));
	CHECK_GL_ERROR(glAttachShader(screen_program_id, screen_fragment_shader_id));

	CHECK_GL_ERROR(glBindBuffer(GL_ARRAY_BUFFER, g_buffer_objects[kScreenVao][kVertexBuffer]));

	// Bind attributes.
	CHECK_GL_ERROR(glBindAttribLocation(screen_program_id, 0, "vertex_position"));
	CHECK_GL_ERROR(glBindAttribLocation(screen_program_id, 1, "aTexCoords"));

	CHECK_GL_ERROR(glBindFragDataLocation(screen_program_id, 0, "fragment_color"));
	glLinkProgram(screen_program_id);
	CHECK_GL_PROGRAM_ERROR(screen_program_id);

	// Get the uniform locations.
	GLint screen_projection_matrix_location = 0;
	CHECK_GL_ERROR(screen_projection_matrix_location =
			glGetUniformLocation(screen_program_id, "screenTexture"));

	// configure alternate framebuffer
	unsigned int framebuffer;
	glGenFramebuffers(1, &framebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

	// create a color attachment texture
	unsigned int textureColorBuffer;
	glGenTextures(1, &textureColorBuffer);
	glBindTexture(GL_TEXTURE_2D, textureColorBuffer);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, window_width, window_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureColorBuffer, 0);

	unsigned int rbo;
	glGenRenderbuffers(1, &rbo);
	glBindRenderbuffer(GL_RENDERBUFFER, rbo);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, window_width, window_height); // use a single renderbuffer object for both a depth AND stencil buffer.
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo); // now actually attach it

	// Check that frame buffer is set up correctly
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE){
		std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	clock_t last_frame_time = clock();
	while (!glfwWindowShouldClose(window)) {
		// render
		// ------
		// bind to framebuffer and draw scene as we normally would to color texture
		glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
		glEnable(GL_DEPTH_TEST); // enable depth testing (is disabled for rendering screen-space quad)

		// make sure we clear the framebuffer's content
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// // Switch to the Geometry VAO.
		// CHECK_GL_ERROR(glBindVertexArray(g_array_objects[kGeometryVao]));

		if (g_menger && g_menger->is_dirty()) {
		  	g_menger->generate_geometry(menger_vertices, menger_normals, menger_faces);
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
				{ menger_model, std_view, std_proj, std_light },
				{ "fragment_color" }
				);

		menger_pass.setup();
		CHECK_GL_ERROR(glDrawElements(GL_TRIANGLES, menger_faces.size() * 3, GL_UNSIGNED_INT, 0));
		// <<<Render Menger>>>

		// <<<Render Floor>>>
		floor_pass.setup();
		CHECK_GL_ERROR(glDrawElements(GL_TRIANGLES, floor_faces.size() * 3, GL_UNSIGNED_INT, 0));
		// <<<Render Floor>>>

		// now bind back to default framebuffer and draw a quad plane with the attached framebuffer color texture
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDisable(GL_DEPTH_TEST); // disable depth test so screen-space quad isn't discarded due to depth test.
    // clear all relevant buffers
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f); // set clear color to white (not really necessery actually, since we won't be able to see behind the quad anyways)
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Use our program.
		CHECK_GL_ERROR(glUseProgram(screen_program_id));
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, textureColorBuffer);	// use the color attachment texture as the texture of the quad plane

		// Set the exposure in the shader
		glUniform1f(glGetUniformLocation(screen_program_id, "exposure"), exposure);

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
