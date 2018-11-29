R"zzz(#version 330 core
// An alternative blur shader which allows for specification of horizontal or vertical
// blur, good for multiple blur passes as described here:
// https://learnopengl.com/Advanced-Lighting/Bloom

in vec2 TexCoords;

uniform sampler2D screenTexture;

uniform bool horizontal;
uniform float weight[5] = float[] (0.2270270270, 0.1945945946, 0.1216216216, 0.0540540541, 0.0162162162);

out vec4 fragment_color;

void main()
{
     vec2 tex_offset = 1.0 / textureSize(screenTexture, 0); // gets size of single texel
     vec3 result = texture(screenTexture, TexCoords).rgb * weight[0];
     if(horizontal)
     {
         for(int i = 1; i < 5; ++i)
         {
            result += texture(screenTexture, TexCoords + vec2(tex_offset.x * i, 0.0)).rgb * weight[i];
            result += texture(screenTexture, TexCoords - vec2(tex_offset.x * i, 0.0)).rgb * weight[i];
         }
     }
     else
     {
         for(int i = 1; i < 5; ++i)
         {
             result += texture(screenTexture, TexCoords + vec2(0.0, tex_offset.y * i)).rgb * weight[i];
             result += texture(screenTexture, TexCoords - vec2(0.0, tex_offset.y * i)).rgb * weight[i];
         }
     }
     fragment_color = vec4(result, 1.0);
}
)zzz"
