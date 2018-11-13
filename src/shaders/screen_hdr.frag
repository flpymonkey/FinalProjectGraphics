R"zzz(#version 330 core
out vec4 fragment_color;
in vec2 TexCoords;
uniform sampler2D screenTexture;

uniform float exposure;

// hdr shader which uses Reinhard tone mapping for high dynamic range
void main()
{
		const float gamma = 2.2;
    vec3 hdrColor = texture(screenTexture, TexCoords).rgb;

		// Exposure tone mapping
    vec3 mapped = vec3(1.0) - exp(-hdrColor * exposure);
    // gamma correction
    mapped = pow(mapped, vec3(1.0 / gamma));

    fragment_color = vec4(mapped, 1.0);
}
)zzz"
