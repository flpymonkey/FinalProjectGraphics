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
			bone->R = calculateRotationMatrix(offset);
			bone->C = bone->R;
		} else {
			Bone* parent = skeleton.bones[parent_id];
			bone->parent = parent;
			bone->LocalToWorld_R = parent->LocalToWorld_R * parent->R;
			bone->LocalToWorld = parent->LocalToWorld * parent->T * parent->R;

			// Calculate local translation relative to parent
			glm::vec3 local_offset = glm::vec3(glm::inverse(bone->LocalToWorld_R) * glm::vec4(offset, 1.0f));
			bone->T = glm::translate(local_offset);
			bone->length = glm::length(local_offset);

			// Create R
			bone->R = calculateRotationMatrix(local_offset);
			bone->C = bone->R;

			// Assign to parent bone
			parent->children.push_back(bone);
		}

		skeleton.bones.push_back(bone);
		bone_id++;
	}
}

glm::mat4 Mesh::calculateRotationMatrix(glm::vec3 offset){
	glm::vec3 tangent = glm::vec3(glm::normalize(offset));
	glm::vec3 normal = glm::normalize(computeNormal(tangent));
	glm::vec3 binormal = glm::normalize(glm::cross(tangent, normal));
	return glm::mat4(glm::vec4(tangent, 0.0f), glm::vec4(normal, 0.0f), glm::vec4(binormal, 0.0f), glm::vec4(0.0f,0.0f,0.0f,1.0f));
}

void Mesh::rotateBone(int bone_id, glm::vec3 mouse_direction, glm::vec3 look, float rotation_speed){
	// take its cross product with the look direction,
	glm::vec3 rotation_axis = glm::cross(mouse_direction, look);
	// and rotate all basis vectors of the bone about this axis by rotation_speed radians.
	glm::vec3 tangent = glm::vec3(skeleton.bones[bone_id]->C[0]);

	glm::vec3 new_offset = glm::rotate(tangent, rotation_speed, rotation_axis);

	Bone* bone = skeleton.bones[bone_id];
	bone->C = calculateRotationMatrix(new_offset);
	bone->T = glm::translate(new_offset * bone->length);
	for (uint i = 0; i < bone->children.size(); ++i) {
		updateLocalToWorld(bone->children[i]);
	}
}

void Mesh::updateLocalToWorld(Bone* bone){
	Bone* parent = skeleton.bones[bone->parent_id];
	bone->LocalToWorld_R = parent->LocalToWorld_R * parent->C;
	bone->LocalToWorld = parent->LocalToWorld * parent->T * parent->C;

	for (uint i = 0; i < bone->children.size(); ++i) {
		updateLocalToWorld(bone->children[i]);
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
	glm::vec4 worldPosition = bone->LocalToWorld * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
	skeleton_vertices.push_back(worldPosition);
	face_counter++;
	glm::vec4 end_worldPosition = bone->LocalToWorld * bone->C * glm::vec4(bone->length, 0.0f, 0.0f, 1.0f);
	skeleton_vertices.push_back(end_worldPosition);
	skeleton_faces.push_back(glm::uvec2(face_counter - 1, face_counter));
	face_counter++;

	for (uint i = 0; i < bone->children.size(); ++i) {
		Bone* child = bone->children[i];
		generateVertices(skeleton_vertices, skeleton_faces, child, face_counter);
	}
}
