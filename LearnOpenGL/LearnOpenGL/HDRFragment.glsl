#version 330 core

in vec2 g_TexCoords;

uniform sampler2D u_Scene;
uniform sampler2D u_BloomBlur;

uniform float u_Exposure;

out vec4 fragColor;

void main() {
	const float gamma = 2.2f;
	vec3 hdrColor = texture(u_Scene, g_TexCoords).rgb;
	vec3 bloomColor = texture(u_BloomBlur, g_TexCoords).rgb;

	hdrColor += bloomColor;

	vec3 result = vec3(1.0f) - exp(-hdrColor * u_Exposure);
	result = pow(result, vec3(1.0f / gamma));
	fragColor = vec4(result, 1.0f);
}