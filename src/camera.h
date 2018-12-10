#ifndef CAMERA_H
#define CAMERA_H

#include <glm/glm.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include "glm/gtc/matrix_transform.hpp"

class Camera {
public:
	Camera();
	~Camera();

	glm::mat4 get_view_matrix() const;
	// FIXME: add functions to manipulate camera objects.
	glm::vec3 get_mouse_move_direction(int, int, float, float, float);
	glm::mat4 lookAt() const;

	// Translation and movement
	void forwardTranslate();
	void backwardTranslate();
	void leftTranslate();
	void rightTranslate();
	void upTranslate();
	void downTranslate();
	void rollRight();
	void rollLeft();
	void upCenter();
	void downCenter();
	void rightCenter();
	void leftCenter();
	void zoomForward();
	void zoomBackward();

	void dynamicCenterRotate(double prev_x, double prev_y, double x, double y,
		int window_width, int window_height);
	void dynamicEyeRotate(double prev_x, double prev_y, double x, double y,
		int window_width, int window_height);
	void dynamicZoom(double prev_y, double y, int window_width, int window_height);

	glm::vec3 getPosition(){
		return eye_;
	}

	float camera_distance_ = 3.0f; // 3
	glm::vec3 look_ = glm::vec3(0.0f, 0.0f, -1.0f); //0, 0, -1
	glm::vec3 up_ = glm::vec3(0.0f, 1.0f, 0.0f);
	glm::vec3 eye_ = glm::vec3(0.0f, 3.0f, camera_distance_); // 0, 0, c

	glm::vec3 center_ = glm::vec3(0.0f, 3.0f, 0.0f);
	glm::vec3 right_ = glm::cross( up_, look_ );
	// Note: you may need additional member variables
	float verticalAngle = 0;
	float horizontalAngle = 0;
};

#endif
