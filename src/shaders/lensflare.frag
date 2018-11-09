// Using code from http://john-chapman-graphics.blogspot.com/2013/02/pseudo-lens-flare.html

uniform sampler2D uInputTex;

uniform int uGhosts; // number of ghost samples
uniform float uGhostDispersal; // dispersion factor

noperspective in vec2 vTexcoord;

out vec4 fResult;

void main() {
  vec2 texcoord = -vTexcoord + vec2(1.0);
  vec2 texelSize = 1.0 / vec2(textureSize(uInputTex, 0));

// ghost vector to image centre:
  vec2 ghostVec = (vec2(0.5) - texcoord) * uGhostDispersal;

// sample ghosts:
  vec4 result = vec4(0.0);
  for (int i = 0; i < uGhosts; ++i) {
     vec2 offset = fract(texcoord + ghostVec * float(i));

     result += texture(uInputTex, offset);
  }

  fResult = result;
}
