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

	while (mr.getJoint(bone_id++, offset, parent_id)) {
		printf("bone_id: %d\n", bone_id);
		printf("offset: (%f, %f, %f)\n", offset.x, offset.y, offset.z);
		printf("parent_id: %d\n", parent_id);

		Bone* bone = new Bone;
		glm::vec3 tangent;
		glm::vec3 normal;
		glm::vec3 binormal;
		glm::mat4 orientation;
		glm::mat4 LocalToWorld;
		float length;
		Bone* parent = NULL;

		if (parent_id == -1) {
			glm::vec3 parent_offset = glm::vec3(0.0f, 0.0f, 0.0f);
			tangent = glm::normalize(offset - parent_offset);
			normal = glm::normalize(computeNormal(tangent));
			binormal = glm::normalize(glm::cross(tangent, normal));
			orientation = computeOrientation(tangent, normal, binormal);
			LocalToWorld = glm::mat4(1.0f);
			length = glm::length(offset);
			skeleton.root = bone;

		} else {
			Bone* parent = skeleton.bones[parent_id];
			tangent = glm::normalize(parent->offset - offset);
			normal = glm::normalize(computeNormal(tangent));
			binormal = glm::normalize(glm::cross(tangent, normal));
			orientation = computeOrientation(tangent, normal, binormal);
			LocalToWorld = parent->LocalToWorld * (parent->orientation * parent->length); // FIXME: should we flip it?
			// FIXME I dont think we are calculating offset correctly? should just be magnitude of offset?
			length = glm::length(offset);
			parent->children.push_back(bone);
			// TODO: Know this for calculating vert position relative to parent,
			// position relative to parent is tangent (from orientation * length)
			// get world position using LocalToWorld * (from orientation * length)
		}

		printf("tangent: (%f, %f, %f)\n", tangent.x, tangent.y, tangent.z);
		printf("normal: (%f, %f, %f)\n", normal.x, normal.y, normal.z);
		printf("binormal: (%f, %f, %f)\n", binormal.x, binormal.y, binormal.z);
		printf("orientation: (%f, %f, %f, %f)\n", orientation[0][0], orientation[0][1], orientation[0][2], orientation[0][3]);
		printf("orientation: (%f, %f, %f, %f)\n", orientation[1][0], orientation[1][1], orientation[1][2], orientation[1][3]);
		printf("orientation: (%f, %f, %f, %f)\n", orientation[2][0], orientation[2][1], orientation[2][2], orientation[2][3]);
		printf("orientation: (%f, %f, %f, %f)\n", orientation[3][0], orientation[3][1], orientation[3][2], orientation[3][3]);
		printf("length: %f\n", length);

		bone->name = "bone_" + bone_id;
		bone->id = bone_id;
		bone->parent_id = parent_id;
		bone->parent = parent;
		bone->length = length;
		bone->orientation = orientation;
		bone->LocalToWorld = LocalToWorld;
		bone->offset = offset;

		skeleton.bones.push_back(bone);

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

glm::mat4 Mesh::computeOrientation(glm::vec3 tangent, glm::vec3 normal, glm::vec3 binormal)
{
	glm::mat4 orientation;
	orientation[0][0] = tangent.x;
	orientation[1][0] = tangent.y;
	orientation[2][0] = tangent.z;
	orientation[3][0] = 0.0f;
	orientation[0][1] = normal.x;
	orientation[1][1] = normal.y;
	orientation[2][1] = normal.z;
	orientation[3][1] = 0.0f;
	orientation[0][2] = binormal.x;
	orientation[1][2] = binormal.y;
	orientation[2][2] = binormal.z;
	orientation[3][2] = 0.0f;
	orientation[0][3] = 0.0f;
	orientation[1][3] = 0.0f;
	orientation[2][3] = 0.0f;
	orientation[3][3] = 1.0f;
	return orientation;
}

// Create a list of vertices (representing joints) from the bones
void Mesh::getSkeletonJoints(std::vector<float>& verts){
	// FIXME: idk if this logic is correct???
	// std::vector<float> verts;
	Bone* bone;

	//FIXME: might want to skip the first bone (bone form origin of world to base)
	for (int i = 0; i < getNumberOfBones(); ++i){
		bone = skeleton.bones[i];

		// // FIXME: Make sure we are multiplying the correct orientation times length!
		// // FIXME: Make sure its not supposed to be parent orientation times this bones length
		// // Get tanget from this bones orientation
		// glm::vec4 tangent = glm::vec4(bone->orientation[0][0], bone->orientation[0][1],
		// 	bone->orientation[0][2], 0);
		// // and multiply by length to get this bones position relative to parent
		// glm::vec4 relativePosition = tangent * bone->length;
		// // multiply this times local to world to get this bones world corrdinates
		// //FIXME: make sure this multiplication is in the correct order!
		// glm::vec4 worldPosition = bone->LocalToWorld * relativePosition;

		glm::vec4 worldPosition = bone->LocalToWorld * glm::vec4(bone->offset, 0.0f);

		// add these points to our verts list for opengl
		verts.push_back(worldPosition.x);
		verts.push_back(worldPosition.y);
		verts.push_back(worldPosition.z);
	}

	// convert this verts vector to an array and return it
	//return &verts[0];
}
