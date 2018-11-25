R"zzz(#version 330 core
in vec2 TexCoords;
uniform sampler2D screenTexture;

uniform int uGhosts; // number of ghost samples: set to 3
uniform float uGhostDispersal; // dispersion factor

out vec4 fragment_color;

// This fragment shader is the basis for doing lens flares, but it does trippy stuff
void main() {
  vec2 texcoord = -TexCoords + vec2(1.0);
  vec2 texelSize = 1.0 / vec2(textureSize(screenTexture, 0));

// ghost vector to image centre:
  vec2 ghostVec = (vec2(0.5) - texcoord) * 1; // uGhostDispersal

// sample ghosts:
  vec4 result = vec4(0.0);
  for (int i = 0; i < 3; ++i) { // number of ghost samples: set to 3
     vec2 offset = fract(texcoord + ghostVec * float(i));

     result += texture(screenTexture, offset);
  }

  fragment_color = result;
}
)zzz"
