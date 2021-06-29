#version 330 core

in vec3 g_TexCoords;

uniform samplerCube u_SkyboxTexture;
uniform vec3 u_DayColor;

out vec4 fragColor;

void main() {
	fragColor = texture(u_SkyboxTexture, g_TexCoords) * vec4(u_DayColor, 1.0f);
}