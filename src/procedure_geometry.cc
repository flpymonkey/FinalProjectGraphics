#include "procedure_geometry.h"
#include "bone_geometry.h"
#include "config.h"
// #include <GL/glew.h>

void create_floor(std::vector<glm::vec4>& floor_vertices, std::vector<glm::uvec3>& floor_faces)
{
	floor_vertices.push_back(glm::vec4(kFloorXMin, kFloorY, kFloorZMax, 1.0f));
	floor_vertices.push_back(glm::vec4(kFloorXMax, kFloorY, kFloorZMax, 1.0f));
	floor_vertices.push_back(glm::vec4(kFloorXMax, kFloorY, kFloorZMin, 1.0f));
	floor_vertices.push_back(glm::vec4(kFloorXMin, kFloorY, kFloorZMin, 1.0f));
	floor_faces.push_back(glm::uvec3(0, 1, 2));
	floor_faces.push_back(glm::uvec3(2, 3, 0));
}

// FIXME: create cylinders and lines for the bones
// Hints: Generate a lattice in [-0.5, 0, 0] x [0.5, 1, 0] We wrap this
// around in the vertex shader to produce a very smooth cylinder.  We only
// need to send a small number of points.  Controlling the grid size gives a
// nice wireframe.

void create_circle(glm::vec3 position, std::vector<glm::vec4>& vertices, std::vector<glm::uvec2>& faces, int& face_count)
{
	 int num_verts = 360;
   for (int i=0; i < num_verts; i++)
   {
      //float degInRad = i*DEG2RAD;

			float theta = 2.0f * 3.1414926f * float(i) / (num_verts / 4);
      glm::vec4 v = glm::vec4(cosf(theta)*kCylinderRadius + position.x, sinf(theta)*kCylinderRadius + position.y, position.z, 1.0f);
			printf("pointx%f\n", v.x);
			printf("pointy%f\n", v.y);
			vertices.push_back(v);

			if (i != 0 || face_count != 0){
				faces.push_back(glm::uvec2(face_count - 1, face_count));
			}
			face_count++;
   }
}

void create_cylinder(std::vector<glm::vec4>& vertices, std::vector<glm::uvec2>& faces)
{
	int face_count = 0;
	create_circle(glm::vec3(0.0f, 1.0f, 0.0f), vertices, faces, face_count);
	printf("%d\n", face_count);
	create_circle(glm::vec3(0.0f, 0.0f, 0.0f), vertices, faces, face_count);
	printf("%d\n", face_count);
	printf("%d\n", vertices.size());
}
