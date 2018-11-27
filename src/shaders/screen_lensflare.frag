R"zzz(#version 330 core
in vec2 TexCoords;
uniform sampler2D screenTexture;

uniform float uGhosts; // number of ghost samples: set to 3
uniform float uGhostDispersal; // dispersion factor
uniform float uHaloWidth; //used to simulate halo on a lens
uniform float uDistortion;
uniform sampler1D uLensColor;

out vec4 fragment_color;


vec4 textureDistorted(
      in sampler2D tex,
      in vec2 texcoord,
      in vec2 direction, // direction of distortion
      in vec3 distortion // per-channel distortion factor
   ) {
  return vec4(
     texture(tex, texcoord + direction * distortion.r).r,
     texture(tex, texcoord + direction * distortion.g).g,
     texture(tex, texcoord + direction * distortion.b).b,
     0.0
  );
}


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

     //vec2 texelSize = 1.0 / vec2(textureSize(screenTexture, 0));
     //vec3 distortion = vec3(-texelSize.x * uDistortion, 0.0, texelSize.x * uDistortion);
     //vec2 direction = normalize(ghostVec);

     //result += textureDistorted(screenTexture, texelSize, direction, distortion);

     result += texture(screenTexture, offset);

     // sample halo:
     vec2 haloVec = normalize(ghostVec) * uHaloWidth;
     weight = length(vec2(0.5) - fract(texcoord + haloVec)) / length(vec2(0.5));
     weight = pow(1.0 - weight, 5.0);
     result += texture(screenTexture, texcoord + haloVec) * weight;
  }

  result *= texture(uLensColor, length(vec2(0.5) - texcoord) / length(vec2(0.5)));

  fragment_color = result;
}
)zzz"
