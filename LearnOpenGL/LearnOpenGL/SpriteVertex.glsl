#version 330 core
layout(location = 0) in vec2 vs_Pos;
layout(location = 1) in vec2 vs_TexCoords;

out vec2 g_TexCoords;

uniform mat4 u_Projection;
uniform mat4 u_Model;

void main() {

	gl_Position = u_Projection * u_Model * vec4(vs_Pos, 0.0f, 1.0f);
	g_TexCoords = vs_TexCoords;
}