#ifndef OBJECT_H
#define OBJECT_H

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

#include "filesystem.h"
#include "loader.h"
#include "mesh.h"
#include "material.h"
#include "render_pass.h"
#include "lights.h"

class Object {
  // the number of objects generated (total)
  static int object_count;
public:
    int object_id;
    std::vector<Mesh> meshes;
    std::vector<Material> materials;

    Object();
    ~Object();

    void load(std::string file);
    void shaders(
        const char* vertex_shader,
        const char* geometry_shader,
        const char* fragment_shader
    );
    void uniforms(
        ShaderUniform std_model,
        ShaderUniform std_view,
        ShaderUniform std_projection,
        ShaderUniform std_light,
        ShaderUniform std_view_position
    );
    void lights(
        std::vector<DirectionalLight> directionalLights,
        std::vector<PointLight> pointLights,
        std::vector<SpotLight> spotLights
    );

    glm::mat4 translate(glm::mat4 model_matrix, glm::vec3 t);
    glm::mat4 rotate(glm::mat4 model_matrix, float degrees, glm::vec3 axis);
    glm::mat4 scale(glm::mat4 model_matrix, glm::vec3 s);

    void setup(unsigned int i);
    void update();
    void render(unsigned int i);

private:
    Loader* loader;

    const char* vertex_shader;
    const char* geometry_shader;
    const char* fragment_shader;

    ShaderUniform std_model;
	ShaderUniform std_view;
	ShaderUniform std_projection;

	ShaderUniform std_light; // TODO: Not sure if we need this.
	ShaderUniform std_view_position; // TODO: rename to camera.

    std::vector<DirectionalLight> directionalLights;
	std::vector<PointLight> pointLights;
	std::vector<SpotLight> spotLights;

    unsigned int diffuseMap;
    unsigned int specularMap;

    RenderPass* model_pass;

    bool initialized;
};

#endif
