#include "gui.h"

std::string glsl_version = "#version 330 core";
ImVec4 clear_color = ImColor(114, 144, 154);
bool show_test_window = true;

BasicGUI::BasicGUI(GLFWwindow* window, int* score, std::string* object_goal){
  // Setup ImGui binding
  if (show_test_window)
  {
    ImGui_ImplGlfwGL3_Init(window, false);
    auto io = ImGui::GetIO();
    io.WantCaptureKeyboard = false;

  }
  this->window = window;
  this->score = score;
  this->object_goal = object_goal;
}

void BasicGUI::render(){

  // 3. Show the ImGui test window. Most of the sample code is in ImGui::ShowTestWindow()
  if (show_test_window)
  {
    ImGui_ImplGlfwGL3_NewFrame();

    static float f = 0.0f;
    ImGui::Text("Your score: %d", *(this->score));
	ImGui::Text("Take a picture of: %s", (*(this->object_goal)).c_str());
    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

    ImGui::SetNextWindowPos(ImVec2(300, 5), ImGuiSetCond_FirstUseEver);
    // Rendering
    ImGui::Render();
  }
}
