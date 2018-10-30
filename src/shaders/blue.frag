R"zzz(
#version 330 core
flat in int vertex;
out vec4 fragment_color;
void main(){
  if (vertex == 0 || vertex == 1) {
    fragment_color = vec4(1.0,1.0,0.0,1.0);
  } else if (vertex == 2 || vertex == 3) {
    fragment_color = vec4(0.0,1.0,0.0,1.0);
  } else {
    fragment_color = vec4(0.0,0.0,1.0,1.0);
  }
}
)zzz"
