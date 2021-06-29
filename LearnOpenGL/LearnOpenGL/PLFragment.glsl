#version 330 core

in vec4 fragPos;

uniform vec3 u_LightPos;
uniform float u_FarPlane;

void main() {
	float lightDistance = length(fragPos.xyz - u_LightPos);
	lightDistance = lightDistance / u_FarPlane;
	gl_FragDepth = lightDistance;
}