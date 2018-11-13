#include "floor.h"

Floor::Floor() {}

Floor::~Floor() {}

void
Floor::create_floor(std::vector<glm::vec4>& floor_obj_vertices,
    std::vector<glm::vec4>& floor_vtx_normals,
    std::vector<glm::uvec3>& floor_obj_faces) const
{
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