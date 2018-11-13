#include "camera.h"
#include <stdio.h>

namespace {
	float pan_speed = 0.1f;
	float roll_speed = 0.1f;
	float rotation_speed = 0.5f;
	float zoom_speed = 0.1f;
};

Camera::Camera() {}
Camera::~Camera() {}

// FIXME: Calculate the view matrix
glm::mat4 Camera::get_view_matrix() const
{
	return lookAt();
}

glm::mat4 Camera::lookAt() const {
	glm::mat4 look_at;

	glm::vec3 Z = glm::normalize(eye_ - center_);
	glm::vec3 Y = up_;
	glm::vec3 X = glm::normalize(glm::cross(Y, Z));
	Y = glm::normalize(glm::cross(Z, X));

	look_at[0][0] = X.x;
  look_at[1][0] = X.y;
  look_at[2][0] = X.z;
  look_at[3][0] = -glm::dot(X, eye_);
  look_at[0][1] = Y.x;
  look_at[1][1] = Y.y;
  look_at[2][1] = Y.z;
  look_at[3][1] = -glm::dot(Y, eye_);
  look_at[0][2] = Z.x;
  look_at[1][2] = Z.y;
  look_at[2][2] = Z.z;
  look_at[3][2] = -glm::dot(Z, eye_);
  look_at[0][3] = 0;
  look_at[1][3] = 0;
  look_at[2][3] = 0;
  look_at[3][3] = 1.0f;

	return look_at;
}

glm::vec3 Camera::get_mouse_move_direction(int window_width, int window_height,
		float xpos, float ypos, float delta) {

	// Compute new orientation
	horizontalAngle += rotation_speed * delta * float(window_width/2 - xpos );
	verticalAngle   += rotation_speed * delta * float( window_height/2 - ypos );

	// Direction : Spherical coordinates to Cartesian coordinates conversion
	glm::vec3 direction(
	    cos(verticalAngle) * sin(horizontalAngle),
	    sin(verticalAngle),
	    cos(verticalAngle) * cos(horizontalAngle)
	);

	look_ = direction;

	// Right vector
	glm::vec3 right = glm::vec3(
	    sin(horizontalAngle - 3.14f/2.0f),
	    0,
	    cos(horizontalAngle - 3.14f/2.0f)
	);

	// Up vector : perpendicular to both direction and right
	up_ = glm::cross( right, direction );

	return direction;
}

void Camera::upTranslate(){
	eye_ += pan_speed * up_;
	center_ += pan_speed * up_;
}

void Camera::downTranslate(){
	eye_ -= pan_speed * up_;
	center_ -= pan_speed * up_;
}

void Camera::rightTranslate(){
	eye_ -= pan_speed * right_;
	center_ -= pan_speed * right_;
}

void Camera::leftTranslate(){
	eye_ += pan_speed * right_;
	center_ += pan_speed * right_;
}

void Camera::forwardTranslate(){
	eye_ += pan_speed * look_;
	center_ += pan_speed * look_;
}

void Camera::backwardTranslate(){
	eye_ -= pan_speed * look_;
	center_ -= pan_speed * look_;
}

void Camera::rollRight(){
	up_ = glm::rotate(up_, -roll_speed, look_);
	right_ = glm::rotate(right_, -roll_speed, look_);
}

void Camera::rollLeft(){
	up_ = glm::rotate(up_, roll_speed, look_);
	right_ = glm::rotate(right_, roll_speed, look_);
}

void Camera::upCenter(){
	center_ += pan_speed * up_;
	look_ = -glm::normalize(eye_ - center_);
	up_ = glm::cross(look_, right_ );
}

void Camera::downCenter(){
	center_ -= pan_speed * up_;
	look_ = -glm::normalize(eye_ - center_);
	up_ = glm::cross(look_, right_);
}

void Camera::rightCenter(){
	center_ += pan_speed * right_;
	look_ = -glm::normalize(eye_ - center_);
	right_ = glm::cross(up_, look_);
}

void Camera::leftCenter(){
	center_ -= pan_speed * right_;
	look_ = -glm::normalize(eye_ - center_);
	right_ = glm::cross( up_, look_ );
}

void Camera::zoomForward(){
	if (abs(distance(center_, eye_)) > 0.1){
		eye_ += zoom_speed * look_;
	}
}

void Camera::zoomBackward(){
	if (abs(distance(center_, eye_)) < 10.0){
		eye_ -= zoom_speed * look_;
	}
}

void Camera::dynamicCenterRotate(double prev_x, double prev_y, double x, double y,
	int window_width, int window_height){

	glm::vec3 old_eye = eye_;
	center_ -= eye_;
	eye_ = glm::vec3(0.0f, 0.0f, 0.0f);

	float delta_x = x / window_width - prev_x /  window_width;
	float delta_y = y / window_height - prev_y / window_height;

	// Shift and adjust on x axis
	center_ = glm::rotate(center_, (float)(delta_x * rotation_speed), up_);
	look_ = -glm::normalize(center_);
	right_ = glm::normalize(glm::cross( look_, up_ ));

	// Shift and adjust on y axis
	center_ = glm::rotate(center_, (float)(delta_y * rotation_speed), right_);
	look_ = glm::normalize(center_);
	up_ = glm::normalize(glm::cross(look_, right_));

	eye_ = old_eye;
	center_ += eye_;
}

void Camera::dynamicEyeRotate(double prev_x, double prev_y, double x, double y,
	int window_width, int window_height){

	glm::vec3 old_center = center_;
	eye_ -= center_;
	center_ = glm::vec3(0.0f, 0.0f, 0.0f);

	float delta_x = x / window_width - prev_x /  window_width;
	float delta_y = y / window_height - prev_y / window_height;

	eye_ = glm::rotate(eye_, (float)(delta_x * rotation_speed), up_);
	look_ = glm::normalize(eye_);
	right_ = glm::normalize(glm::cross( look_, up_ ));

	eye_ = glm::rotate(eye_, (float)(delta_y * rotation_speed), right_);
	look_ = -glm::normalize(eye_);
	up_ = glm::normalize(glm::cross(look_, right_));

	center_ = old_center;
	eye_ += center_;
}

void Camera::dynamicZoom(double prev_y, double y, int window_width, int window_height) {
	float delta_y = y / window_height - prev_y / window_height;
	if (abs(distance(center_, eye_)) > 0.1 || delta_y < 0 ){
		eye_ += zoom_speed * 10 * delta_y * look_;
	}
}
