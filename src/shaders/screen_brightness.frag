R"zzz(#version 330 core
in vec2 TexCoords;
uniform sampler2D screenTexture;

out vec4 fragment_color;
out vec4 bright_color;

void main()
{
    vec3 result = texture(screenTexture, TexCoords).rgb;

    // check whether result is higher than some threshold, if so, output as bloom threshold color
    float brightness = dot(result, vec3(0.2126, 0.7152, 0.0722));
    if(brightness > 1.0)
        bright_color = vec4(result, 1.0);
    else
        bright_color = vec4(0.0, 0.0, 0.0, 1.0);
        
    fragment_color = vec4(result, 1.0);
}
)zzz"
