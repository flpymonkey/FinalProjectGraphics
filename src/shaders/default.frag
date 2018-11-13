R"zzz(#version 330 core
in vec4 normal;
in vec4 light_direction;
in vec4 world_normal;
out vec4 fragment_color;
void main()
{
	vec4 color = abs(normalize(world_normal)) + vec4(0.0, 0.0, 0.0, 1.0);
	float dot_nl = dot(normalize(light_direction), normalize(normal));
	dot_nl = clamp(dot_nl, 0.0, 1.0);
	fragment_color = clamp(dot_nl * color, 0.0, 1.0);
}
)zzz"