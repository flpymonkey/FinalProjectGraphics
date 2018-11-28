#include "menger.h"
#include "stdio.h"

namespace {
	const int kMinLevel = 0;
	const int kMaxLevel = 4;
};

Menger::Menger()
{
	// Add additional initialization if you like
}

Menger::~Menger()
{
}

void
Menger::set_nesting_level(int level)
{
	nesting_level_ = level;
	dirty_ = true;
}

bool
Menger::is_dirty() const
{
	return dirty_;
}

void
Menger::set_clean()
{
	dirty_ = false;
}

// FIXME generate Menger sponge geometry
void
Menger::generate_geometry(std::vector<glm::vec4>& obj_vertices,
	std::vector<glm::vec4>& vtx_normals,
  std::vector<glm::uvec3>& obj_faces,
  glm::vec4 position) const
{
    obj_vertices.clear();
    vtx_normals.clear();
    obj_faces.clear();

	float length = 1.0f;

	if (nesting_level_ <= 0) {
		create_cube(obj_vertices, vtx_normals, obj_faces, position, length, 0);
	} else {
		create_menger(obj_vertices, vtx_normals, obj_faces, position, length / 3.0f, 1, 0);
	}
}

int
Menger::create_menger(std::vector<glm::vec4>& obj_vertices,
	std::vector<glm::vec4>& vtx_normals, std::vector<glm::uvec3>& obj_faces,
	glm::vec4 position, float length, int level, int obj_faces_i) const
{
	for (int z = 0; z < 3; z++) {
		for (int y = 0; y < 3; y++) {
			for (int x = 0; x < 3; x++) {
				if (x % 2 + y % 2 + z % 2 < 2) {
					glm::vec4 position_i = glm::vec4(-length + x * length + position.x,
						-length + y * length + position.y, -length + z * length + position.z, 1.0f);
					if (level < nesting_level_) {
						obj_faces_i = create_menger(obj_vertices, vtx_normals, obj_faces,
							position_i, length / 3.0f, level + 1, obj_faces_i);
					} else {
						create_cube(obj_vertices, vtx_normals, obj_faces, position_i, length, obj_faces_i);
						obj_faces_i += 36;
					}
				}
			}
		}
	}
	return obj_faces_i;
}

void
Menger::create_cube(std::vector<glm::vec4>& obj_vertices, std::vector<glm::vec4>& vtx_normals,
	std::vector<glm::uvec3>& obj_faces, glm::vec4 position, float length, int obj_faces_i) const
{
	// Calculate half lengths based on position.
	float half_length = length * 0.5f;
	float minx = -half_length + position.x;
    float maxx = half_length + position.x;
	float miny = -half_length + position.y;
	float maxy = half_length + position.y;
	float minz = -half_length + position.z;
	float maxz = half_length + position.z;

	// Cube data.
	// Front, bottom-right triangle.
	obj_vertices.push_back(glm::vec4(minx, miny, maxz, 1.0f));
	vtx_normals.push_back(glm::vec4(0.0f, 0.0f, 1.0f, 0.0f));
	obj_vertices.push_back(glm::vec4(maxx, miny, maxz, 1.0f));
	vtx_normals.push_back(glm::vec4(0.0f, 0.0f, 1.0f, 0.0f));
	obj_vertices.push_back(glm::vec4(maxx, maxy, maxz, 1.0f));
	vtx_normals.push_back(glm::vec4(0.0f, 0.0f, 1.0f, 0.0f));
	obj_faces.push_back(glm::uvec3(obj_faces_i + 0, obj_faces_i + 1, obj_faces_i + 2));

	// Front, top-left triangle.
	obj_vertices.push_back(glm::vec4(minx, miny, maxz, 1.0f));
	vtx_normals.push_back(glm::vec4(0.0f, 0.0f, 1.0f, 0.0f));
	obj_vertices.push_back(glm::vec4(maxx, maxy, maxz, 1.0f));
	vtx_normals.push_back(glm::vec4(0.0f, 0.0f, 1.0f, 0.0f));
	obj_vertices.push_back(glm::vec4(minx, maxy, maxz, 1.0f));
	vtx_normals.push_back(glm::vec4(0.0f, 0.0f, 1.0f, 0.0f));
	obj_faces.push_back(glm::uvec3(obj_faces_i + 3, obj_faces_i + 4, obj_faces_i + 5));

	// Right, bottom-right triangle.
	obj_vertices.push_back(glm::vec4(maxx, miny, maxz, 1.0f));
	vtx_normals.push_back(glm::vec4(1.0f, 0.0f, 0.0f, 0.0f));
	obj_vertices.push_back(glm::vec4(maxx, miny, minz, 1.0f));
	vtx_normals.push_back(glm::vec4(1.0f, 0.0f, 0.0f, 0.0f));
	obj_vertices.push_back(glm::vec4(maxx, maxy, minz, 1.0f));
	vtx_normals.push_back(glm::vec4(1.0f, 0.0f, 0.0f, 0.0f));
	obj_faces.push_back(glm::uvec3(obj_faces_i + 6, obj_faces_i + 7, obj_faces_i + 8));

	// Right, top-left triangle.
	obj_vertices.push_back(glm::vec4(maxx, miny, maxz, 1.0f));
	vtx_normals.push_back(glm::vec4(1.0f, 0.0f, 0.0f, 0.0f));
	obj_vertices.push_back(glm::vec4(maxx, maxy, minz, 1.0f));
	vtx_normals.push_back(glm::vec4(1.0f, 0.0f, 0.0f, 0.0f));
	obj_vertices.push_back(glm::vec4(maxx, maxy, maxz, 1.0f));
	vtx_normals.push_back(glm::vec4(1.0f, 0.0f, 0.0f, 0.0f));
	obj_faces.push_back(glm::uvec3(obj_faces_i + 9, obj_faces_i + 10, obj_faces_i + 11));

	// Top, bottom-right triangle.
	obj_vertices.push_back(glm::vec4(maxx, maxy, maxz, 1.0f));
	vtx_normals.push_back(glm::vec4(0.0f, 1.0f, 0.0f, 0.0f));
	obj_vertices.push_back(glm::vec4(maxx, maxy, minz, 1.0f));
	vtx_normals.push_back(glm::vec4(0.0f, 1.0f, 0.0f, 0.0f));
	obj_vertices.push_back(glm::vec4(minx, maxy, minz, 1.0f));
	vtx_normals.push_back(glm::vec4(0.0f, 1.0f, 0.0f, 0.0f));
	obj_faces.push_back(glm::uvec3(obj_faces_i + 12, obj_faces_i + 13, obj_faces_i + 14));

	// Top, top-left triangle.
	obj_vertices.push_back(glm::vec4(maxx, maxy, maxz, 1.0f));
	vtx_normals.push_back(glm::vec4(0.0f, 1.0f, 0.0f, 0.0f));
	obj_vertices.push_back(glm::vec4(minx, maxy, minz, 1.0f));
	vtx_normals.push_back(glm::vec4(0.0f, 1.0f, 0.0f, 0.0f));
	obj_vertices.push_back(glm::vec4(minx, maxy, maxz, 1.0f));
	vtx_normals.push_back(glm::vec4(0.0f, 1.0f, 0.0f, 0.0f));
	obj_faces.push_back(glm::uvec3(obj_faces_i + 15, obj_faces_i + 16, obj_faces_i + 17));

	// Bottom, bottom-right triangle.
	obj_vertices.push_back(glm::vec4(minx, miny, maxz, 1.0f));
	vtx_normals.push_back(glm::vec4(0.0f, -1.0f, 0.0f, 0.0f));
	obj_vertices.push_back(glm::vec4(minx, miny, minz, 1.0f));
	vtx_normals.push_back(glm::vec4(0.0f, -1.0f, 0.0f, 0.0f));
	obj_vertices.push_back(glm::vec4(maxx, miny, minz, 1.0f));
	vtx_normals.push_back(glm::vec4(0.0f, -1.0f, 0.0f, 0.0f));
	obj_faces.push_back(glm::uvec3(obj_faces_i + 18, obj_faces_i + 19, obj_faces_i + 20));

	// Bottom, top-left triangle.
	obj_vertices.push_back(glm::vec4(minx, miny, maxz, 1.0f));
	vtx_normals.push_back(glm::vec4(0.0f, -1.0f, 0.0f, 0.0f));
	obj_vertices.push_back(glm::vec4(maxx, miny, minz, 1.0f));
	vtx_normals.push_back(glm::vec4(0.0f, -1.0f, 0.0f, 0.0f));
	obj_vertices.push_back(glm::vec4(maxx, miny, maxz, 1.0f));
	vtx_normals.push_back(glm::vec4(0.0f, -1.0f, 0.0f, 0.0f));
	obj_faces.push_back(glm::uvec3(obj_faces_i + 21, obj_faces_i + 22, obj_faces_i + 23));

	// Back, bottom-right triangle.
	obj_vertices.push_back(glm::vec4(maxx, miny, minz, 1.0f));
	vtx_normals.push_back(glm::vec4(0.0f, 0.0f, -1.0f, 0.0f));
	obj_vertices.push_back(glm::vec4(minx, miny, minz, 1.0f));
	vtx_normals.push_back(glm::vec4(0.0f, 0.0f, -1.0f, 0.0f));
	obj_vertices.push_back(glm::vec4(minx, maxy, minz, 1.0f));
	vtx_normals.push_back(glm::vec4(0.0f, 0.0f, -1.0f, 0.0f));
	obj_faces.push_back(glm::uvec3(obj_faces_i + 24, obj_faces_i + 25, obj_faces_i + 26));

	// Back, top-left triangle.
	obj_vertices.push_back(glm::vec4(maxx, miny, minz, 1.0f));
	vtx_normals.push_back(glm::vec4(0.0f, 0.0f, -1.0f, 0.0f));
	obj_vertices.push_back(glm::vec4(minx, maxy, minz, 1.0f));
	vtx_normals.push_back(glm::vec4(0.0f, 0.0f, -1.0f, 0.0f));
	obj_vertices.push_back(glm::vec4(maxx, maxy, minz, 1.0f));
	vtx_normals.push_back(glm::vec4(0.0f, 0.0f, -1.0f, 0.0f));
	obj_faces.push_back(glm::uvec3(obj_faces_i + 27, obj_faces_i + 28, obj_faces_i + 29));

	// Left, bottom-right triangle.
	obj_vertices.push_back(glm::vec4(minx, miny, minz, 1.0f));
	vtx_normals.push_back(glm::vec4(-1.0f, 0.0f, 0.0f, 0.0f));
	obj_vertices.push_back(glm::vec4(minx, miny, maxz, 1.0f));
	vtx_normals.push_back(glm::vec4(-1.0f, 0.0f, 0.0f, 0.0f));
	obj_vertices.push_back(glm::vec4(minx, maxy, maxz, 1.0f));
	vtx_normals.push_back(glm::vec4(-1.0f, 0.0f, 0.0f, 0.0f));
	obj_faces.push_back(glm::uvec3(obj_faces_i + 30, obj_faces_i + 31, obj_faces_i + 32));

	// Left, top-left triangle.
	obj_vertices.push_back(glm::vec4(minx, miny, minz, 1.0f));
	vtx_normals.push_back(glm::vec4(-1.0f, 0.0f, 0.0f, 0.0f));
	obj_vertices.push_back(glm::vec4(minx, maxy, maxz, 1.0f));
	vtx_normals.push_back(glm::vec4(-1.0f, 0.0f, 0.0f, 0.0f));
	obj_vertices.push_back(glm::vec4(minx, maxy, minz, 1.0f));
	vtx_normals.push_back(glm::vec4(-1.0f, 0.0f, 0.0f, 0.0f));
	obj_faces.push_back(glm::uvec3(obj_faces_i + 33, obj_faces_i + 34, obj_faces_i + 35));
}
