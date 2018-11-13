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
#include "menger.h"
#include "camera.h"

int window_width = 800, window_height = 600;
double prev_x = 0;
double prev_y = 0;

// Used to brighten hdr exposure shader as described in this tutorial:
// https://learnopengl.com/Advanced-Lighting/HDR
float exposure = 1.0f;

bool fps_mode = true;

// VBO and VAO descriptors.
enum { kVertexBuffer, kNormalBuffer, kIndexBuffer, kNumVbos };

// These are our VAOs.
enum { kGeometryVao, kFloorVao, kScreenVao, kNumVaos };

GLuint g_array_objects[kNumVaos];  // This will store the VAO descriptors.
GLuint g_buffer_objects[kNumVaos][kNumVbos];  // These will store VBO descriptors.

// C++ 11 String Literal
// See http://en.cppreference.com/w/cpp/language/string_literal
const char* vertex_shader =
R"zzz(#version 330 core
in vec4 vertex_position;
in vec4 vertex_normal;
uniform mat4 view;
uniform mat4 projection;
uniform vec4 light_position;
out vec4 light_direction;
out vec4 normal;
out vec4 world_normal;
out vec4 world_position;
void main()
{
// Transform vertex into clipping coordinates
	gl_Position = projection * view * vertex_position;
// Lighting in camera coordinates
//  Compute light direction and transform to camera coordinates
        light_direction = view * (light_position - vertex_position);
//  Transform normal to camera coordinates
        normal = view * vertex_normal;
        world_normal = vertex_normal;
        world_position = projection * view * vertex_position;
}
)zzz";

const char* fragment_shader =
R"zzz(#version 330 core
in vec4 normal;
in vec4 light_direction;
in vec4 world_normal;
out vec4 fragment_color;
void main()
{
	vec4 color = abs(normalize(world_normal)) + vec4(0.0, 0.0, 0.0, 1.0);
	float dot_nl = dot(normalize(light_direction), normalize(normal));
	dot_nl = clamp(dot_nl, 0.0, 1.0);
	fragment_color = clamp(dot_nl * color, 0.0, 1.0);
}
)zzz";

// FIXME: Implement shader effects with an alternative shader.
const char* floor_fragment_shader =
R"zzz(#version 330 core
in vec4 normal;
in vec4 light_direction;
in vec4 world_position;
uniform mat4 view;
uniform mat4 projection;
uniform vec4 light_position;
out vec4 fragment_color;
void main()
{
	vec4 position = inverse(projection * view) * vec4(world_position.xyz / world_position.w, 1.0f);
    position /= position.w;
	vec4 color = vec4(0.0, 0.0, 0.0, 1.0);
    if (mod(floor(position[0]) + floor(position[2]), 2) == 0) {
        color = vec4(1.0, 1.0, 1.0, 1.0);
    }
	float dot_nl = dot(normalize(light_direction), normalize(normal));
	dot_nl = clamp(dot_nl, 0.0, 1.0);
	fragment_color = clamp(dot_nl * color, 0.0, 1.0);
}
)zzz";

const char* screen_vertex_shader =
R"zzz(#version 330 core
in vec2 vertex_position;
in vec2 aTexCoords;
out vec2 TexCoords;
void main()
{
    TexCoords = aTexCoords;
    gl_Position = vec4(vertex_position.x, vertex_position.y, 0.0, 1.0);
}
)zzz";

// hdr shader which uses Reinhard tone mapping for high dynamic range
const char* screen_fragment_shader =
R"zzz(#version 330 core
out vec4 fragment_color;
in vec2 TexCoords;
uniform sampler2D screenTexture;

uniform float exposure;

void main()
{
		const float gamma = 2.2;
    vec3 hdrColor = texture(screenTexture, TexCoords).rgb;

		// Exposure tone mapping
    vec3 mapped = vec3(1.0) - exp(-hdrColor * exposure);
    // gamma correction
    mapped = pow(mapped, vec3(1.0 / gamma));

    fragment_color = vec4(mapped, 1.0);
}
)zzz";

// // this is the standard screen_fragment_shader, simply copies pixels with no post-processing
// const char* screen_fragment_shader =
// R"zzz(#version 330 core
// out vec4 fragment_color;
// in vec2 TexCoords;
// uniform sampler2D screenTexture;
// void main()
// {
//     vec3 col = texture(screenTexture, TexCoords).rgb;
//     fragment_color = vec4(col, 1.0);
// }
// )zzz";

// This fragment shader is the basis for doing lens flare's, but it does trippy stuff
// const char* screen_fragment_shader =
// R"zzz(#version 330 core
// in vec2 TexCoords;
// uniform sampler2D screenTexture;
//
// uniform int uGhosts; // number of ghost samples: set to 3
// uniform float uGhostDispersal; // dispersion factor
//
// out vec4 fragment_color;
//
// void main() {
//   vec2 texcoord = -TexCoords + vec2(1.0);
//   vec2 texelSize = 1.0 / vec2(textureSize(screenTexture, 0));
//
// // ghost vector to image centre:
//   vec2 ghostVec = (vec2(0.5) - texcoord) * 10; // uGhostDispersal
//
// // sample ghosts:
//   vec4 result = vec4(0.0);
//   for (int i = 0; i < 3; ++i) { // number of ghost samples: set to 3
//      vec2 offset = fract(texcoord + ghostVec * float(i));
//
//      result += texture(screenTexture, offset);
//   }
//
//   fragment_color = result;
// }
// )zzz";



void
ErrorCallback(int error, const char* description)
{
	std::cerr << "GLFW Error: " << description << "\n";
}

std::shared_ptr<Menger> g_menger;
Camera g_camera;

void
KeyCallback(GLFWwindow* window,
            int key,
            int scancode,
            int action,
            int mods)
{
	// Note:
	// This is only a list of functions to implement.
	// you may want to re-organize this piece of code.
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);
	else if (key == GLFW_KEY_W && action != GLFW_RELEASE) {
		if (fps_mode){
			g_camera.forwardTranslate();
		} else {
			g_camera.zoomForward();
		}
	} else if (key == GLFW_KEY_S && action != GLFW_RELEASE) {
		if (fps_mode){
			g_camera.backwardTranslate();
		} else {
			g_camera.zoomBackward();
		}
	} else if (key == GLFW_KEY_A && action != GLFW_RELEASE) {
		if (fps_mode){
			g_camera.leftTranslate();
		} else {
			g_camera.rightCenter();
		}
	} else if (key == GLFW_KEY_D && action != GLFW_RELEASE) {
		if (fps_mode) {
			g_camera.rightTranslate();
		} else {
			g_camera.leftCenter();
		}
	} else if (key == GLFW_KEY_LEFT && action != GLFW_RELEASE) {
		g_camera.rollLeft();
	} else if (key == GLFW_KEY_RIGHT && action != GLFW_RELEASE) {
		g_camera.rollRight();
	} else if (key == GLFW_KEY_DOWN && action != GLFW_RELEASE) {
		if (fps_mode){
			g_camera.downTranslate();
		} else {
			g_camera.downCenter();
		}
	} else if (key == GLFW_KEY_UP && action != GLFW_RELEASE) {
		if (fps_mode){
			g_camera.upTranslate();
		} else {
			g_camera.upCenter();
		}
	} else if (key == GLFW_KEY_C && action != GLFW_RELEASE) {
		fps_mode = !fps_mode;
	} else if (key == GLFW_KEY_Q && action != GLFW_RELEASE){
		// Adjust exposure down
		printf("%s\n", "DOWN");
		if (exposure > 0.0f){
			exposure -= 0.1f;
		} else {
			exposure = 0.0f;
		}
	} else if (key == GLFW_KEY_E && action != GLFW_RELEASE) {
		printf("%s\n", "UP");
		// Adjust exposure up
		exposure += 0.1f;
	}

	if (!g_menger)
		return ; // 0-4 only available in Menger mode.
	if (key == GLFW_KEY_0 && action != GLFW_RELEASE) {
		g_menger->set_nesting_level(0);
	} else if (key == GLFW_KEY_1 && action != GLFW_RELEASE) {
		g_menger->set_nesting_level(1);
	} else if (key == GLFW_KEY_2 && action != GLFW_RELEASE) {
		g_menger->set_nesting_level(2);
	} else if (key == GLFW_KEY_3 && action != GLFW_RELEASE) {
		g_menger->set_nesting_level(3);
	} else if (key == GLFW_KEY_4 && action != GLFW_RELEASE) {
		g_menger->set_nesting_level(4);
	}
}

int g_current_button;
bool g_mouse_pressed;

void
MousePosCallback(GLFWwindow* window, double mouse_x, double mouse_y)
{
	// FIXME COULD BE WEIRD RESULT IF MOUSE IS INITIALLY PRESSED ON GAME START
	if (!g_mouse_pressed){
		prev_x = mouse_x;
		prev_y = mouse_y;
		return;
	}
	if (g_current_button == GLFW_MOUSE_BUTTON_LEFT) {
		if (fps_mode) {
			g_camera.dynamicCenterRotate(prev_x, prev_y, mouse_x, mouse_y, window_width, window_height);
		} else {
			g_camera.dynamicEyeRotate(prev_x, prev_y, mouse_x, mouse_y, window_width, window_height);
		}
	} else if (g_current_button == GLFW_MOUSE_BUTTON_RIGHT) {
		g_camera.dynamicZoom(prev_y, mouse_y, window_width, window_height);
	} else if (g_current_button == GLFW_MOUSE_BUTTON_MIDDLE) {
		// FIXME: middle drag
	}
	prev_x = mouse_x;
	prev_y = mouse_y;
}

void
MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
	g_mouse_pressed = (action == GLFW_PRESS);
	g_current_button = button;
}

void
create_floor(std::vector<glm::vec4>& floor_obj_vertices,
    std::vector<glm::vec4>& floor_vtx_normals,
    std::vector<glm::uvec3>& floor_obj_faces)
{
    // Calculate half lengths.
    float length = 1000.0f;
    float half_length = length * 0.5f;
    float minx = -half_length;
    float maxx = half_length;
    float minz = -half_length;
    float maxz = half_length;

    // Floor data.
	// Top, bottom-right triangle.
	// floor_obj_vertices.push_back(glm::vec4(minx, -2.0f, maxz, 1.0f));
	// floor_vtx_normals.push_back(glm::vec4(0.0f, 1.0f, 0.0f, 0.0f));
	// floor_obj_vertices.push_back(glm::vec4(maxx, -2.0f, minz, 1.0f));
	// floor_vtx_normals.push_back(glm::vec4(0.0f, 1.0f, 0.0f, 0.0f));
	// floor_obj_vertices.push_back(glm::vec4(minx, -2.0f, minz, 1.0f));
	// floor_vtx_normals.push_back(glm::vec4(0.0f, 1.0f, 0.0f, 0.0f));
	// floor_obj_faces.push_back(glm::uvec3(0, 1, 2));

	// // Top, top-left triangle.
	// floor_obj_vertices.push_back(glm::vec4(minx, -2.0f, maxz, 1.0f));
	// floor_vtx_normals.push_back(glm::vec4(0.0f, 1.0f, 0.0f, 0.0f));
	// floor_obj_vertices.push_back(glm::vec4(maxx, -2.0f, maxz, 1.0f));
	// floor_vtx_normals.push_back(glm::vec4(0.0f, 1.0f, 0.0f, 0.0f));
	// floor_obj_vertices.push_back(glm::vec4(maxx, -2.0f, minz, 1.0f));
	// floor_vtx_normals.push_back(glm::vec4(0.0f, 1.0f, 0.0f, 0.0f));
	// floor_obj_faces.push_back(glm::uvec3(3, 4, 5));

	floor_obj_vertices.push_back(glm::vec4(0.0f, -2.0f, 0.0f, 1.0f)); // 0
	floor_vtx_normals.push_back(glm::vec4(0.0f, 1.0f, 0.0f, 0.0f));
	floor_obj_vertices.push_back(glm::vec4(1.0f, 0.0f, 0.0f, 0.0f)); // 1
	floor_vtx_normals.push_back(glm::vec4(0.0f, 1.0f, 0.0f, 0.0f));
	floor_obj_vertices.push_back(glm::vec4(0.0f, 0.0f, 1.0f, 0.0f)); // 2
	floor_vtx_normals.push_back(glm::vec4(0.0f, 1.0f, 0.0f, 0.0f));
	floor_obj_vertices.push_back(glm::vec4(-1.0f, 0.0f, 0.0f, 0.0f)); // 3
	floor_vtx_normals.push_back(glm::vec4(0.0f, 1.0f, 0.0f, 0.0f));
	floor_obj_vertices.push_back(glm::vec4(0.0f, 0.0f, -1.0f, 0.0f)); // 4
	floor_vtx_normals.push_back(glm::vec4(0.0f, 1.0f, 0.0f, 0.0f));
	floor_obj_faces.push_back(glm::uvec3(0, 2, 1));
	floor_obj_faces.push_back(glm::uvec3(0, 3, 2));
	floor_obj_faces.push_back(glm::uvec3(0, 4, 3));
	floor_obj_faces.push_back(glm::uvec3(0, 1, 4));
}

int main(int argc, char* argv[])
{
	std::string window_title = "Menger";
	if (!glfwInit()) exit(EXIT_FAILURE);
	g_menger = std::make_shared<Menger>();
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

	CHECK_SUCCESS(glewInit() == GLEW_OK);
	glGetError();  // clear GLEW's error for it
	glfwSetKeyCallback(window, KeyCallback);
	glfwSetCursorPosCallback(window, MousePosCallback);
	glfwSetMouseButtonCallback(window, MouseButtonCallback);
	glfwSwapInterval(1);
	const GLubyte* renderer = glGetString(GL_RENDERER);  // get renderer string
	const GLubyte* version = glGetString(GL_VERSION);    // version as a string
	std::cout << "Renderer: " << renderer << "\n";
	std::cout << "OpenGL version supported:" << version << "\n";

	glEnable(GL_CULL_FACE); // Added to see faces are correct.

    // FIXME: Create the geometry from a Menger object (in menger.cc).
    std::vector<glm::vec4> obj_vertices;
    std::vector<glm::vec4> vtx_normals;
    std::vector<glm::uvec3> obj_faces;

	g_menger->set_nesting_level(1);
	g_menger->generate_geometry(obj_vertices, vtx_normals, obj_faces);
	g_menger->set_clean();

	glm::vec4 min_bounds = glm::vec4(std::numeric_limits<float>::max());
	glm::vec4 max_bounds = glm::vec4(-std::numeric_limits<float>::max());
	for (int i = 0; i < obj_vertices.size(); ++i) {
		min_bounds = glm::min(obj_vertices[i], min_bounds);
		max_bounds = glm::max(obj_vertices[i], max_bounds);
	}
	std::cout << "min_bounds = " << glm::to_string(min_bounds) << "\n";
	std::cout << "max_bounds = " << glm::to_string(max_bounds) << "\n";

	// Setup our VAO array.
	CHECK_GL_ERROR(glGenVertexArrays(kNumVaos, &g_array_objects[0]));

	// Switch to the VAO for Geometry.
	CHECK_GL_ERROR(glBindVertexArray(g_array_objects[kGeometryVao]));

	// Generate buffer objects
	CHECK_GL_ERROR(glGenBuffers(kNumVbos, &g_buffer_objects[kGeometryVao][0]));

	// Setup vertex data in a VBO.
	CHECK_GL_ERROR(glBindBuffer(GL_ARRAY_BUFFER, g_buffer_objects[kGeometryVao][kVertexBuffer]));
	// NOTE: We do not send anything right now, we just describe it to OpenGL.
	CHECK_GL_ERROR(glBufferData(GL_ARRAY_BUFFER,
				sizeof(float) * obj_vertices.size() * 4, nullptr,
				GL_STATIC_DRAW));
	CHECK_GL_ERROR(glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0));
	CHECK_GL_ERROR(glEnableVertexAttribArray(0));

	CHECK_GL_ERROR(glBindBuffer(GL_ARRAY_BUFFER, g_buffer_objects[kGeometryVao][kNormalBuffer]));
	// NOTE: We do not send anything right now, we just describe it to OpenGL.
	CHECK_GL_ERROR(glBufferData(GL_ARRAY_BUFFER,
				sizeof(float) * vtx_normals.size() * 4, nullptr,
				GL_STATIC_DRAW));
	CHECK_GL_ERROR(glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, 0));
	CHECK_GL_ERROR(glEnableVertexAttribArray(1));

	// Setup element array buffer.
	CHECK_GL_ERROR(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_buffer_objects[kGeometryVao][kIndexBuffer]));
	CHECK_GL_ERROR(glBufferData(GL_ELEMENT_ARRAY_BUFFER,
				sizeof(uint32_t) * obj_faces.size() * 3,
				&obj_faces[0], GL_STATIC_DRAW));

	/*
 	 * So far the geometry is loaded into g_buffer_objects[kGeometryVao][*].
	 * These buffers are bound to g_array_objects[kGeometryVao]
	 */

	// FIXME: load the floor into g_buffer_objects[kFloorVao][*],
	//        and bind the VBO to g_array_objects[kFloorVao]
    std::vector<glm::vec4> floor_obj_vertices;
    std::vector<glm::vec4> floor_vtx_normals;
    std::vector<glm::uvec3> floor_obj_faces;

    create_floor(floor_obj_vertices, floor_vtx_normals, floor_obj_faces);


  // Switch to Floor VAO.
	CHECK_GL_ERROR(glBindVertexArray(g_array_objects[kFloorVao]));

	// Generate buffer objects
	CHECK_GL_ERROR(glGenBuffers(kNumVbos, &g_buffer_objects[kFloorVao][0]));

	// Setup vertex data in a VBO.
	CHECK_GL_ERROR(glBindBuffer(GL_ARRAY_BUFFER, g_buffer_objects[kFloorVao][kVertexBuffer]));
	// NOTE: We do not send anything right now, we just describe it to OpenGL.
	CHECK_GL_ERROR(glBufferData(GL_ARRAY_BUFFER,
				sizeof(float) * floor_obj_vertices.size() * 4, &floor_obj_vertices[0],
				GL_STATIC_DRAW));
	CHECK_GL_ERROR(glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0));
	CHECK_GL_ERROR(glEnableVertexAttribArray(0));

	CHECK_GL_ERROR(glBindBuffer(GL_ARRAY_BUFFER, g_buffer_objects[kFloorVao][kNormalBuffer]));
	// NOTE: We do not send anything right now, we just describe it to OpenGL.
	CHECK_GL_ERROR(glBufferData(GL_ARRAY_BUFFER,
				sizeof(float) * floor_vtx_normals.size() * 4, &floor_vtx_normals[0],
				GL_STATIC_DRAW));
	CHECK_GL_ERROR(glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, 0));
	CHECK_GL_ERROR(glEnableVertexAttribArray(1));

	// Setup element array buffer.
	CHECK_GL_ERROR(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_buffer_objects[kFloorVao][kIndexBuffer]));
	CHECK_GL_ERROR(glBufferData(GL_ELEMENT_ARRAY_BUFFER,
				sizeof(uint32_t) * floor_obj_faces.size() * 3,
				&floor_obj_faces[0], GL_STATIC_DRAW));

	// Setup vertex shader.
	GLuint vertex_shader_id = 0;
	const char* vertex_source_pointer = vertex_shader;
	CHECK_GL_ERROR(vertex_shader_id = glCreateShader(GL_VERTEX_SHADER));
	CHECK_GL_ERROR(glShaderSource(vertex_shader_id, 1, &vertex_source_pointer, nullptr));
	glCompileShader(vertex_shader_id);
	CHECK_GL_SHADER_ERROR(vertex_shader_id);

	// Setup fragment shader.
	GLuint fragment_shader_id = 0;
	const char* fragment_source_pointer = fragment_shader;
	CHECK_GL_ERROR(fragment_shader_id = glCreateShader(GL_FRAGMENT_SHADER));
	CHECK_GL_ERROR(glShaderSource(fragment_shader_id, 1, &fragment_source_pointer, nullptr));
	glCompileShader(fragment_shader_id);
	CHECK_GL_SHADER_ERROR(fragment_shader_id);

	// Let's create our program.
	GLuint program_id = 0;
	CHECK_GL_ERROR(program_id = glCreateProgram());
	CHECK_GL_ERROR(glAttachShader(program_id, vertex_shader_id));
	CHECK_GL_ERROR(glAttachShader(program_id, fragment_shader_id));

	CHECK_GL_ERROR(glBindBuffer(GL_ARRAY_BUFFER, g_buffer_objects[kGeometryVao][kVertexBuffer]));
	// NOTE: We do not send anything right now, we just describe it to OpenGL.
	CHECK_GL_ERROR(glBufferData(GL_ARRAY_BUFFER,
				sizeof(float) * obj_vertices.size() * 4, nullptr,
				GL_STATIC_DRAW));
	CHECK_GL_ERROR(glBindBuffer(GL_ARRAY_BUFFER, g_buffer_objects[kGeometryVao][kNormalBuffer]));
	// NOTE: We do not send anything right now, we just describe it to OpenGL.
	CHECK_GL_ERROR(glBufferData(GL_ARRAY_BUFFER,
				sizeof(float) * vtx_normals.size() * 4, nullptr,
				GL_STATIC_DRAW));
	// Bind attributes.
	CHECK_GL_ERROR(glBindAttribLocation(program_id, 0, "vertex_position"));

	CHECK_GL_ERROR(glBindAttribLocation(program_id, 1, "vertex_normal"));
	CHECK_GL_ERROR(glBindFragDataLocation(program_id, 0, "fragment_color"));
	glLinkProgram(program_id);
	CHECK_GL_PROGRAM_ERROR(program_id);

	// Get the uniform locations.
	GLint projection_matrix_location = 0;
	CHECK_GL_ERROR(projection_matrix_location =
			glGetUniformLocation(program_id, "projection"));
	GLint view_matrix_location = 0;
	CHECK_GL_ERROR(view_matrix_location =
			glGetUniformLocation(program_id, "view"));
	GLint light_position_location = 0;
	CHECK_GL_ERROR(light_position_location =
			glGetUniformLocation(program_id, "light_position"));

	// Create floor program ========================
	// Setup fragment shader for the floor
	GLuint floor_fragment_shader_id = 0;
	const char* floor_fragment_source_pointer = floor_fragment_shader;
	CHECK_GL_ERROR(floor_fragment_shader_id = glCreateShader(GL_FRAGMENT_SHADER));
	CHECK_GL_ERROR(glShaderSource(floor_fragment_shader_id, 1,
				&floor_fragment_source_pointer, nullptr));
	glCompileShader(floor_fragment_shader_id);
	CHECK_GL_SHADER_ERROR(floor_fragment_shader_id);

	// FIXME: Setup another program for the floor, and get its locations.
	// Note: you can reuse the vertex and geometry shader objects
	GLuint floor_program_id = 0;
    CHECK_GL_ERROR(floor_program_id = glCreateProgram());
	CHECK_GL_ERROR(glAttachShader(floor_program_id, vertex_shader_id));
	CHECK_GL_ERROR(glAttachShader(floor_program_id, floor_fragment_shader_id));

	CHECK_GL_ERROR(glBindBuffer(GL_ARRAY_BUFFER, g_buffer_objects[kFloorVao][kVertexBuffer]));

	// Bind attributes.
	CHECK_GL_ERROR(glBindAttribLocation(floor_program_id, 0, "vertex_position"));
	CHECK_GL_ERROR(glBindAttribLocation(floor_program_id, 1, "vertex_normal"));
	CHECK_GL_ERROR(glBindFragDataLocation(floor_program_id, 0, "fragment_color"));
	glLinkProgram(floor_program_id);
	CHECK_GL_PROGRAM_ERROR(floor_program_id);

    // Get the uniform locations.
	GLint floor_projection_matrix_location = 0;
	CHECK_GL_ERROR(floor_projection_matrix_location =
			glGetUniformLocation(floor_program_id, "projection"));
	GLint floor_view_matrix_location = 0;
	CHECK_GL_ERROR(floor_view_matrix_location =
			glGetUniformLocation(floor_program_id, "view"));
	GLint floor_light_position_location = 0;
	CHECK_GL_ERROR(floor_light_position_location =
			glGetUniformLocation(floor_program_id, "light_position"));

	glm::vec4 light_position = glm::vec4(5.0f, 5.0f, 5.0f, 1.0f);
	float aspect = 0.0f;
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

	// Create basic quad program ========================
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

		// Switch to the Geometry VAO.
		CHECK_GL_ERROR(glBindVertexArray(g_array_objects[kGeometryVao]));

		if (g_menger && g_menger->is_dirty()) {
		  g_menger->generate_geometry(obj_vertices, vtx_normals, obj_faces);
			g_menger->set_clean();
		}

		// Compute the projection matrix.
		aspect = static_cast<float>(window_width) / window_height;
		glm::mat4 projection_matrix =
			glm::perspective(glm::radians(45.0f), aspect, 0.0001f, 1000.0f);

		glm::mat4 view_matrix = g_camera.get_view_matrix();

		// Send vertices to the GPU.
		CHECK_GL_ERROR(glBindBuffer(GL_ARRAY_BUFFER,
		                            g_buffer_objects[kGeometryVao][kVertexBuffer]));
		CHECK_GL_ERROR(glBufferData(GL_ARRAY_BUFFER,
		                            sizeof(float) * obj_vertices.size() * 4,
		                            &obj_vertices[0], GL_STATIC_DRAW));
		CHECK_GL_ERROR(glBindBuffer(GL_ARRAY_BUFFER,
		                            g_buffer_objects[kGeometryVao][kNormalBuffer]));
		CHECK_GL_ERROR(glBufferData(GL_ARRAY_BUFFER,
		                            sizeof(float) * vtx_normals.size() * 4,
		                            &vtx_normals[0], GL_STATIC_DRAW));
		CHECK_GL_ERROR(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_buffer_objects[kGeometryVao][kIndexBuffer]));
		CHECK_GL_ERROR(glBufferData(GL_ELEMENT_ARRAY_BUFFER,
					sizeof(uint32_t) * obj_faces.size() * 3,
					&obj_faces[0], GL_STATIC_DRAW));

		// Use our program.
		CHECK_GL_ERROR(glUseProgram(program_id));

		// Pass uniforms in.
		CHECK_GL_ERROR(glUniformMatrix4fv(projection_matrix_location, 1, GL_FALSE,
					&projection_matrix[0][0]));
		CHECK_GL_ERROR(glUniformMatrix4fv(view_matrix_location, 1, GL_FALSE,
					&view_matrix[0][0]));
		CHECK_GL_ERROR(glUniform4fv(light_position_location, 1, &light_position[0]));

		// Draw our triangles.
		CHECK_GL_ERROR(glDrawElements(GL_TRIANGLES, obj_faces.size() * 3, GL_UNSIGNED_INT, 0));

		// FIXME: Render the floor
		// Note: What you need to do is
		// 	1. Switch VAO
		// 	2. Switch Program
		// 	3. Pass Uniforms
		// 	4. Call glDrawElements, since input geometry is
		// 	indicated by VAO.

    // Switch to Floor VAO.
    CHECK_GL_ERROR(glBindVertexArray(g_array_objects[kFloorVao]));

    // Use our program.
		CHECK_GL_ERROR(glUseProgram(floor_program_id));

		// Pass uniforms in.
		CHECK_GL_ERROR(glUniformMatrix4fv(floor_projection_matrix_location, 1, GL_FALSE,
					&projection_matrix[0][0]));
		CHECK_GL_ERROR(glUniformMatrix4fv(floor_view_matrix_location, 1, GL_FALSE,
					&view_matrix[0][0]));
		CHECK_GL_ERROR(glUniform4fv(floor_light_position_location, 1, &light_position[0]));

		// Draw our triangles.
		CHECK_GL_ERROR(glDrawElements(GL_TRIANGLES, floor_obj_faces.size() * 3, GL_UNSIGNED_INT, 0));


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
