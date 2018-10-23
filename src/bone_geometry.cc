#include "config.h"
#include "bone_geometry.h"
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <glm/gtx/io.hpp>
#include <glm/gtx/transform.hpp>

/*
 * For debugging purpose.
 */
template <typename T>
std::ostream& operator<<(std::ostream& os, const std::vector<T>& v) {
	size_t count = std::min(v.size(), static_cast<size_t>(10));
	for (size_t i = 0; i < count; ++i) os << i << " " << v[i] << "\n";
	os << "size = " << v.size() << "\n";
	return os;
}

std::ostream& operator<<(std::ostream& os, const BoundingBox& bounds)
{
	os << "min = " << bounds.min << " max = " << bounds.max;
	return os;
}



// FIXME: Implement bone animation.


Mesh::Mesh()
{
}

Mesh::~Mesh()
{
}

void Mesh::loadpmd(const std::string& fn)
{
	MMDReader mr;
	mr.open(fn);
	mr.getMesh(vertices, faces, vertex_normals, uv_coordinates);
	computeBounds();
	mr.getMaterial(materials);

	// FIXME: load skeleton and blend weights from PMD file
	//        also initialize the skeleton as needed

	int bone_id = 0;
	int parent_id;
	glm::vec3 offset;
	bool useful_bone;

	while (useful_bone) {
		useful_bone = mr.getJoint(bone_id++, offset, parent_id);
		printf("bone_id: %d\n", bone_id);
		printf("offset: (%f, %f, %f)\n", offset.x, offset.y, offset.z);
		printf("parent_id: %d\n", parent_id);

		Bone* bone = new Bone;

		if (parent_id == -1) {
			glm::vec3 parent_offset = glm::vec3(0.0f, 0.0f, 0.0f);
			glm::vec3 tangent = glm::normalize(parent_offset - offset);
			printf("tangent: (%f, %f, %f)\n", tangent.x, tangent.y, tangent.z);

			glm::vec3 normal = computeNormal(tangent);
			printf("normal: (%f, %f, %f)\n", normal.x, normal.y, normal.z);

			glm::vec3 binormal = glm::cross(tangent, normal);
			printf("binormal: (%f, %f, %f)\n", binormal.x, binormal.y, binormal.z);

			bone->id = bone_id;
			bone->parent_id = parent_id;
		} else {
			bone->id = bone_id;
			bone->parent_id = parent_id;
		}

	}
}

void Mesh::updateAnimation()
{
	animated_vertices = vertices;
	// FIXME: blend the vertices to animated_vertices, rather than copy
	//        the data directly.
}


void Mesh::computeBounds()
{
	bounds.min = glm::vec3(std::numeric_limits<float>::max());
	bounds.max = glm::vec3(-std::numeric_limits<float>::max());
	for (const auto& vert : vertices) {
		bounds.min = glm::min(glm::vec3(vert), bounds.min);
		bounds.max = glm::max(glm::vec3(vert), bounds.max);
	}
}

glm::vec3 Mesh::computeNormal(glm::vec3 tangent)
{
	glm::vec3 v = glm::normalize(tangent);
	v.x = glm::abs(v.x);
	v.z = glm::abs(v.z);
	v.y = glm::abs(v.y);

	float min = glm::min(v.x, glm::min(v.y, v.z));
	if (min == v.x) {
		v.x = 1.0f;
		v.y = 0.0f;
		v.z = 0.0f;
	}
	if (min == v.y) {
		v.x = 0.0f;
		v.y = 1.0f;
		v.z = 0.0f;
	}
	if (min == v.z) {
		v.x = 0.0f;
		v.y = 0.0f;
		v.z = 1.0f;
	}

	printf("v: (%f, %f, %f)\n", v.x, v.y, v.z);

	glm::vec3 nominator = glm::cross(tangent, v);
	float denominator = glm::length(nominator);
	return nominator / denominator;
}

