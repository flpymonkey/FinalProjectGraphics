R"zzz(#version 330 core
in vec2 TexCoords;
uniform sampler2D screenTexture;

uniform float uGhosts; // number of ghost samples: set to 3
uniform float uGhostDispersal; // dispersion factor
uniform sampler2D uLensColor;

out vec4 fragment_color;

// This fragment shader is the basis for doing lens flares, but it does trippy stuff
void main() {
  vec2 texcoord = -TexCoords + vec2(1.0);
  vec2 texelSize = 1.0 / vec2(textureSize(screenTexture, 0));

// ghost vector to image centre:
  vec2 ghostVec = (vec2(0.5) - texcoord) * 0.25;

// sample ghosts:
  vec4 result = vec4(0.0);
  for (int i = 0; i < 3; ++i) { // number of ghost samples: set to 3
     vec2 offset = fract(texcoord + ghostVec * float(i));

      float weight = length(vec2(0.5) - offset) / length(vec2(0.5));
      weight = pow(1.0 - weight, 10.0);

     result += texture(screenTexture, offset);
  }

  result *= texture(uLensColor, length(vec2(0.5) - texcoord) / length(vec2(0.5)));

  fragment_color = result;
}
)zzz"
