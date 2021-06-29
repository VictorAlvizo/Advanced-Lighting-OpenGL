#version 330 core

in vec2 g_TexCoords;

uniform sampler2D u_SpriteTexture;
uniform vec3 u_Color;

out vec4 fragColor;

void main() {
	fragColor = texture(u_SpriteTexture, g_TexCoords) * vec4(u_Color, 1.0f);
}