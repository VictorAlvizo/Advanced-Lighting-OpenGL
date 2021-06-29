#version 330 core

in vec2 g_TexCoords;

uniform sampler2D u_DepthMap;
uniform float u_NearPlane;
uniform float u_FarPlane;
uniform bool u_Perspective;

out vec4 fragColor;

float LinearizeDepth(float depth) {
	float z = depth * 2.0f - 1.0f;
	return (2.0f * u_NearPlane * u_FarPlane) / (u_FarPlane + u_NearPlane - z * (u_FarPlane - u_NearPlane));
}

void main() {
	float depthValue = texture(u_DepthMap, g_TexCoords).r;
	fragColor = (u_Perspective) ? vec4(vec3(LinearizeDepth(depthValue) / u_FarPlane), 1.0f) : vec4(vec3(depthValue), 1.0f);
}