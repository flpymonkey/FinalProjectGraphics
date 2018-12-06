#include "object.h"

const char* picker_fragment_shader =
#include "shaders/picker.frag"
;

const char* picker_vertex_shader =
#include "shaders/picker.vert"
;

Object::Object() {
    loader = new Loader();
    object_id = object_count++;
}

Object::~Object() {
    // TODO:
}

void Object::load(std::string file) {
    loader->loadObj(path(file).c_str(), meshes, materials);
}

void Object::shaders(
    const char* vertex_shader,
    const char* geometry_shader,
    const char* fragment_shader
    ) {

    this->vertex_shader = vertex_shader;
    this->geometry_shader = geometry_shader;
    this->fragment_shader = fragment_shader;
}

void Object::uniforms(
    ShaderUniform std_model,
    ShaderUniform std_view,
    ShaderUniform std_projection,
    ShaderUniform std_light,
    ShaderUniform std_view_position
    ) {

    this->std_model = std_model;
    this->std_view = std_view;
    this->std_projection = std_projection;
    this->std_light = std_light;
    this->std_view_position = std_view_position;
}

void Object::lights(
    std::vector<DirectionalLight> directionalLights,
    std::vector<PointLight> pointLights,
    std::vector<SpotLight> spotLights
    ) {

    this->directionalLights = directionalLights;
    this->pointLights = pointLights;
    this->spotLights = spotLights;
}

glm::mat4 Object::translate(glm::mat4 model_matrix, glm::vec3 t) {
    return glm::translate(model_matrix, t);
}

glm::mat4 Object::rotate(glm::mat4 model_matrix, float degrees, glm::vec3 axis) {
    return glm::rotate(model_matrix, degrees, axis);
}

glm::mat4 Object::scale(glm::mat4 model_matrix, glm::vec3 s) {
    return glm::scale(model_matrix, s);
}

void Object::setup(unsigned int i) {
    //if (materials.size() == 0) {
        //diffuseMap = loader->loadTexture(path("/src/assets/container2.png").c_str());
        specularMap = loader->loadTexture(path("/src/assets/container2_specular.png").c_str());
    //} else {
        diffuseMap = materials[0].diffuse_ids[0];
        //specularMap = materials[0].specular_ids[0];
    //}

    // Create the ShaderUniform for this object_id
    auto vector_binder = [](int loc, const void* data) {
    	glUniform4fv(loc, 1, (const GLfloat*)data);
    };
    printf("iddddddddddddd%d\n", object_id);
    int r = (object_id & 0x000000FF) >>  0;
    int g = (object_id & 0x0000FF00) >>  8;
    int b = (object_id & 0x00FF0000) >> 16;
    glm::vec4 color_id = glm::vec4(r/255.0f, g/255.0f, b/255.0f, 1.0f);
    color_id = glm::vec4(120.0f, 120.0f, 120.0f, 1.0f);
    auto std_color_id_data = [&color_id]() -> const void* {
  		return &color_id[0];
  	};
    ShaderUniform color_id_uniform = { "PickingColor", vector_binder, std_color_id_data };

    RenderDataInput id_pass_input;
    id_pass_input.assign(0, "vertex_position", meshes[i].vertices.data(), meshes[i].vertices.size(), 4, GL_FLOAT);
    id_pass_input.assign(1, "normal", meshes[i].normals.data(), meshes[i].normals.size(), 4, GL_FLOAT);
    id_pass_input.assign(2, "uv", meshes[i].uvs.data(), meshes[i].uvs.size(), 2, GL_FLOAT);
    id_pass_input.assign_index(meshes[i].faces.data(), meshes[i].faces.size(), 3);

    id_pass = new RenderPass(
        -1,
        id_pass_input,
        {picker_vertex_shader, geometry_shader, picker_fragment_shader},
        {std_model, std_view, std_projection, std_light, std_view_position, color_id_uniform},
        {"fragment_color"}
    );

    RenderDataInput model_pass_input;
    model_pass_input.assign(0, "vertex_position", meshes[i].vertices.data(), meshes[i].vertices.size(), 4, GL_FLOAT);
    model_pass_input.assign(1, "normal", meshes[i].normals.data(), meshes[i].normals.size(), 4, GL_FLOAT);
    model_pass_input.assign(2, "uv", meshes[i].uvs.data(), meshes[i].uvs.size(), 2, GL_FLOAT);
    model_pass_input.assign_index(meshes[i].faces.data(), meshes[i].faces.size(), 3);

    model_pass = new RenderPass(
        -1,
        model_pass_input,
        {vertex_shader, geometry_shader, fragment_shader},
        {std_model, std_view, std_projection, std_light, std_view_position},
        {"fragment_color"}
    );

    model_pass->loadLights(directionalLights, pointLights, spotLights);
    model_pass->loadMaterials();
}

void Object::update() {

}

void Object::render(unsigned int i) {
    model_pass->setup();

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, diffuseMap);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, specularMap);

	  CHECK_GL_ERROR(glDrawElements(GL_TRIANGLES, meshes[i].faces.size() * 3, GL_UNSIGNED_INT, 0));
}

void Object::render_id(unsigned int i) {
    id_pass->setup();

	  CHECK_GL_ERROR(glDrawElements(GL_TRIANGLES, meshes[i].faces.size() * 3, GL_UNSIGNED_INT, 0));
}
