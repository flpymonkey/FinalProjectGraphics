#include "gui.h"
#include "config.h"
#include <jpegio.h>
#include "bone_geometry.h"
#include <iostream>
#include <debuggl.h>
#include <glm/gtc/matrix_access.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/projection.hpp>

namespace {
	// Intersect a cylinder with radius 1/2, height 1, with base centered at
	// (0, 0, 0) and up direction (0, 1, 0).
	bool IntersectCylinder(const glm::vec3& origin, const glm::vec3& direction,
			float radius, float height, float* t)
	{
		//FIXME perform proper ray-cylinder collision detection
		return true;
	}
}

GUI::GUI(GLFWwindow* window)
	:window_(window)
{
	glfwSetWindowUserPointer(window_, this);
	glfwSetKeyCallback(window_, KeyCallback);
	glfwSetCursorPosCallback(window_, MousePosCallback);
	glfwSetMouseButtonCallback(window_, MouseButtonCallback);

	glfwGetWindowSize(window_, &window_width_, &window_height_);
	float aspect_ = static_cast<float>(window_width_) / window_height_;
	projection_matrix_ = glm::perspective((float)(kFov * (M_PI / 180.0f)), aspect_, kNear, kFar);
}

GUI::~GUI()
{
}

void GUI::assignMesh(Mesh* mesh)
{
	mesh_ = mesh;
	center_ = mesh_->getCenter();
}

void GUI::keyCallback(int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
		glfwSetWindowShouldClose(window_, GL_TRUE);
		return ;
	}
	if (key == GLFW_KEY_J && action == GLFW_RELEASE) {
		//FIXME save out a screenshot using SaveJPEG
	}

	if (captureWASDUPDOWN(key, action))
		return ;
	if (key == GLFW_KEY_LEFT || key == GLFW_KEY_RIGHT) {
		float roll_speed;
		if (key == GLFW_KEY_RIGHT)
			roll_speed = -roll_speed_;
		else
			roll_speed = roll_speed_;
		// FIXME: actually roll the bone here
	} else if (key == GLFW_KEY_C && action != GLFW_RELEASE) {
		fps_mode_ = !fps_mode_;
	} else if (key == GLFW_KEY_LEFT_BRACKET && action == GLFW_RELEASE) {
		current_bone_--;
		current_bone_ += mesh_->getNumberOfBones();
		current_bone_ %= mesh_->getNumberOfBones();
	} else if (key == GLFW_KEY_RIGHT_BRACKET && action == GLFW_RELEASE) {
		current_bone_++;
		current_bone_ += mesh_->getNumberOfBones();
		current_bone_ %= mesh_->getNumberOfBones();
	} else if (key == GLFW_KEY_T && action != GLFW_RELEASE) {
		transparent_ = !transparent_;
	}
}

void GUI::mousePosCallback(double mouse_x, double mouse_y)
{
	last_x_ = current_x_;
	last_y_ = current_y_;
	current_x_ = mouse_x;
	current_y_ = window_height_ - mouse_y;
	float delta_x = current_x_ - last_x_;
	float delta_y = current_y_ - last_y_;
	if (sqrt(delta_x * delta_x + delta_y * delta_y) < 1e-15)
		return;
	glm::vec3 mouse_direction = glm::normalize(glm::vec3(delta_x, delta_y, 0.0f));
	glm::vec2 mouse_start = glm::vec2(last_x_, last_y_);
	glm::vec2 mouse_end = glm::vec2(current_x_, current_y_);
	glm::uvec4 viewport = glm::uvec4(0, 0, window_width_, window_height_);

	bool drag_camera = drag_state_ && current_button_ == GLFW_MOUSE_BUTTON_RIGHT;
	bool drag_bone = drag_state_ && current_button_ == GLFW_MOUSE_BUTTON_LEFT;

	if (drag_camera) {
		glm::vec3 axis = glm::normalize(
				orientation_ *
				glm::vec3(mouse_direction.y, -mouse_direction.x, 0.0f)
				);
		orientation_ =
			glm::mat3(glm::rotate(rotation_speed_, axis) * glm::mat4(orientation_));
		tangent_ = glm::column(orientation_, 0);
		up_ = glm::column(orientation_, 1);
		look_ = glm::column(orientation_, 2);
	} else if (drag_bone && current_bone_ != -1) {
		// FIXME: Handle bone rotation
		return ;
	}

	// FIXME: highlight bones that have been moused over
	current_bone_ = checkRayBoneIntersect(mouse_x, mouse_y);
}

void printInt(std::string name, int data) {
	printf("%s: %i\n", name.c_str(), data);
}

void printFloat(std::string name, float data) {
	printf("%s: %f\n", name.c_str(), data);
}

void printVec3(std::string name, glm::vec3 data) {
	printf("%s: (%f, %f, %f)\n", name.c_str(), data.x, data.y, data.z);
}

void printVec4(std::string name, glm::vec4 data) {
	printf("%s: (%f, %f, %f, %f)\n", name.c_str(), data.x, data.y, data.z, data.w);
}

void printMat4(std::string name, glm::mat4 data) {
	printf("%s: (%f, %f, %f, %f)\n", name.c_str(), data[0][0], data[1][0], data[2][0], data[3][0]);
	printf("%s: (%f, %f, %f, %f)\n", name.c_str(), data[0][1], data[1][1], data[2][1], data[3][1]);
	printf("%s: (%f, %f, %f, %f)\n", name.c_str(), data[0][2], data[1][2], data[2][2], data[3][2]);
	printf("%s: (%f, %f, %f, %f)\n", name.c_str(), data[0][3], data[1][3], data[2][3], data[3][3]);
}

int GUI::checkRayBoneIntersect(double mouse_x, double mouse_y){
	Ray r;
	r.direction = getCameraRayDirection(mouse_x, mouse_y);
	printVec4("r.d", r.direction);
	r.origin = glm::vec4(eye_, 1.0f);
	r.intersect_id = -1; // No intersection found
	r.minimum_t = kFar; // No t has been found if this is farthest t

	identifyBoneIntersect(r);

	if (r.intersect_id != -1) {
		printf("cid: %i\n", r.intersect_id);
	}

	return r.intersect_id;
}

// FIXME THIS SUCKS, FIND A MATH WAY TO DO THIS
int signOfFloat(float i){
	if (i < 0){
		return -1;
	}
	return 1;
}

void GUI::identifyBoneIntersect(Ray& r){
	for (int i = 0; i < mesh_->getNumberOfBones(); ++i){
		Bone* bone = mesh_->skeleton.bones[i];
		glm::mat4 WtL = glm::inverse(bone->LocalToWorld);
		glm::mat4 WtL_R = glm::inverse(bone->LocalToWorld_R);

		glm::vec4 bone_local_rdir = WtL_R * r.direction;
		glm::vec4 bone_local_rpos = WtL * r.origin;

		// Projects ray direction onto yz plane of circle
		glm::vec4 projection_dir = glm::proj(bone_local_rdir, glm::vec4(1.0f, 0.0f, 0.0f, 0.0f));

		// FIXME: Make sure the asre the correct axis!!!!!
		glm::vec2 zy_plane_rdir(projection_dir.z, projection_dir.y);
		glm::vec2 zy_plane_rpos(bone_local_rpos.z, bone_local_rpos.y);

		// Check for intersection with circle
		glm::vec2 max_ray_point = zy_plane_rdir * kFar;

		float dx = max_ray_point.x - zy_plane_rpos.x;
		float dy = max_ray_point.y - zy_plane_rpos.y;

		float dr = glm::length(glm::vec2(dx, dy));
		assert(dr != 0); // No radius doesnt make sense

		float D = zy_plane_rpos.x*max_ray_point.y - max_ray_point.x*zy_plane_rpos.y;

		// Calculate discriminant
		float discriminant = (kCylinderRadius * kCylinderRadius) * (dr * dr) - (D * D);

		if (discriminant >= 0){
			// We have an intersection with a circle
			float sqrt_factor = sqrt((kCylinderRadius * kCylinderRadius) * (dr * dr) - (D * D));
			float nominator = D * dy - signOfFloat(dy) * dx * sqrt_factor;
			float intersection_min_x = nominator / (dr * dr);

			nominator = -D * dx - abs(dy) * sqrt_factor;
			float intersection_min_y = nominator / (dr * dr);

			float t = glm::length(glm::vec2(intersection_min_x, intersection_min_y) - zy_plane_rpos);

			// Check that it is in the cylinder on the (X axis)
			glm::vec4 bone_local_ipos = bone_local_rpos + bone_local_rdir * t;

			if (bone_local_ipos.x >= 0 && bone_local_ipos.x <= bone->length){
				// We have an intersection with a cylinder!!!
				// Check if this intersection is the minimum of all bones
				if (r.minimum_t > t) {
					r.minimum_t = t;
					r.intersect_id = i;
				}
			}
		}
	}
}

glm::vec4 GUI::getCameraRayDirection(double mouse_x, double mouse_y){
	glm::vec3 win((float) mouse_x, (float)mouse_y, 0.0f); // What is this third value supposed to be?
	glm::mat4 model = view_matrix_ * model_matrix_;
	glm::mat4 proj = projection_matrix_;
	glm::vec4 viewport(0.0f, 0.0f, (float)window_width_, (float)window_height_);

	glm::vec3 unprojectedPoint = glm::unProject(win, model, proj, viewport);
	glm::vec3 direction = glm::normalize(eye_ - unprojectedPoint);

	return glm::vec4(direction, 0.0f);

}

void GUI::mouseButtonCallback(int button, int action, int mods)
{
	drag_state_ = (action == GLFW_PRESS);
	current_button_ = button;
}

void GUI::updateMatrices()
{
	// Compute our view, and projection matrices.
	if (fps_mode_)
		center_ = eye_ + camera_distance_ * look_;
	else
		eye_ = center_ - camera_distance_ * look_;

	view_matrix_ = glm::lookAt(eye_, center_, up_);
	light_position_ = glm::vec4(eye_, 1.0f);

	aspect_ = static_cast<float>(window_width_) / window_height_;
	projection_matrix_ =
		glm::perspective((float)(kFov * (M_PI / 180.0f)), aspect_, kNear, kFar);
	model_matrix_ = glm::mat4(1.0f);
}

MatrixPointers GUI::getMatrixPointers() const
{
	MatrixPointers ret;
	ret.projection = &projection_matrix_[0][0];
	ret.model= &model_matrix_[0][0];
	ret.view = &view_matrix_[0][0];
	return ret;
}

bool GUI::setCurrentBone(int i)
{
	if (i < 0 || i >= mesh_->getNumberOfBones())
		return false;
	current_bone_ = i;
	return true;
}

bool GUI::captureWASDUPDOWN(int key, int action)
{
	if (key == GLFW_KEY_W) {
		if (fps_mode_)
			eye_ += zoom_speed_ * look_;
		else
			camera_distance_ -= zoom_speed_;
		return true;
	} else if (key == GLFW_KEY_S) {
		if (fps_mode_)
			eye_ -= zoom_speed_ * look_;
		else
			camera_distance_ += zoom_speed_;
		return true;
	} else if (key == GLFW_KEY_A) {
		if (fps_mode_)
			eye_ -= pan_speed_ * tangent_;
		else
			center_ -= pan_speed_ * tangent_;
		return true;
	} else if (key == GLFW_KEY_D) {
		if (fps_mode_)
			eye_ += pan_speed_ * tangent_;
		else
			center_ += pan_speed_ * tangent_;
		return true;
	} else if (key == GLFW_KEY_DOWN) {
		if (fps_mode_)
			eye_ -= pan_speed_ * up_;
		else
			center_ -= pan_speed_ * up_;
		return true;
	} else if (key == GLFW_KEY_UP) {
		if (fps_mode_)
			eye_ += pan_speed_ * up_;
		else
			center_ += pan_speed_ * up_;
		return true;
	}
	return false;
}


// Delegrate to the actual GUI object.
void GUI::KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	GUI* gui = (GUI*)glfwGetWindowUserPointer(window);
	gui->keyCallback(key, scancode, action, mods);
}

void GUI::MousePosCallback(GLFWwindow* window, double mouse_x, double mouse_y)
{
	GUI* gui = (GUI*)glfwGetWindowUserPointer(window);
	gui->mousePosCallback(mouse_x, mouse_y);
}

void GUI::MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
	GUI* gui = (GUI*)glfwGetWindowUserPointer(window);
	gui->mouseButtonCallback(button, action, mods);
}
