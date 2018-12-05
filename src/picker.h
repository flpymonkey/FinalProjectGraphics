#include <glm/glm.hpp>

glm::vec3 get_color_from_id(int i){
  // Convert "i", the integer mesh ID, into an RGB color
  int r = (i & 0x000000FF) >>  0;
  int g = (i & 0x0000FF00) >>  8;
  int b = (i & 0x00FF0000) >> 16;
  return glm::vec3(r, g, b);
}

int get_id_from_color_data(char* data){
  // Convert the color back to an integer ID
  int pickedID =
  	data[0] +
  	data[1] * 256 +
  	data[2] * 256*256;
  return pickedID;
}
