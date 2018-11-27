R"zzz(#version 330 core

// Source: https://learnopengl.com/Lighting/Multiple-lights

struct Material {
    sampler2D diffuse;
    sampler2D specular;
    float shininess;
};

struct DirectionalLight {
    vec3 direction;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct PointLight {
    vec3 position;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float constant;
    float linear;
    float quadratic;
};

struct SpotLight {
    vec3 position;
    vec3 direction;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float constant;
    float linear;
    float quadratic;
    float cutOff;
    float outerCutOff;
};

in vec4 normal;
in vec4 light_direction;
in vec4 world_normal;
in vec4 world_position;

uniform vec4 view_position;

uniform int dLights;
uniform int pLights;
uniform int sLights;

uniform DirectionalLight directionalLights[10];
uniform PointLight pointLights[10];
uniform SpotLight spotLights[10];

out vec4 fragment_color;

// calculates the color when using a directional light.
vec3 CalcDirLight(DirectionalLight light, vec3 normal, vec3 viewDir)
{
    vec3 lightDir = normalize(-light.direction);
    // diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    // specular shading
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 1.0); // material.shininess);
    // combine results
    vec3 ambient = light.ambient; // * vec3(texture(material.diffuse, TexCoords));
    vec3 diffuse = light.diffuse * diff; // * vec3(texture(material.diffuse, TexCoords));
    vec3 specular = light.specular * spec; // * vec3(texture(material.specular, TexCoords));
    return (ambient + diffuse + specular);
}

// calculates the color when using a point light.
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
    vec3 lightDir = normalize(light.position - fragPos);
    // diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    // specular shading
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 1.0); // material.shininess);
    // attenuation
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));
    // combine results
    vec3 ambient = light.ambient; // * vec3(texture(material.diffuse, TexCoords));
    vec3 diffuse = light.diffuse * diff; // * vec3(texture(material.diffuse, TexCoords));
    vec3 specular = light.specular * spec; // * vec3(texture(material.specular, TexCoords));
    ambient *= attenuation;
    diffuse *= attenuation;
    specular *= attenuation;
    return (ambient + diffuse + specular);
}

// calculates the color when using a spot light.
vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
    vec3 lightDir = normalize(light.position - fragPos);
    // diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    // specular shading
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 1.0); // material.shininess);
    // attenuation
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));
    // spotlight intensity
    float theta = dot(lightDir, normalize(-light.direction));
    float epsilon = light.cutOff - light.outerCutOff;
    float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);
    // combine results
    vec3 ambient = light.ambient; // * vec3(texture(material.diffuse, TexCoords));
    vec3 diffuse = light.diffuse * diff; // * vec3(texture(material.diffuse, TexCoords));
    vec3 specular = light.specular * spec; // * vec3(texture(material.specular, TexCoords));
    ambient *= attenuation * intensity;
    diffuse *= attenuation * intensity;
    specular *= attenuation * intensity;
    return (ambient + diffuse + specular);
}

void main()
{
	vec4 color = abs(normalize(world_normal)) + vec4(2.0, 2.0, 2.0, 1.0);

	vec3 norm = vec3(normalize(normal));
	vec3 viewDir = normalize(vec3(view_position) - vec3(world_position));

    for (int dLight = 0; dLight < dLights; dLight++) {
        fragment_color += vec4(CalcDirLight(directionalLights[dLight], norm, viewDir), 1.0);
    }

    for (int pLight = 0; pLight < pLights; pLight++) {
	   fragment_color += vec4(CalcPointLight(pointLights[pLight], norm, vec3(world_position), viewDir), 1.0);
    }

    for (int sLight = 0; sLight < sLights; sLight++) {
        fragment_color += vec4(CalcSpotLight(spotLights[sLight], norm, vec3(world_position), viewDir), 1.0);
    }

    fragment_color *= color;
}
)zzz"
