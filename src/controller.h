#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <glm/glm.hpp>
#include <vector>
#include <GLFW/glfw3.h>
#include <memory>
#include <ctime>

#include "camera.h"
#include "menger.h"
#include "jpegio.h"
#include "saver.h"
#include "filesystem.h"

class Controller {
public:
	Controller(GLFWwindow* window, Camera* camera, Menger* menger, float* exposure, bool* showMeshes, bool* lensEffects, bool* captureImage);
	~Controller();

	void keyCallback(int key, int scancode, int action, int mods);
	void mousePosCallback(double mouse_x, double mouse_y);
	void mouseButtonCallback(int button, int action, int mods);

	static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
	static void MousePosCallback(GLFWwindow* window, double mouse_x, double mouse_y);
	static void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);

private:
	GLFWwindow* window;
	Camera* camera;
	Menger* menger;

	int window_width;
	int window_height;

	float* exposure;
  bool* showMeshes;
  bool* lensEffects;
  bool* captureImage;
	bool fps_mode;
	double prev_x;
	double prev_y;

	int g_current_button;
	bool g_mouse_pressed;

};

#endif
