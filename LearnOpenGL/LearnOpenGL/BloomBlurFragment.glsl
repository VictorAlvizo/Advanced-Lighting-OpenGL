#version 330 core

in vec2 g_TexCoords;

uniform sampler2D u_Image;
uniform bool u_Horizontal;
uniform float weight[5] =  float[] (0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);

out vec4 fragColor;

void main() {
	vec2 texOffset = 1.0f / textureSize(u_Image, 0); //Size of a single texel
	vec3 result = texture(u_Image, g_TexCoords).rgb * weight[0];

	if(u_Horizontal) {
		for(int i = 1; i < 5; ++i) {
			result += texture(u_Image, g_TexCoords + vec2(texOffset.x * i, 0.0f)).rgb * weight[i];
			result += texture(u_Image, g_TexCoords - vec2(texOffset.x * i, 0.0f)).rgb * weight[i];
		}
	}else {
		for(int i = 1; i < 5; ++i) {
			result += texture(u_Image, g_TexCoords + vec2(0.0f, texOffset.y * i)).rgb * weight[i];
			result += texture(u_Image, g_TexCoords - vec2(0.0f, texOffset.y * i)).rgb * weight[i];
		}
	}

	fragColor = vec4(result, 1.0f);
}