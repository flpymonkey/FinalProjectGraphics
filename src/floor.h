#ifndef FLOOR_H
#define FLOOR_H

#include <glm/glm.hpp>
#include <vector>

class Floor {
public:
	Floor();
	~Floor();

	void create_floor(std::vector<glm::vec4>& floor_obj_vertices,
    	std::vector<glm::vec4>& floor_vtx_normals,
    	std::vector<glm::uvec3>& floor_obj_faces) const;

private:
	
};

#endif