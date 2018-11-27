R"zzz(#version 330 core
in vec2 TexCoords;
uniform sampler2D screenTexture;

uniform float uGhosts; // number of ghost samples: set to 3
uniform float uGhostDispersal; // dispersion factor
uniform float uHaloWidth; //used to simulate halo on a lens
uniform sampler2D uLensColor;

out vec4 fragment_color;

// This fragment shader is the basis for doing lens flares
void main() {
  vec2 texcoord = -TexCoords + vec2(1.0);
  vec2 texelSize = 1.0 / vec2(textureSize(screenTexture, 0));

// ghost vector to image centre:
  vec2 ghostVec = (vec2(0.5) - texcoord) * uGhostDispersal;

// sample ghosts:
  vec4 result = vec4(0.0);
  for (int i = 0; i < uGhosts; ++i) { // number of ghost samples: set to 3
     vec2 offset = fract(texcoord + ghostVec * float(i));

     float weight = length(vec2(0.5) - offset) / length(vec2(0.5));
     weight = pow(1.0 - weight, 10.0);

     result += texture(screenTexture, offset);

     // sample halo:
     vec2 haloVec = normalize(ghostVec) * uHaloWidth;
     weight = length(vec2(0.5) - fract(texcoord + haloVec)) / length(vec2(0.5));
     weight = pow(1.0 - weight, 5.0);
     result += texture(screenTexture, texcoord + haloVec) * weight;
  }

  // result = texture(uLensColor, texcoord);

  fragment_color = result;
}
)zzz"
