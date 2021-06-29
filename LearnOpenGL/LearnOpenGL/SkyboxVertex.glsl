#version 330 core
layout(location = 0) in vec3 vs_Pos;

uniform mat4 u_Projection;
uniform mat4 u_View;

out vec3 g_TexCoords;

void main() {
	vec4 pos = u_Projection * u_View * vec4(vs_Pos, 1.0f);
	gl_Position = pos.xyww;
	g_TexCoords = vs_Pos;
}