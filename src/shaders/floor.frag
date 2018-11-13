R"zzz(#version 330 core
in vec4 normal;
in vec4 light_direction;
in vec4 world_position;
uniform mat4 view;
uniform mat4 projection;
uniform vec4 light_position;
out vec4 fragment_color;
void main()
{
	vec4 position = inverse(projection * view) * vec4(world_position.xyz / world_position.w, 1.0f);
    position /= position.w;
	vec4 color = vec4(0.0, 0.0, 0.0, 1.0);
    if (mod(floor(position[0]) + floor(position[2]), 2) == 0) {
        color = vec4(1.0, 1.0, 1.0, 1.0);
    }
	float dot_nl = dot(normalize(light_direction), normalize(normal));
	dot_nl = clamp(dot_nl, 0.0, 1.0);
	fragment_color = clamp(dot_nl * color, 0.0, 1.0);
}
)zzz"