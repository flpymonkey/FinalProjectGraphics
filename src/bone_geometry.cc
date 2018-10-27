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

	// TODO: load skeleton and blend weights from PMD file
	//        also initialize the skeleton as needed

	int bone_id = 0;
	int parent_id;
	glm::vec3 offset;

	while (mr.getJoint(bone_id, offset, parent_id)) {
		Bone* bone = new Bone;
		bone->id = bone_id;
		bone->parent_id = parent_id;

		glm::vec3 tangent;
		glm::vec3 normal;
		glm::vec3 binormal;

		// TODO: Get rid of offset, local_offset, make sure ordering is good in struct and in here. Get rid of dup code.
		if (parent_id == -1) {
			// This is the root of the skeleton_faces
			skeleton.root = bone;

			bone->parent = NULL;
			bone->LocalToWorld = glm::mat4(1.0f);
			bone->LocalToWorld_R = glm::mat4(1.0f);
			bone->length = glm::length(offset);
			bone->T = glm::translate(offset);

			//Calculate rotation matrix
			tangent = glm::normalize(offset);
			normal = glm::normalize(computeNormal(tangent));
			binormal = glm::normalize(glm::cross(tangent, normal));
			bone->R = glm::mat4(glm::vec4(tangent, 0.0f), glm::vec4(normal, 0.0f), glm::vec4(binormal, 0.0f), glm::vec4(0.0f,0.0f,0.0f,1.0f));
		} else {
			Bone* parent = skeleton.bones[parent_id];
			bone->parent = parent;
			bone->LocalToWorld_R = parent->LocalToWorld_R * parent->R;
			bone->LocalToWorld = parent->LocalToWorld * parent->T * parent->R;

			// Calculate local translation relative to parent
			glm::vec3 local_offset = glm::vec3(glm::inverse(bone->LocalToWorld_R) * glm::vec4(offset, 1.0f));
			bone->T = glm::translate(local_offset);
			bone->length = glm::length(local_offset);

			//Calculate rotation matrix relative to parent
			glm::vec4 parent_position = parent->LocalToWorld * parent->T * parent->R * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
			glm::vec4 position = bone->LocalToWorld * bone->T * glm::vec4(0.0f, 0.0f, bone->length, 1.0f);
			tangent = glm::vec3(glm::normalize(position - parent_position));
			normal = glm::normalize(computeNormal(tangent));
			binormal = glm::normalize(glm::cross(tangent, normal));
			bone->R = glm::mat4(glm::vec4(tangent, 0.0f), glm::vec4(normal, 0.0f), glm::vec4(binormal, 0.0f), glm::vec4(0.0f,0.0f,0.0f,1.0f));

			// Assign to parent bone
			parent->children.push_back(bone);
		}

		skeleton.bones.push_back(bone);
		bone_id++;
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
	} else if (min == v.y) {
		v.x = 0.0f;
		v.y = 1.0f;
		v.z = 0.0f;
	} else if (min == v.z) {
		v.x = 0.0f;
		v.y = 0.0f;
		v.z = 1.0f;
	}

	glm::vec3 nominator = glm::cross(v, tangent);
	float denominator = glm::length(nominator);
	return nominator / denominator;
}

// Create a list of vertices (representing joints) from the bones
void Mesh::generateSkeleton(std::vector<glm::vec4>& skeleton_vertices, std::vector<glm::uvec2>& skeleton_faces) {
	int face_counter = 0;
	generateVertices(skeleton_vertices, skeleton_faces, skeleton.root, face_counter);
}

void Mesh::generateVertices(std::vector<glm::vec4>& skeleton_vertices, std::vector<glm::uvec2>& skeleton_faces, Bone* bone, int& face_counter) {
	if (bone->children.size() == 0) {
		return;
	}

	for (uint i = 0; i < bone->children.size(); ++i) {
		Bone* child = bone->children[i];
		glm::vec4 worldPosition = bone->LocalToWorld * bone->T * bone->R * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
		skeleton_vertices.push_back(worldPosition);
		face_counter++;
		glm::vec4 child_worldPosition = child->LocalToWorld * child->T * child->R * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
		skeleton_vertices.push_back(child_worldPosition);
		skeleton_faces.push_back(glm::uvec2(face_counter - 1, face_counter));
		face_counter++;
		generateVertices(skeleton_vertices, skeleton_faces, child, face_counter);
	}
}

void Mesh::printInt(std::string name, int data) {
	printf("%s: %i\n", name.c_str(), data);
}

void Mesh::printFloat(std::string name, float data) {
	printf("%s: %f\n", name.c_str(), data);
}

void Mesh::printVec3(std::string name, glm::vec3 data) {
	printf("%s: (%f, %f, %f)\n", name.c_str(), data.x, data.y, data.z);
}

void Mesh::printVec4(std::string name, glm::vec4 data) {
	printf("%s: (%f, %f, %f, %f)\n", name.c_str(), data.x, data.y, data.z, data.w);
}

void Mesh::printMat4(std::string name, glm::mat4 data) {
	printf("%s: (%f, %f, %f, %f)\n", name.c_str(), data[0][0], data[1][0], data[2][0], data[3][0]);
	printf("%s: (%f, %f, %f, %f)\n", name.c_str(), data[0][1], data[1][1], data[2][1], data[3][1]);
	printf("%s: (%f, %f, %f, %f)\n", name.c_str(), data[0][2], data[1][2], data[2][2], data[3][2]);
	printf("%s: (%f, %f, %f, %f)\n", name.c_str(), data[0][3], data[1][3], data[2][3], data[3][3]);
}
