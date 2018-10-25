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
		// printInt("bone_id", bone_id - 1);
		// printVec3("offset", offset);
		// printInt("parent_id", parent_id);

		Bone* bone = new Bone;
		glm::vec3 tangent;
		glm::vec3 normal;
		glm::vec3 binormal;
		glm::mat4 orientation;
		glm::mat4 LocalToWorld;
		float length;
		Bone* parent = NULL;

		// glm::mat4 T;
		// glm::mat4 R;

		if (parent_id == -1) {
			glm::vec3 parent_offset = glm::vec3(0.0f, 0.0f, 0.0f);
			tangent = glm::normalize(offset - parent_offset);
			normal = glm::normalize(computeNormal(tangent));
			binormal = glm::normalize(glm::cross(tangent, normal));
			orientation = computeOrientation(tangent, normal, binormal);
			LocalToWorld = glm::mat4(1.0f);
			length = glm::length(offset);
			skeleton.root = bone;
			printVec3("offset", offset);

			bone->T = glm::transpose(glm::translate(offset));
			//R = glm::rotate()

		} else {
			Bone* parent = skeleton.bones[parent_id];
			tangent = glm::normalize(offset - parent->offset);
			normal = glm::normalize(computeNormal(tangent));
			binormal = glm::normalize(glm::cross(tangent, normal));
			orientation = computeOrientation(tangent, normal, binormal);
			LocalToWorld = (parent->T * parent->orientation) * parent->LocalToWorld; // FIXME: should we flip it?
			// FIXME I dont think we are calculating offset correctly? should just be magnitude of offset?
			length = glm::length(offset);
			parent->children.push_back(bone);
			// TODO: Know this for calculating vert position relative to parent,
			// position relative to parent is tangent (from orientation * length)
			// get world position using LocalToWorld * (from orientation * length)

			bone->T = glm::transpose(glm::translate(offset));
			//float angle = glm::acos(glm::dot(parent->tangent, tangent));
			//R = rotate(angle, parent->normal);
		}

		// printVec3("tangent", tangent);
		// printVec3("normal", normal);
		// printVec3("binormal", binormal);
		// printMat4("orientation", orientation);
		// printMat4("LocalToWorld", LocalToWorld);
		// printFloat("length", length);
		// printMat4("T", bone->T);
		// printMat4("O", orientation);

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

	//printf("v: (%f, %f, %f)\n", v.x, v.y, v.z);

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


	// orientation[0][0] = tangent.x;
	// orientation[1][0] = normal.x;
	// orientation[2][0] = binormal.x;
	// orientation[3][0] = 0.0f;
	// orientation[0][1] = tangent.y;
	// orientation[1][1] = normal.y;
	// orientation[2][1] = binormal.y;
	// orientation[3][1] = 0.0f;
	// orientation[0][2] = tangent.z;
	// orientation[1][2] = normal.z;
	// orientation[2][2] = binormal.z;
	// orientation[3][2] = 0.0f;
	// orientation[0][3] = 0.0f;
	// orientation[1][3] = 0.0f;
	// orientation[2][3] = 0.0f;
	// orientation[3][3] = 1.0f;
	return orientation;
}

// Create a list of vertices (representing joints) from the bones
void Mesh::getSkeletonJointsVec(std::vector<glm::vec4>& skeleton_vertices, std::vector<glm::uvec2>& skeleton_faces){
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

		glm::vec4 worldPosition = bone->LocalToWorld * (bone->T * bone->orientation) * glm::vec4(bone->offset, 1.0f);

		printMat4("LocalToWorld", bone->LocalToWorld);
		printMat4("T", bone->T);
		printMat4("O", bone->orientation);
		printVec4("V", glm::vec4(bone->offset, 1.0f));
		printVec4("worldPosition", worldPosition);

		// add these points to our verts list for opengl
		skeleton_vertices.push_back(worldPosition);

		if ((i + 1) % 2 == 0) {
			skeleton_faces.push_back(glm::uvec2(i - 1, i));
		}
	}



	// convert this verts vector to an array and return it
	//return &verts[0];
}

void Mesh::printInt(char* name, int data) {
	printf("%s: %i\n", name, data);
}

void Mesh::printFloat(char* name, float data) {
	printf("%s: %f\n", name, data);
}

void Mesh::printVec3(char* name, glm::vec3 data) {
	printf("%s: (%f, %f, %f)\n", name, data.x, data.y, data.z);
}

void Mesh::printVec4(char* name, glm::vec4 data) {
	printf("%s: (%f, %f, %f, %f)\n", name, data.x, data.y, data.z, data.w);
}

void Mesh::printMat4(char* name, glm::mat4 data) {
	printf("%s: (%f, %f, %f, %f)\n", name, data[0][0], data[0][1], data[0][2], data[0][3]);
	printf("%s: (%f, %f, %f, %f)\n", name, data[1][0], data[1][1], data[1][2], data[1][3]);
	printf("%s: (%f, %f, %f, %f)\n", name, data[2][0], data[2][1], data[2][2], data[2][3]);
	printf("%s: (%f, %f, %f, %f)\n", name, data[3][0], data[3][1], data[3][2], data[3][3]);
}
