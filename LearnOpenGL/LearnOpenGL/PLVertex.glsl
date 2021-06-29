#version 330 core
layout(location = 0) in vec3 vs_Pos;

uniform mat4 u_Model;

void main() {
	gl_Position = u_Model * vec4(vs_Pos, 1.0f);
}