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

	while (mr.getJoint(bone_id, offset, parent_id)) {
		printInt("bone_id", bone_id);
		printVec3("offset", offset);
		printInt("parent_id", parent_id);

		if (bone_id == 2){
			break;
		}

		Bone* bone = new Bone;
		bone->id = bone_id;
		bone->parent_id = parent_id;

		glm::vec3 tangent;
		glm::vec3 normal;
		glm::vec3 binormal;
	
		if (parent_id == -1) {
			bone->parent = NULL;
			bone->offset = offset;
			bone->local_offset = offset;
			bone->length = glm::length(offset);
			tangent = glm::normalize(offset);
			normal = glm::normalize(computeNormal(tangent));
			binormal = glm::normalize(glm::cross(tangent, normal));
			bone->T = glm::translate(offset);
			bone->R = glm::mat4(glm::vec4(tangent, 0.0f), glm::vec4(normal, 0.0f), glm::vec4(binormal, 0.0f), glm::vec4(0.0f,0.0f,0.0f,1.0f));
			bone->LocalToWorld = glm::mat4(1.0f);
			bone->LocalToWorld_R = glm::mat4(1.0f);
			skeleton.root = bone;
		} else {
			Bone* parent = skeleton.bones[parent_id];
			bone->parent = parent;
			bone->LocalToWorld_R = parent->LocalToWorld_R * parent->R;
			bone->LocalToWorld = parent->LocalToWorld * parent->T * parent->R;
			bone->local_offset = glm::vec3(glm::inverse(bone->LocalToWorld_R) * glm::vec4(offset, 1.0f));
			bone->T = glm::translate(bone->local_offset);
			bone->length = glm::length(bone->local_offset);
			glm::vec4 parent_position = parent->LocalToWorld * parent->T * parent->R * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
			//parent_position.w = 1.0f;
			glm::vec4 position = bone->LocalToWorld * bone->T * glm::vec4(0.0f, 0.0f, bone->length, 1.0f);
			printMat4("LTW", bone->LocalToWorld);
			printVec4("parent_position", parent_position);
			printVec4("position", position);
			tangent = glm::vec3(glm::normalize(position - parent_position));
			normal = glm::normalize(computeNormal(tangent));
			binormal = glm::normalize(glm::cross(tangent, normal));
			bone->R = glm::mat4(glm::vec4(tangent, 0.0f), glm::vec4(normal, 0.0f), glm::vec4(binormal, 0.0f), glm::vec4(0.0f,0.0f,0.0f,1.0f));
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
	} else
	if (min == v.y) {
		v.x = 0.0f;
		v.y = 1.0f;
		v.z = 0.0f;
	} else
	if (min == v.z) {
		v.x = 0.0f;
		v.y = 0.0f;
		v.z = 1.0f;
	}

	//printf("v: (%f, %f, %f)\n", v.x, v.y, v.z);

	glm::vec3 nominator = glm::cross(v, tangent);
	float denominator = glm::length(nominator);
	return nominator / denominator;
}

// Create a list of vertices (representing joints) from the bones
void Mesh::getSkeletonJointsVec(std::vector<glm::vec4>& skeleton_vertices, std::vector<glm::uvec2>& skeleton_faces){
	// FIXME: idk if this logic is correct???
	// std::vector<float> verts;
	Bone* bone;

	//FIXME: might want to skip the first bone (bone form origin of world to base)
	for (int i = 0; i < 2; ++i){
		bone = skeleton.bones[i];

		// // FIXME: Make sure we are multiplying the correct R times length!
		// // FIXME: Make sure its not supposed to be parent R times this bones length
		// // Get tanget from this bones R
		// glm::vec4 tangent = glm::vec4(bone->R[0][0], bone->R[0][1],
		// 	bone->R[0][2], 0);
		// // and multiply by length to get this bones position relative to parent
		// glm::vec4 relativePosition = tangent * bone->length;
		// // multiply this times local to world to get this bones world corrdinates
		// //FIXME: make sure this multiplication is in the correct order!
		// glm::vec4 worldPosition = bone->LocalToWorld * relativePosition;

		//glm::vec4 worldPosition = glm::vec4(0, 0, bone->length, 1.0f) * bone->R * bone->T * bone->LocalToWorld;
		glm::vec4 worldPosition = bone->LocalToWorld * bone->T * bone->R * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
		//worldPosition.w = 1.0f;
		printMat4("LocalToWorld", bone->LocalToWorld);
		printMat4("T", bone->T);
		printMat4("R", bone->R);
		printVec4("V", glm::vec4(0, 0, bone->length, 1.0f));
		printVec4("worldPosition", worldPosition);

		// add these points to our verts list for opengl
		skeleton_vertices.push_back(worldPosition);

		if (i != 0) {
			skeleton_faces.push_back(glm::uvec2(i - 1, i));
		}
	}



	// convert this verts vector to an array and return it
	//return &verts[0];
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
