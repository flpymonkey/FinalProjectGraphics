R"zzz(#version 330 core
in vec4 normal;
in vec4 light_direction;
in vec4 world_normal;
in vec4 world_position;
out vec4 fragment_color;

uniform vec4 view_position;

struct PointLight {
    vec3 position;

    float constant;
    float linear;
    float quadratic;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

// calculates the color when using a point light.
// Source: https://learnopengl.com/Lighting/Multiple-lights
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
    vec3 lightDir = normalize(light.position - fragPos);
    // diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    // specular shading
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = max(dot(viewDir, reflectDir), 0.0);
    // attenuation
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));
    // combine results
    vec3 ambient = light.ambient;
    vec3 diffuse = light.diffuse * diff;
    vec3 specular = light.specular * spec;
    ambient *= attenuation;
    diffuse *= attenuation;
    specular *= attenuation;
    return (ambient + diffuse + specular);
}

void main()
{
	PointLight light;
	light.position = vec3(5.0, 5.0, 5.0);
	light.ambient = vec3(0.05, 0.05, 0.05);
	light.diffuse = vec3(0.8, 0.8, 0.8);
	light.specular = vec3(1.0, 1.0, 1.0);
	light.constant = 1.0;
	light.linear = 0.09;
	light.quadratic = 0.032;

	vec4 color = abs(normalize(world_normal)) + vec4(0.0, 0.0, 0.0, 1.0);

	vec3 norm = vec3(normalize(normal));
	vec3 viewDir = normalize(vec3(view_position) - vec3(world_position));
	fragment_color = vec4(CalcPointLight(light, norm, vec3(world_position), viewDir), 1.0) * color;
}
)zzz"
