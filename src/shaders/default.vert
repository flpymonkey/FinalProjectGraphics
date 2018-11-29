R"zzz(#version 330 core
in vec4 vertex_position;
in vec4 vertex_normal;
in vec2 vertex_uv;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform vec4 light_position;
out vec4 light_direction;
out vec4 normal;
out vec4 world_normal;
out vec4 world_position;
out vec2 uv;
void main()
{
// Transform vertex into clipping coordinates
	gl_Position = projection * view * model * vertex_position;
// Lighting in camera coordinates
//  Compute light direction and transform to camera coordinates
        light_direction = view * (light_position - vertex_position);
//  Transform normal to camera coordinates
        normal = view * vertex_normal;
        world_normal = vertex_normal;
        world_position = projection * view * vertex_position;
        uv = vertex_uv;
}
)zzz"