R"zzz(
#version 330 core
uniform mat4 projection;
uniform mat4 model;
uniform mat4 view;
in vec4 vertex_position;
in int vertex_i;
flat out int vertex;
void main() {
	gl_Position = projection * view * model * vertex_position;

	vertex = vertex_i;
}
)zzz"
