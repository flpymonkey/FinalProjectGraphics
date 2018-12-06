R"zzz(#version 330 core
out vec4 fragment_color;
in vec2 TexCoords;
uniform sampler2D screenTexture;
uniform sampler2D lensEffect;
uniform sampler2D lensEffect2;
void main()
{
    vec3 col = texture(screenTexture, TexCoords).rgb +
      texture(lensEffect, TexCoords).rgb + texture(lensEffect2, TexCoords).rgb;
    fragment_color = vec4(col, 1.0);
}
)zzz"
