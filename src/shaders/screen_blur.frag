R"zzz(#version 330 core
uniform sampler2D screenTexture;

uniform float offset[3] = float[]( 0.0, 1.3846153846, 3.2307692308 );
uniform float weight[3] = float[]( 0.1, 0.02, 0.1 );

out vec4 fragment_color;

void main(void)
{
    fragment_color = texture2D( screenTexture, vec2(gl_FragCoord)/1024.0 ) * weight[0];
    for (int i=1; i<3; i++) {
        fragment_color +=
            texture2D( screenTexture, ( vec2(gl_FragCoord)+vec2(0.0, offset[i]) )/1024.0 )
                * weight[i];
        fragment_color +=
            texture2D( screenTexture, ( vec2(gl_FragCoord)-vec2(0.0, offset[i]) )/1024.0 )
                * weight[i];
    }
}
)zzz"
