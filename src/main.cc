#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <ctime>
#include <random>

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

# define M_PI           3.14159265358979323846  /* pi */

//gui
#include "gui.h"

// game state variables:
// ====================
int window_width = 1080;
int window_height = 720;
bool showGui = true;

// Depth of field controls
bool drunkMode = false;
int light_rays_for_bokeh = 1; // number of light rays
float aperture = 0.05;

// Used to brighten hdr exposure shader as described in this tutorial:
// https://learnopengl.com/Advanced-Lighting/HDR
float exposure = 0.8f;
bool showMeshes = true;
bool lensEffects = true;
bool captureImage = false;
// ====================

struct MatrixPointers {
	const float *projection, *model, *view;
};

glm::mat4 projection_matrix;
glm::mat4 view_matrix;
glm::mat4 model_matrix;

float aspect = 0.0f;

// Initialize static member of class Box
int Object::object_count = 0;

// gui variables
int score = 0;
std::string object_goal = "doggie";

// Game logic
// List of objects which will be photographable
std::vector<Object *> photo_objects;
int goal_id;

// Chooses a random object and sets goal_id and object_goal;
void
choose_photo_object() {
	std::random_device rd;
	std::mt19937 rng(rd());
	std::uniform_int_distribution<int> uni(0, photo_objects.size() - 1);
	int i = uni(rng);
	object_goal = photo_objects[i]->name;
	goal_id = photo_objects[i]->object_id;
}

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

	// Setup GUI
	BasicGUI* gui = new BasicGUI(window, &score, &object_goal);

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
	PointLight pointLight = PointLight(glm::vec3(0.0f, 25.0f, 10.0f));
	pointLights.push_back(pointLight);
    pointLight = PointLight(glm::vec3(-20.0f, 50.0f, -20.0f));
    pointLights.push_back(pointLight);

	std::vector<SpotLight> spotLights;
	SpotLight spotLight = SpotLight(glm::vec3(12.0f, 20.0f, 0.0f), glm::vec3(-1.0f, -1.0f, 0.0f));
	spotLights.push_back(spotLight);
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

    glm::vec4 menger_pos = glm::vec4(0.0f, 25.0f, 10.0f, 1.0f);

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

    // <<<Scene>>>
    // <<<Cone>>>
    Object* cone = new Object("Christmas Tree");
    cone->load("/src/assets/primitives/cone.obj");

    glm::mat4 cone_model_matrix = glm::mat4(1.0f);

    cone_model_matrix = cone->translate(cone_model_matrix, glm::vec3(0.0f, 15.0f, 10.0f));
    //cone_model_matrix = cone->rotate(cone_model_matrix, 0.0f, glm::vec3(0.0f, 0.0f, 0.0f));
    cone_model_matrix = cone->scale(cone_model_matrix, glm::vec3(5.0f, 10.0f, 5.0f));

    auto cone_model_data = [&cone_model_matrix]() -> const void* {
		return &cone_model_matrix[0][0];
    };

    ShaderUniform cone_model = {"model", matrix_binder, cone_model_data};

    cone->shaders(object_vertex_shader, NULL, object_fragment_shader);
    cone->uniforms(cone_model, std_view, std_proj, std_light, std_view_position);
    cone->lights(directionalLights, pointLights, spotLights);
    cone->textures("/src/assets/textures/grass.png", "/src/assets/textures/wall_s.jpg");

    cone->setup();

	photo_objects.push_back(cone);
    // <<<Cone>>>

    // <<<Sphere>>>
    Object* sphere = new Object("The Sun");
    sphere->load("/src/assets/primitives/sphere.obj");

    glm::mat4 sphere_model_matrix = glm::mat4(1.0f);

    sphere_model_matrix = sphere->translate(sphere_model_matrix, glm::vec3(-20.0f, 50.0f, -20.0f));
    //sphere_model_matrix = sphere->rotate(sphere_model_matrix, 0.0f, glm::vec3(0.0f, 0.0f, 0.0f));
    sphere_model_matrix = sphere->scale(sphere_model_matrix, glm::vec3(5.0f, 5.0f, 5.0f));

    auto sphere_model_data = [&sphere_model_matrix]() -> const void* {
		return &sphere_model_matrix[0][0];
    };

    ShaderUniform sphere_model = {"model", matrix_binder, sphere_model_data};

    sphere->shaders(object_vertex_shader, NULL, object_fragment_shader);
    sphere->uniforms(sphere_model, std_view, std_proj, std_light, std_view_position);
    sphere->lights(directionalLights, pointLights, spotLights);
    sphere->lightColor(glm::vec4(1.9f, 1.9f, 1.9f, 1.0f));
    sphere->textures("/src/assets/textures/gold.jpg", "/src/assets/textures/wall_s.jpg");

    sphere->setup();

	photo_objects.push_back(sphere);
    // <<<Sphere>>>

    // <<<Sphere2>>>
    Object* sphere2 = new Object("Sphere 2");
    sphere2->load("/src/assets/primitives/sphere2.obj");

    glm::mat4 sphere2_model_matrix = glm::mat4(1.0f);

    sphere2_model_matrix = sphere2->translate(sphere2_model_matrix, glm::vec3(3.0f, 2.0f, 13.0f));
    //sphere2_model_matrix = sphere2->rotate(sphere2_model_matrix, 0.0f, glm::vec3(0.0f, 0.0f, 0.0f));
    sphere2_model_matrix = sphere2->scale(sphere2_model_matrix, glm::vec3(0.1f, 0.1f, 0.1f));

    auto sphere2_model_data = [&sphere2_model_matrix]() -> const void* {
		return &sphere2_model_matrix[0][0];
    };

    ShaderUniform sphere2_model = {"model", matrix_binder, sphere2_model_data};

    sphere2->shaders(object_vertex_shader, NULL, object_fragment_shader);
    sphere2->uniforms(sphere2_model, std_view, std_proj, std_light, std_view_position);
    sphere2->lights(directionalLights, pointLights, spotLights);
    sphere2->textures("/src/assets/textures/wood2.png", "/src/assets/textures/wood_s.jpg");

    sphere2->setup();
    // <<<Sphere2>>>

		// <<<TreeLight>>>
		Object* treelight = new Object("treelight");
		treelight->load("/src/assets/primitives/sphere.obj");

		glm::mat4 treelight_model_matrix = glm::mat4(1.0f);

		treelight_model_matrix = treelight->translate(treelight_model_matrix, glm::vec3(0.0f, 5.0f, 5.0f));
		//treelight_model_matrix = treelight->rotate(treelight_model_matrix, 0.0f, glm::vec3(0.0f, 0.0f, 0.0f));
		treelight_model_matrix = treelight->scale(treelight_model_matrix, glm::vec3(0.1f, 0.1f, 0.1f));

		auto treelight_model_data = [&treelight_model_matrix]() -> const void* {
		return &treelight_model_matrix[0][0];
		};

		ShaderUniform treelight_model = {"model", matrix_binder, treelight_model_data};

		treelight->shaders(object_vertex_shader, NULL, object_fragment_shader);
		treelight->uniforms(treelight_model, std_view, std_proj, std_light, std_view_position);
		treelight->lights(directionalLights, pointLights, spotLights);
		treelight->lightColor(glm::vec4(1.0f, 1.0f, 1.5f, 1.0f));
		treelight->textures("/src/assets/textures/gold.jpg", "/src/assets/textures/wall_s.jpg");

		treelight->setup();
		// <<<TreeLight>>>

		// <<<TreeLight2>>>
		Object* treelight2 = new Object("treelight2");
		treelight2->load("/src/assets/primitives/sphere.obj");

		glm::mat4 treelight2_model_matrix = glm::mat4(1.0f);

		treelight2_model_matrix = treelight2->translate(treelight2_model_matrix, glm::vec3(2.0f, 7.0f, 6.0f));
		//treelight2_model_matrix = treelight2->rotate(treelight2_model_matrix, 0.0f, glm::vec3(0.0f, 0.0f, 0.0f));
		treelight2_model_matrix = treelight2->scale(treelight2_model_matrix, glm::vec3(0.1f, 0.1f, 0.1f));

		auto treelight2_model_data = [&treelight2_model_matrix]() -> const void* {
		return &treelight2_model_matrix[0][0];
		};

		ShaderUniform treelight2_model = {"model", matrix_binder, treelight2_model_data};

		treelight2->shaders(object_vertex_shader, NULL, object_fragment_shader);
		treelight2->uniforms(treelight2_model, std_view, std_proj, std_light, std_view_position);
		treelight2->lights(directionalLights, pointLights, spotLights);
		treelight2->lightColor(glm::vec4(1.0f, 1.0f, 1.5f, 1.0f));
		treelight2->textures("/src/assets/textures/gold.jpg", "/src/assets/textures/wall_s.jpg");

		treelight2->setup();
		// <<<TreeLight2>>>

		// <<<TreeLight3>>>
		Object* treelight3 = new Object("treelight3");
		treelight3->load("/src/assets/primitives/sphere.obj");

		glm::mat4 treelight3_model_matrix = glm::mat4(1.0f);

		treelight3_model_matrix = treelight3->translate(treelight3_model_matrix, glm::vec3(-2.0f, 9.0f, 6.5f));
		//treelight3_model_matrix = treelight3->rotate(treelight3_model_matrix, 0.0f, glm::vec3(0.0f, 0.0f, 0.0f));
		treelight3_model_matrix = treelight3->scale(treelight3_model_matrix, glm::vec3(0.1f, 0.1f, 0.1f));

		auto treelight3_model_data = [&treelight3_model_matrix]() -> const void* {
		return &treelight3_model_matrix[0][0];
		};

		ShaderUniform treelight3_model = {"model", matrix_binder, treelight3_model_data};

		treelight3->shaders(object_vertex_shader, NULL, object_fragment_shader);
		treelight3->uniforms(treelight3_model, std_view, std_proj, std_light, std_view_position);
		treelight3->lights(directionalLights, pointLights, spotLights);
		treelight3->lightColor(glm::vec4(1.0f, 1.0f, 1.5f, 1.0f));
		treelight3->textures("/src/assets/textures/gold.jpg", "/src/assets/textures/wall_s.jpg");

		treelight3->setup();
		// <<<TreeLight3>>>

		// <<<TreeLight4>>>
		Object* treelight4 = new Object("treelight4");
		treelight4->load("/src/assets/primitives/sphere.obj");

		glm::mat4 treelight4_model_matrix = glm::mat4(1.0f);

		treelight4_model_matrix = treelight4->translate(treelight4_model_matrix, glm::vec3(0.5f, 11.0f, 6.5f));
		//treelight4_model_matrix = treelight4->rotate(treelight4_model_matrix, 0.0f, glm::vec3(0.0f, 0.0f, 0.0f));
		treelight4_model_matrix = treelight4->scale(treelight4_model_matrix, glm::vec3(0.1f, 0.1f, 0.1f));

		auto treelight4_model_data = [&treelight4_model_matrix]() -> const void* {
		return &treelight4_model_matrix[0][0];
		};

		ShaderUniform treelight4_model = {"model", matrix_binder, treelight4_model_data};

		treelight4->shaders(object_vertex_shader, NULL, object_fragment_shader);
		treelight4->uniforms(treelight4_model, std_view, std_proj, std_light, std_view_position);
		treelight4->lights(directionalLights, pointLights, spotLights);
		treelight4->lightColor(glm::vec4(1.0f, 1.0f, 1.5f, 1.0f));
		treelight4->textures("/src/assets/textures/gold.jpg", "/src/assets/textures/wall_s.jpg");

		treelight4->setup();
		// <<<TreeLight4>>>

		// <<<TreeLight5>>>
		Object* treelight5 = new Object("treelight5");
		treelight5->load("/src/assets/primitives/sphere.obj");

		glm::mat4 treelight5_model_matrix = glm::mat4(1.0f);

		treelight5_model_matrix = treelight5->translate(treelight5_model_matrix, glm::vec3(2.0f, 15.0f, 8.5f));
		//treelight5_model_matrix = treelight5->rotate(treelight5_model_matrix, 0.0f, glm::vec3(0.0f, 0.0f, 0.0f));
		treelight5_model_matrix = treelight5->scale(treelight5_model_matrix, glm::vec3(0.1f, 0.1f, 0.1f));

		auto treelight5_model_data = [&treelight5_model_matrix]() -> const void* {
		return &treelight5_model_matrix[0][0];
		};

		ShaderUniform treelight5_model = {"model", matrix_binder, treelight5_model_data};

		treelight5->shaders(object_vertex_shader, NULL, object_fragment_shader);
		treelight5->uniforms(treelight5_model, std_view, std_proj, std_light, std_view_position);
		treelight5->lights(directionalLights, pointLights, spotLights);
		treelight5->lightColor(glm::vec4(1.0f, 1.0f, 1.5f, 1.0f));
		treelight5->textures("/src/assets/textures/gold.jpg", "/src/assets/textures/wall_s.jpg");

		treelight5->setup();
		// <<<TreeLight5>>>

		// <<<TreeLight6>>>
		Object* treelight6 = new Object("treelight6");
		treelight6->load("/src/assets/primitives/sphere.obj");

		glm::mat4 treelight6_model_matrix = glm::mat4(1.0f);

		treelight6_model_matrix = treelight6->translate(treelight6_model_matrix, glm::vec3(-1.0f, 17.0f, 8.1f));
		//treelight6_model_matrix = treelight6->rotate(treelight6_model_matrix, 0.0f, glm::vec3(0.0f, 0.0f, 0.0f));
		treelight6_model_matrix = treelight6->scale(treelight6_model_matrix, glm::vec3(0.1f, 0.1f, 0.1f));

		auto treelight6_model_data = [&treelight6_model_matrix]() -> const void* {
		return &treelight6_model_matrix[0][0];
		};

		ShaderUniform treelight6_model = {"model", matrix_binder, treelight6_model_data};

		treelight6->shaders(object_vertex_shader, NULL, object_fragment_shader);
		treelight6->uniforms(treelight6_model, std_view, std_proj, std_light, std_view_position);
		treelight6->lights(directionalLights, pointLights, spotLights);
		treelight6->lightColor(glm::vec4(1.0f, 1.0f, 1.5f, 1.0f));
		treelight6->textures("/src/assets/textures/gold.jpg", "/src/assets/textures/wall_s.jpg");

		treelight6->setup();
		// <<<TreeLight6>>>

		// <<<TreeLight7>>>
		Object* treelight7 = new Object("treelight7");
		treelight7->load("/src/assets/primitives/sphere.obj");

		glm::mat4 treelight7_model_matrix = glm::mat4(1.0f);

		treelight7_model_matrix = treelight7->translate(treelight7_model_matrix, glm::vec3(0.2f, 19.5f, 8.3f));
		//treelight7_model_matrix = treelight7->rotate(treelight7_model_matrix, 0.0f, glm::vec3(0.0f, 0.0f, 0.0f));
		treelight7_model_matrix = treelight7->scale(treelight7_model_matrix, glm::vec3(0.1f, 0.1f, 0.1f));

		auto treelight7_model_data = [&treelight7_model_matrix]() -> const void* {
		return &treelight7_model_matrix[0][0];
		};

		ShaderUniform treelight7_model = {"model", matrix_binder, treelight7_model_data};

		treelight7->shaders(object_vertex_shader, NULL, object_fragment_shader);
		treelight7->uniforms(treelight7_model, std_view, std_proj, std_light, std_view_position);
		treelight7->lights(directionalLights, pointLights, spotLights);
		treelight7->lightColor(glm::vec4(1.0f, 1.0f, 1.5f, 1.0f));
		treelight7->textures("/src/assets/textures/gold.jpg", "/src/assets/textures/wall_s.jpg");

		treelight7->setup();
		// <<<TreeLight7>>>

		// <<<TreeLight8>>>
		Object* treelight8 = new Object("treelight8");
		treelight8->load("/src/assets/primitives/sphere.obj");

		glm::mat4 treelight8_model_matrix = glm::mat4(1.0f);

		treelight8_model_matrix = treelight8->translate(treelight8_model_matrix, glm::vec3(4.0f, 5.0f, 7.0f));
		//treelight8_model_matrix = treelight8->rotate(treelight8_model_matrix, 0.0f, glm::vec3(0.0f, 0.0f, 0.0f));
		treelight8_model_matrix = treelight8->scale(treelight8_model_matrix, glm::vec3(0.1f, 0.1f, 0.1f));

		auto treelight8_model_data = [&treelight8_model_matrix]() -> const void* {
		return &treelight8_model_matrix[0][0];
		};

		ShaderUniform treelight8_model = {"model", matrix_binder, treelight8_model_data};

		treelight8->shaders(object_vertex_shader, NULL, object_fragment_shader);
		treelight8->uniforms(treelight8_model, std_view, std_proj, std_light, std_view_position);
		treelight8->lights(directionalLights, pointLights, spotLights);
		treelight8->lightColor(glm::vec4(1.0f, 1.0f, 1.5f, 1.0f));
		treelight8->textures("/src/assets/textures/gold.jpg", "/src/assets/textures/wall_s.jpg");

		treelight8->setup();
		// <<<TreeLight8>>>

		// <<<TreeLight9>>>
		Object* treelight9 = new Object("treelight9");
		treelight9->load("/src/assets/primitives/sphere.obj");

		glm::mat4 treelight9_model_matrix = glm::mat4(1.0f);

		treelight9_model_matrix = treelight9->translate(treelight9_model_matrix, glm::vec3(-4.0f, 5.0f, 7.0f));
		//treelight9_model_matrix = treelight9->rotate(treelight9_model_matrix, 0.0f, glm::vec3(0.0f, 0.0f, 0.0f));
		treelight9_model_matrix = treelight9->scale(treelight9_model_matrix, glm::vec3(0.1f, 0.1f, 0.1f));

		auto treelight9_model_data = [&treelight9_model_matrix]() -> const void* {
		return &treelight9_model_matrix[0][0];
		};

		ShaderUniform treelight9_model = {"model", matrix_binder, treelight9_model_data};

		treelight9->shaders(object_vertex_shader, NULL, object_fragment_shader);
		treelight9->uniforms(treelight9_model, std_view, std_proj, std_light, std_view_position);
		treelight9->lights(directionalLights, pointLights, spotLights);
		treelight9->lightColor(glm::vec4(1.0f, 1.0f, 1.5f, 1.0f));
		treelight9->textures("/src/assets/textures/gold.jpg", "/src/assets/textures/wall_s.jpg");

		treelight9->setup();
		// <<<TreeLight9>>>

		// <<<TreeLight10>>>
		Object* treelight10 = new Object("treelight10");
		treelight10->load("/src/assets/primitives/sphere.obj");

		glm::mat4 treelight10_model_matrix = glm::mat4(1.0f);

		treelight10_model_matrix = treelight10->translate(treelight10_model_matrix, glm::vec3(-2.0f, 6.8f, 5.9f));
		//treelight10_model_matrix = treelight10->rotate(treelight10_model_matrix, 0.0f, glm::vec3(0.0f, 0.0f, 0.0f));
		treelight10_model_matrix = treelight10->scale(treelight10_model_matrix, glm::vec3(0.1f, 0.1f, 0.1f));

		auto treelight10_model_data = [&treelight10_model_matrix]() -> const void* {
		return &treelight10_model_matrix[0][0];
		};

		ShaderUniform treelight10_model = {"model", matrix_binder, treelight10_model_data};

		treelight10->shaders(object_vertex_shader, NULL, object_fragment_shader);
		treelight10->uniforms(treelight10_model, std_view, std_proj, std_light, std_view_position);
		treelight10->lights(directionalLights, pointLights, spotLights);
		treelight10->lightColor(glm::vec4(1.0f, 1.0f, 1.5f, 1.0f));
		treelight10->textures("/src/assets/textures/gold.jpg", "/src/assets/textures/wall_s.jpg");

		treelight10->setup();
		// <<<TreeLight10>>>

    // <<<Cylinder>>>
    Object* cylinder = new Object("Cyl");
    cylinder->load("/src/assets/primitives/cylinder.obj");

    glm::mat4 cylinder_model_matrix = glm::mat4(1.0f);

    cylinder_model_matrix = cylinder->translate(cylinder_model_matrix, glm::vec3(0.0f, 1.0f, 10.0f));
    //cylinder_model_matrix = cylinder->rotate(cylinder_model_matrix, 0.0f, glm::vec3(0.0f, 0.0f, 0.0f));
    cylinder_model_matrix = cylinder->scale(cylinder_model_matrix, glm::vec3(1.0f, 5.0f, 1.0f));

    auto cylinder_model_data = [&cylinder_model_matrix]() -> const void* {
		return &cylinder_model_matrix[0][0];
    };

    ShaderUniform cylinder_model = {"model", matrix_binder, cylinder_model_data};

    cylinder->shaders(object_vertex_shader, NULL, object_fragment_shader);
    cylinder->uniforms(cylinder_model, std_view, std_proj, std_light, std_view_position);
    cylinder->lights(directionalLights, pointLights, spotLights);
    cylinder->textures("/src/assets/textures/wood.jpg", "/src/assets/textures/wood_s.jpg");

    cylinder->setup();
    // <<<Cylinder>>>

    // <<<Torus>>>
    Object* torus = new Object("The Grand Torus!");
    torus->load("/src/assets/primitives/torus.obj");

    glm::mat4 torus_model_matrix = glm::mat4(1.0f);

    torus_model_matrix = torus->translate(torus_model_matrix, glm::vec3(-10.0f, 2.5f, -10.0f));
    torus_model_matrix = torus->rotate(torus_model_matrix, 0.7854f, glm::vec3(1.0f, 0.0f, 1.0f));
    torus_model_matrix = torus->scale(torus_model_matrix, glm::vec3(10.0f, 10.0f, 5.0f));

    auto torus_model_data = [&torus_model_matrix]() -> const void* {
		return &torus_model_matrix[0][0];
    };

    ShaderUniform torus_model = {"model", matrix_binder, torus_model_data};

    torus->shaders(object_vertex_shader, NULL, object_fragment_shader);
    torus->uniforms(torus_model, std_view, std_proj, std_light, std_view_position);
    torus->lights(directionalLights, pointLights, spotLights);
    torus->textures("/src/assets/textures/metal2.jpg", "/src/assets/textures/metal2_s.jpg");

    torus->setup();
	photo_objects.push_back(torus);
    // <<<Torus>>>

    // <<<Monkey>>>
    Object* monkey = new Object("Monkey Light");
    monkey->load("/src/assets/primitives/monkey.obj");

    glm::mat4 monkey_model_matrix = glm::mat4(1.0f);

    monkey_model_matrix = monkey->translate(monkey_model_matrix, glm::vec3(12.0f, 20.0f, 0.0f));
    monkey_model_matrix = monkey->rotate(monkey_model_matrix, -1.5708f, glm::vec3(0.0f, 1.0f, 0.0f));
    monkey_model_matrix = monkey->scale(monkey_model_matrix, glm::vec3(2.0f, 2.0f, 2.0f));

    auto monkey_model_data = [&monkey_model_matrix]() -> const void* {
		return &monkey_model_matrix[0][0];
    };

    ShaderUniform monkey_model = {"model", matrix_binder, monkey_model_data};

    monkey->shaders(object_vertex_shader, NULL, object_fragment_shader);
    monkey->uniforms(monkey_model, std_view, std_proj, std_light, std_view_position);
    monkey->lights(directionalLights, pointLights, spotLights);
    monkey->lightColor(glm::vec4(1.1f, 1.1f, 1.5f, 1.0f));
    monkey->textures("/src/assets/textures/black.jpg", "/src/assets/textures/wall_s.jpg");

    monkey->setup();
    // <<<Monkey>>>

    // <<<Cat>>>
    Object* cat = new Object("The Cat");
    cat->load("/src/assets/animals/cat/cat.obj");

    glm::mat4 cat_model_matrix = glm::mat4(1.0f);

    cat_model_matrix = cat->translate(cat_model_matrix, glm::vec3(1.0f, 1.0f, 12.0f));
    cat_model_matrix = cat->rotate(cat_model_matrix, 0.7854f, glm::vec3(0.0f, 1.0f, 0.0f));
    //cat_model_matrix = cat->scale(cat_model_matrix, glm::vec3(0.001f, 0.001f, 0.001f));

    auto cat_model_data = [&cat_model_matrix]() -> const void* {
		return &cat_model_matrix[0][0];
	};

    ShaderUniform cat_model = {"model", matrix_binder, cat_model_data};

    cat->shaders(object_vertex_shader, NULL, object_fragment_shader);
    cat->uniforms(cat_model, std_view, std_proj, std_light, std_view_position);
    cat->lights(directionalLights, pointLights, spotLights);
    cat->textures("/src/assets/textures/wood3.png", "/src/assets/textures/wall_s.jpg");

    cat->setup();
	photo_objects.push_back(cat);
    // <<<Cat>>>

    // <<<Dog>>>
    Object* dog = new Object("The Dog");
    dog->load("/src/assets/animals/dog/dog.obj");

    glm::mat4 dog_model_matrix = glm::mat4(1.0f);

    dog_model_matrix = dog->translate(dog_model_matrix, glm::vec3(0.0f, 1.0f, 6.0f));
    dog_model_matrix = dog->rotate(dog_model_matrix, -1.5708f, glm::vec3(1.0f, 0.0f, 0.0f));
    dog_model_matrix = dog->scale(dog_model_matrix, glm::vec3(0.05f, 0.05f, 0.05f));

    auto dog_model_data = [&dog_model_matrix]() -> const void* {
		return &dog_model_matrix[0][0];
	};

    ShaderUniform dog_model = {"model", matrix_binder, dog_model_data};

    dog->shaders(object_vertex_shader, NULL, object_fragment_shader);
    dog->uniforms(dog_model, std_view, std_proj, std_light, std_view_position);
    dog->lights(directionalLights, pointLights, spotLights);
    dog->textures("/src/assets/animals/dog/Dog_diffuse.jpg", "/src/assets/textures/wall_s.jpg");

    dog->setup();
	photo_objects.push_back(dog);
    // <<<Dog>>>

    // <<<Deer>>>
    Object* deer = new Object("The Deer");
    deer->load("/src/assets/animals/deer.obj");

    glm::mat4 deer_model_matrix = glm::mat4(1.0f);

    deer_model_matrix = deer->translate(deer_model_matrix, glm::vec3(-10.0f, 1.0f, -10.0f));
    //deer_model_matrix = deer->rotate(deer_model_matrix, -1.5708f, glm::vec3(1.0f, 0.0f, 0.0f));
    deer_model_matrix = deer->scale(deer_model_matrix, glm::vec3(0.001f, 0.001f, 0.001f));

    auto deer_model_data = [&deer_model_matrix]() -> const void* {
		return &deer_model_matrix[0][0];
	};

    ShaderUniform deer_model = {"model", matrix_binder, deer_model_data};

    deer->shaders(object_vertex_shader, NULL, object_fragment_shader);
    deer->uniforms(deer_model, std_view, std_proj, std_light, std_view_position);
    deer->lights(directionalLights, pointLights, spotLights);
    deer->textures("/src/assets/textures/metal.jpg", "/src/assets/textures/metal_s.jpg");

    deer->setup();
	photo_objects.push_back(deer);
    // <<<Deer>>>

    // <<<Building>>>
    Object* building = new Object("Large Building");
    building->load("/src/assets/buildings/flatiron/13943_Flatiron_Building_v1_l1.obj");

    glm::mat4 building_model_matrix = glm::mat4(1.0f);

    building_model_matrix = building->translate(building_model_matrix, glm::vec3(15.0f, 10.0f, 0.0f));
    building_model_matrix = building->rotate(building_model_matrix, -1.5708f, glm::vec3(1.0f, 0.0f, 0.0f));
    building_model_matrix = building->scale(building_model_matrix, glm::vec3(0.0025f, 0.0025f, 0.0025f));

    auto building_model_data = [&building_model_matrix]() -> const void* {
		return &building_model_matrix[0][0];
	};

    ShaderUniform building_model = {"model", matrix_binder, building_model_data};

    building->shaders(object_vertex_shader, NULL, object_fragment_shader);
    building->uniforms(building_model, std_view, std_proj, std_light, std_view_position);
    building->lights(directionalLights, pointLights, spotLights);
    building->textures("/src/assets/textures/concrete.png", "/src/assets/textures/wall_s.jpg");

    building->setup();
    // <<<Building>>>

    // <<<Grass>>>
    Object* grass = new Object("Grass");
    grass->load("/src/assets/primitives/cube.obj");

    glm::mat4 grass_model_matrix = glm::mat4(1.0f);

    //grass_model_matrix = grass->translate(grass_model_matrix, glm::vec3(0.0f, 0.0f, 0.0f));
    //grass_model_matrix = grass->rotate(grass_model_matrix, 0.0f, glm::vec3(0.0f, 0.0f, 0.0f));
    grass_model_matrix = grass->scale(grass_model_matrix, glm::vec3(20.0f, 1.0f, 20.0f));

    auto grass_model_data = [&grass_model_matrix]() -> const void* {
		return &grass_model_matrix[0][0];
    };

    ShaderUniform grass_model = {"model", matrix_binder, grass_model_data};

    grass->shaders(object_vertex_shader, NULL, object_fragment_shader);
    grass->uniforms(grass_model, std_view, std_proj, std_light, std_view_position);
    grass->lights(directionalLights, pointLights, spotLights);
    grass->textures("/src/assets/textures/grass.png", "/src/assets/textures/wall_s.jpg");

    grass->setup();
	photo_objects.push_back(grass);
    // <<<Grass>>>

    // <<<Wall>>>
    Object* wall = new Object("Wall");
    wall->load("/src/assets/primitives/cube.obj");

    glm::mat4 wall_model_matrix = glm::mat4(1.0f);

    wall_model_matrix = wall->translate(wall_model_matrix, glm::vec3(0.0f, 0.0f, 20.0f));
    //wall_model_matrix = wall->rotate(wall_model_matrix, 0.0f, glm::vec3(0.0f, 0.0f, 0.0f));
    wall_model_matrix = wall->scale(wall_model_matrix, glm::vec3(20.0f, 4.0f, 1.0f));

    auto wall_model_data = [&wall_model_matrix]() -> const void* {
		return &wall_model_matrix[0][0];
    };

    ShaderUniform wall_model = {"model", matrix_binder, wall_model_data};

    wall->shaders(object_vertex_shader, NULL, object_fragment_shader);
    wall->uniforms(wall_model, std_view, std_proj, std_light, std_view_position);
    wall->lights(directionalLights, pointLights, spotLights);
    wall->textures("/src/assets/textures/wall.png", "/src/assets/textures/wall_s.jpg");

    wall->setup();
    // <<<Wall>>>

    // <<<Wall2>>>
    Object* wall2 = new Object("Wall 2");
    wall2->load("/src/assets/primitives/cube.obj");

    glm::mat4 wall2_model_matrix = glm::mat4(1.0f);

    wall2_model_matrix = wall2->translate(wall2_model_matrix, glm::vec3(0.0f, 0.0f, -20.0f));
    //wall2_model_matrix = wall2->rotate(wall2_model_matrix, 0.0f, glm::vec3(0.0f, 0.0f, 0.0f));
    wall2_model_matrix = wall2->scale(wall2_model_matrix, glm::vec3(20.0f, 4.0f, 1.0f));

    auto wall2_model_data = [&wall2_model_matrix]() -> const void* {
		return &wall2_model_matrix[0][0];
    };

    ShaderUniform wall2_model = {"model", matrix_binder, wall2_model_data};

    wall2->shaders(object_vertex_shader, NULL, object_fragment_shader);
    wall2->uniforms(wall2_model, std_view, std_proj, std_light, std_view_position);
    wall2->lights(directionalLights, pointLights, spotLights);
    wall2->textures("/src/assets/textures/wall.png", "/src/assets/textures/wall_s.jpg");

    wall2->setup();
    // <<<Wall2>>>

    // <<<Wall3>>>
    Object* wall3 = new Object("Wall 3");
    wall3->load("/src/assets/primitives/cube.obj");

    glm::mat4 wall3_model_matrix = glm::mat4(1.0f);

    wall3_model_matrix = wall3->translate(wall3_model_matrix, glm::vec3(20.0f, 0.0f, 0.0f));
    //wall3_model_matrix = wall3->rotate(wall3_model_matrix, 0.0f, glm::vec3(0.0f, 0.0f, 0.0f));
    wall3_model_matrix = wall3->scale(wall3_model_matrix, glm::vec3(1.0f, 4.0f, 20.0f));

    auto wall3_model_data = [&wall3_model_matrix]() -> const void* {
		return &wall3_model_matrix[0][0];
    };

    ShaderUniform wall3_model = {"model", matrix_binder, wall3_model_data};

    wall3->shaders(object_vertex_shader, NULL, object_fragment_shader);
    wall3->uniforms(wall3_model, std_view, std_proj, std_light, std_view_position);
    wall3->lights(directionalLights, pointLights, spotLights);
    wall3->textures("/src/assets/textures/wall.png", "/src/assets/textures/wall_s.jpg");

    wall3->setup();
    // <<<Wall3>>>

    // <<<Wall4>>>
    Object* wall4 = new Object("Wall 4");
    wall4->load("/src/assets/primitives/cube.obj");

    glm::mat4 wall4_model_matrix = glm::mat4(1.0f);

    wall4_model_matrix = wall4->translate(wall4_model_matrix, glm::vec3(-20.0f, 0.0f, 0.0f));
    //wall4_model_matrix = wall4->rotate(wall4_model_matrix, 0.0f, glm::vec3(0.0f, 0.0f, 0.0f));
    wall4_model_matrix = wall4->scale(wall4_model_matrix, glm::vec3(1.0f, 4.0f, 20.0f));

    auto wall4_model_data = [&wall4_model_matrix]() -> const void* {
		return &wall4_model_matrix[0][0];
    };

    ShaderUniform wall4_model = {"model", matrix_binder, wall4_model_data};

    wall4->shaders(object_vertex_shader, NULL, object_fragment_shader);
    wall4->uniforms(wall4_model, std_view, std_proj, std_light, std_view_position);
    wall4->lights(directionalLights, pointLights, spotLights);
    wall4->textures("/src/assets/textures/wall.png", "/src/assets/textures/wall_s.jpg");

    wall4->setup();
    // <<<Wall4>>>
    // <<<Scene>>>

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

	// configure alternate id_framebuffer
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
	// configure MSAA framebuffer
  // --------------------------
  unsigned int msaa_framebuffer;
  glGenFramebuffers(1, &msaa_framebuffer);
  glBindFramebuffer(GL_FRAMEBUFFER, msaa_framebuffer);
  // create a multisampled color attachment texture
  unsigned int textureColorBufferMultiSampled;
  glGenTextures(1, &textureColorBufferMultiSampled);
  glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, textureColorBufferMultiSampled);
  glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 4, GL_RGBA, window_width, window_height, GL_TRUE);
  glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, textureColorBufferMultiSampled, 0);
  // create a (also multisampled) renderbuffer object for depth and stencil attachments
  unsigned int msaa_rbo;
  glGenRenderbuffers(1, &msaa_rbo);
  glBindRenderbuffer(GL_RENDERBUFFER, msaa_rbo);
  glRenderbufferStorageMultisample(GL_RENDERBUFFER, 4, GL_DEPTH24_STENCIL8, window_width, window_height);
  glBindRenderbuffer(GL_RENDERBUFFER, 0);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, msaa_rbo);

  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
      std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// Regular geometry buffer ====================================
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

	choose_photo_object();

	clock_t last_frame_time = clock();
	while (!glfwWindowShouldClose(window)) {
		// Compute the projection matrix.
		aspect = static_cast<float>(window_width) / window_height;
		projection_matrix =
			glm::perspective(glm::radians(45.0f), aspect, 0.0001f, 1000.0f);

    // Calculate for depth of field pass
    // FIXME: may be able to just use camera variables here instead of recalculating
    glm::vec3 right = glm::normalize(glm::cross(g_camera->up_, g_camera->center_ - g_camera->eye_));
    glm::vec3 p_up = glm::normalize(glm::cross(g_camera->center_ - g_camera->eye_, right));

		// render
		// ------
		// bind to geometry_framebuffer and draw scene as we normally would to color texture
		glBindFramebuffer(GL_FRAMEBUFFER, msaa_framebuffer);
		glEnable(GL_DEPTH_TEST); // enable depth testing (is disabled for rendering screen-space quad)

		// make sure we clear the geometry_framebuffer's content
		glClearColor(0.3f, 0.5f, 0.8f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		//glClear(GL_ACCUM_BUFFER_BIT);

    for(int i = 0; i < light_rays_for_bokeh; i++) {
      glm::vec3 bokeh = right * cosf(i * 2 * M_PI / light_rays_for_bokeh) + p_up * sinf(i * 2 * M_PI / light_rays_for_bokeh);
      // TODO: Switch back to using our custom get_view_matrix function
  		view_matrix = glm::lookAt(g_camera->eye_ + aperture * bokeh, g_camera->center_, p_up);

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
  		menger_pass.loadLightColor(glm::vec4(5.0f, 1.5f, 1.5f, 1.0f));

  		menger_pass.setup();
  		CHECK_GL_ERROR(glDrawElements(GL_TRIANGLES, menger_faces.size() * 3, GL_UNSIGNED_INT, 0));
  		// <<<Render Menger>>>

  		// <<<Render Floor>>>
  		floor_pass.setup();
  		CHECK_GL_ERROR(glDrawElements(GL_TRIANGLES, floor_faces.size() * 3, GL_UNSIGNED_INT, 0));
  		// <<<Render Floor>>>

  				// <<<Scene>>>
  				if (showMeshes){
  					//tree lights
  					treelight->render();
  					treelight2->render();
  					treelight3->render();
  					treelight4->render();
  					treelight5->render();
  					treelight6->render();
  					treelight7->render();
  					treelight8->render();
  					treelight9->render();
  					treelight10->render();
  					// ==========
  					cone->render();
  					sphere->render();
  					sphere2->render();
  					cylinder->render();
  					torus->render();
  					monkey->render();

  					cat->render();
  					dog->render();
  					deer->render();

  					building->render();
  					grass->render();
  					wall->render();
  					wall2->render();
  					wall3->render();
  					wall4->render();
  				}
  				// <<<Scene>>>
  		// End of geometry pass ====================================================
      //glAccum(GL_ACCUM, 0.25);
    }
		// 2. now blit multisampled buffer(s) to normal colorbuffer of intermediate FBO. Image is stored in screenTexture
    glBindFramebuffer(GL_READ_FRAMEBUFFER, msaa_framebuffer);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, geometry_framebuffer);
    glBlitFramebuffer(0, 0, window_width, window_height, 0, 0, window_width, window_height, GL_COLOR_BUFFER_BIT, GL_NEAREST);


		//glAccum(GL_RETURN, 1);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

		if (drunkMode){
			// Use our post processing programs ========================================
			// post lensflair blur pass
			glBindFramebuffer(GL_FRAMEBUFFER, blur_framebuffer);
			glDisable(GL_DEPTH_TEST); // disable depth test so screen-space quad isn't discarded due to depth test.
			// clear all relevant buffers
			glClearColor(1.0f, 1.0f, 1.0f, 1.0f); // set clear color to white (not really necessery actually, since we won't be able to see behind the quad anyways)
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			CHECK_GL_ERROR(glUseProgram(screen_blur_program_id));
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, geometry_textureColorBuffer);	// use the color attachment texture as the texture of the quad plane

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
			glBindTexture(GL_TEXTURE_2D, blur_textureColorBuffer);	// use the color attachment texture as the texture of the quad plane

			glBindVertexArray(quadVAO);
			glDrawArrays(GL_TRIANGLES, 0, 6);
		} else {
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
		}

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
		glClearColor(1.0f, 1.0f, 1.0f, 1.0f); // set clear color to white (not really necessery actually, since we won't be able to see behind the quad anyways)
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

		if (captureImage){
      //  color id pass
    	glBindFramebuffer(GL_FRAMEBUFFER, id_tracking_framebuffer);
  		glEnable(GL_DEPTH_TEST); // enable depth testing (is disabled for rendering screen-space quad)

  		// make sure we clear the id_tracking_framebuffer's content
  		glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
  		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

			// every object with an id needs to be rendered here
			if (showMeshes){
				//tree lights
				treelight->render_id();
				treelight2->render_id();
				treelight3->render_id();
				treelight4->render_id();
				treelight5->render_id();
				treelight6->render_id();
				treelight7->render_id();
				treelight8->render_id();
				treelight9->render_id();
				treelight10->render_id();
				// ==========
				cone->render_id();
				sphere->render_id();
				sphere2->render_id();
				cylinder->render_id();
				torus->render_id();
				monkey->render_id();

				cat->render_id();
				dog->render_id();
				deer->render_id();

				building->render_id();
				grass->render_id();
				wall->render_id();
				wall2->render_id();
				wall3->render_id();
				wall4->render_id();
			}

      glBindFramebuffer(GL_FRAMEBUFFER, 0);
	    glDisable(GL_DEPTH_TEST); // disable depth test so screen-space quad isn't discarded due to depth test.
	    // clear all relevant buffers
	    glClearColor(1.0f, 1.0f, 1.0f, 1.0f); // set clear color to white (not really necessery actually, since we won't be able to see behind the quad anyways)
	    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			// draw to the quad:
			CHECK_GL_ERROR(glUseProgram(screen_single_program_id));
			glActiveTexture(GL_TEXTURE0);
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
	  if ((pickedID / 3) == goal_id) {
		  score += 5;
		  choose_photo_object();
	  }

      captureImage = false;
			// end of color id pass ===========================================================
    }

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
