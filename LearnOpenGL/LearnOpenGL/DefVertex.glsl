#version 330 core
layout(location = 0) in vec3 vs_Pos;
layout(location = 1) in vec2 vs_TexCoords;

out vec2 g_TexCoords;

void main() {
	gl_Position = vec4(vs_Pos, 1.0f);
	g_TexCoords = vs_TexCoords;
}