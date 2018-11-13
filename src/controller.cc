#include "controller.h"

Controller::Controller(GLFWwindow* window, Camera* camera, Menger* menger, float* exposure) {
	this->window = window;
	this->camera = camera;
	this->menger = menger;

	this->exposure = exposure;
	this->fps_mode = true;
	this->prev_x = 0.0;
	this->prev_y = 0.0;

	glfwSetWindowUserPointer(window, this);
	glfwSetKeyCallback(window, KeyCallback);
	glfwSetCursorPosCallback(window, MousePosCallback);
	glfwSetMouseButtonCallback(window, MouseButtonCallback);
	glfwGetWindowSize(window, &window_width, &window_height);
}

Controller::~Controller() {}

void
Controller::keyCallback(int key, int scancode, int action, int mods)
{
	// Note:
	// This is only a list of functions to implement.
	// you may want to re-organize this piece of code.
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);
	else if (key == GLFW_KEY_W && action != GLFW_RELEASE) {
		if (fps_mode){
			camera->forwardTranslate();
		} else {
			camera->zoomForward();
		}
	} else if (key == GLFW_KEY_S && action != GLFW_RELEASE) {
		if (fps_mode){
			camera->backwardTranslate();
		} else {
			camera->zoomBackward();
		}
	} else if (key == GLFW_KEY_A && action != GLFW_RELEASE) {
		if (fps_mode){
			camera->leftTranslate();
		} else {
			camera->rightCenter();
		}
	} else if (key == GLFW_KEY_D && action != GLFW_RELEASE) {
		if (fps_mode) {
			camera->rightTranslate();
		} else {
			camera->leftCenter();
		}
	} else if (key == GLFW_KEY_LEFT && action != GLFW_RELEASE) {
		camera->rollLeft();
	} else if (key == GLFW_KEY_RIGHT && action != GLFW_RELEASE) {
		camera->rollRight();
	} else if (key == GLFW_KEY_DOWN && action != GLFW_RELEASE) {
		if (fps_mode){
			camera->downTranslate();
		} else {
			camera->downCenter();
		}
	} else if (key == GLFW_KEY_UP && action != GLFW_RELEASE) {
		if (fps_mode){
			camera->upTranslate();
		} else {
			camera->upCenter();
		}
	} else if (key == GLFW_KEY_C && action != GLFW_RELEASE) {
		fps_mode = !fps_mode;
	}	else if (key == GLFW_KEY_Q && action != GLFW_RELEASE){
		// Adjust exposure down
		if (*(this->exposure) > 0.0f){
			*(this->exposure) -= 0.02f;
		} else {
			*(this->exposure) = 0.0f;
		}
	} else if (key == GLFW_KEY_E && action != GLFW_RELEASE) {
		// Adjust exposure up
		*(this->exposure) += 0.02f;
	}


	if (!menger)
		return ; // 0-4 only available in Menger mode.
	if (key == GLFW_KEY_0 && action != GLFW_RELEASE) {
		menger->set_nesting_level(0);
	} else if (key == GLFW_KEY_1 && action != GLFW_RELEASE) {
		menger->set_nesting_level(1);
	} else if (key == GLFW_KEY_2 && action != GLFW_RELEASE) {
		menger->set_nesting_level(2);
	} else if (key == GLFW_KEY_3 && action != GLFW_RELEASE) {
		menger->set_nesting_level(3);
	} else if (key == GLFW_KEY_4 && action != GLFW_RELEASE) {
		menger->set_nesting_level(4);
	}
}

void
Controller::mousePosCallback(double mouse_x, double mouse_y)
{
	// FIXME COULD BE WEIRD RESULT IF MOUSE IS INITIALLY PRESSED ON GAME START
	if (!g_mouse_pressed){
		prev_x = mouse_x;
		prev_y = mouse_y;
		return;
	}
	if (g_current_button == GLFW_MOUSE_BUTTON_LEFT) {
		if (fps_mode) {
			camera->dynamicCenterRotate(prev_x, prev_y, mouse_x, mouse_y, window_width, window_height);
		} else {
			camera->dynamicEyeRotate(prev_x, prev_y, mouse_x, mouse_y, window_width, window_height);
		}
	} else if (g_current_button == GLFW_MOUSE_BUTTON_RIGHT) {
		camera->dynamicZoom(prev_y, mouse_y, window_width, window_height);
	} else if (g_current_button == GLFW_MOUSE_BUTTON_MIDDLE) {
		// FIXME: middle drag
	}
	prev_x = mouse_x;
	prev_y = mouse_y;
}

void
Controller::mouseButtonCallback(int button, int action, int mods)
{
	g_mouse_pressed = (action == GLFW_PRESS);
	g_current_button = button;
}

// Delegrate to the actual Controller object.
void
Controller::KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	Controller* controller = (Controller*)glfwGetWindowUserPointer(window);
	controller->keyCallback(key, scancode, action, mods);
}

void
Controller::MousePosCallback(GLFWwindow* window, double mouse_x, double mouse_y)
{
	Controller* controller = (Controller*)glfwGetWindowUserPointer(window);
	controller->mousePosCallback(mouse_x, mouse_y);
}

void
Controller::MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
	Controller* controller = (Controller*)glfwGetWindowUserPointer(window);
	controller->mouseButtonCallback(button, action, mods);
}
