#version 330 core
layout(location = 0) in vec3 vs_Pos;
layout(location = 1) in vec3 vs_Normal;
layout(location = 2) in vec2 vs_TexCoords;
layout(location = 3) in vec3 vs_Tangent;
layout(location = 4) in vec3 vs_Bitangent;

layout(std140) uniform Matrix {
	mat4 u_Projection;
	mat4 u_View;
};

uniform mat4 u_Model;

uniform bool u_ViewApplied;

out vec2 g_TexCoords;
out vec3 g_Normal;
out vec3 g_FragPos;
out vec3 g_FragPosViewSpace;
out vec3 g_NormalViewSpace;

void main() {
	g_TexCoords = vs_TexCoords;

	if(u_ViewApplied) {
		gl_Position = u_Projection * u_View * u_Model * vec4(vs_Pos, 1.0f);
		g_Normal = mat3(transpose(inverse(u_Model))) * vs_Normal;
		g_FragPos = (u_Model * vec4(vs_Pos, 1.0f)).xyz;
		g_FragPosViewSpace = (u_View * u_Model * vec4(vs_Pos, 1.0f)).xyz;
		g_NormalViewSpace = mat3(transpose(inverse(u_View * u_Model))) * vs_Normal;
	}else {
		gl_Position = u_Projection * u_Model * vec4(vs_Pos, 1.0f);
		g_Normal = mat3(transpose(inverse(u_View * u_Model))) * vs_Normal;
		g_FragPos = (u_View * u_Model * vec4(vs_Pos, 1.0f)).xyz;
		g_FragPosViewSpace = (u_Model * vec4(vs_Pos, 1.0f)).xyz;;
		g_NormalViewSpace = mat3(transpose(inverse(u_Model))) * vs_Normal;;
	}
}