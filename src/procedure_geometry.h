#ifndef PROCEDURE_GEOMETRY_H
#define PROCEDURE_GEOMETRY_H

#include <vector>
#include <glm/glm.hpp>

class LineMesh;

// Conversion from degrees to radians
const float DEG2RAD = 3.14159/180;

void create_floor(std::vector<glm::vec4>& floor_vertices, std::vector<glm::uvec3>& floor_faces);
// FIXME: Add functions to generate the bone mesh.
void create_circle(std::vector<glm::vec4>& vertices, std::vector<glm::uvec2>& faces);

#endif
