R"zzz(#version 330 core
in vec2 TexCoords;
uniform sampler2D screenTexture;

uniform vec4 uScale;
uniform vec4 uBias;

out vec4 fragment_color;

void main() {
  fragment_color = max(vec4(0.0), texture(screenTexture, TexCoords) + 0.4) * 0.5;
}
)zzz"
