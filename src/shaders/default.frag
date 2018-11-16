R"zzz(#version 330 core
in vec4 normal;
in vec4 light_direction;
in vec4 world_normal;
in vec4 world_position;

uniform vec4 view_position;
//uniform PointLight pointLights[1];

//out vec4 fragment_color;

// Source: https://learnopengl.com/Lighting/Multiple-lights

struct Material {
    sampler2D diffuse;
    sampler2D specular;
    float shininess;
}; 

struct DirLight {
    vec3 direction;
	
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct PointLight {
    vec3 position;

    float constant;
    float linear;
    float quadratic;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct SpotLight {
    vec3 position;
    vec3 direction;
    float cutOff;
    float outerCutOff;
  
    float constant;
    float linear;
    float quadratic;
  
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;       
};

uniform PointLight pointLights[1];
out vec4 fragment_color;

// calculates the color when using a directional light.
vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir)
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
	//PointLight light;
	//light.position = vec3(5.0, 5.0, 5.0);
	//light.ambient = vec3(0.05, 0.05, 0.05);
	//light.diffuse = vec3(0.8, 0.8, 0.8);
	//light.specular = vec3(1.0, 1.0, 1.0);
	//light.constant = 1.0;
	//light.linear = 0.09;
	//light.quadratic = 0.032;

	vec4 color = abs(normalize(world_normal)) + vec4(0.0, 0.0, 0.0, 1.0);

	vec3 norm = vec3(normalize(normal));
	vec3 viewDir = normalize(vec3(view_position) - vec3(world_position));
	fragment_color = vec4(CalcPointLight(pointLights[0], norm, vec3(world_position), viewDir), 1.0) * color;
}
)zzz"
