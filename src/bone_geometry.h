#ifndef BONE_GEOMETRY_H
#define BONE_GEOMETRY_H

#include <ostream>
#include <vector>
#include <map>
#include <limits>
#include <glm/glm.hpp>
#include <mmdadapter.h>

struct BoundingBox {
	BoundingBox()
		: min(glm::vec3(-std::numeric_limits<float>::max())),
		max(glm::vec3(std::numeric_limits<float>::max())) {}
	glm::vec3 min;
	glm::vec3 max;
};

struct Joint {
	// FIXME: Implement your Joint data structure.
	// Note: PMD represents weights on joints, but you need weights on
	//       bones to calculate the actual animation.
};

struct Bone {
	std::string name;
	int id;
	int parent_id;
	std::vector<Bone*> children;
	Bone* parent;
	float length;
	glm::mat4 orientation;
	glm::mat4 LocalToWorld;

	//Trmporary
	glm::vec3 offset;
	glm::mat4 T;
	glm::mat4 R;
	
};


struct Skeleton {
	std::vector<Bone*> bones;
	Bone* root;
};

struct Mesh {
	Mesh();
	~Mesh();
	std::vector<glm::vec4> vertices;
	std::vector<glm::vec4> animated_vertices;
	std::vector<glm::uvec3> faces;
	std::vector<glm::vec4> vertex_normals;
	std::vector<glm::vec4> face_normals;
	std::vector<glm::vec2> uv_coordinates;
	std::vector<Material> materials;
	BoundingBox bounds;
	Skeleton skeleton;

	void loadpmd(const std::string& fn);
	void updateAnimation();
	int getNumberOfBones() const
	{
		return skeleton.bones.size();
	}
	glm::vec3 getCenter() const { return 0.5f * glm::vec3(bounds.min + bounds.max); }

	// Added mesh functions:
	void getSkeletonJointsVec(std::vector<glm::vec4>& skeleton_vertices, std::vector<glm::uvec2>& skeleton_faces);

private:
	void computeBounds();
	glm::vec3 computeNormal(glm::vec3 tangent);
	glm::mat4 computeOrientation(glm::vec3 tangent, glm::vec3 normal, glm::vec3 binormal);

	void printInt(char* name, int data);
	void printFloat(char* name, float value);
	void printVec3(char* name, glm::vec3 data);
	void printVec4(char* name, glm::vec4 data);
	void printMat4(char* name, glm::mat4 data);
};

#endif
