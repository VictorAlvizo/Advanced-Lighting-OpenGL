#version 330 core
layout(location = 0) out vec3 gb_Pos;
layout(location = 1) out vec3 gb_Normal;
layout(location = 2) out vec4 gb_AlbedoSpec;
layout(location = 3) out vec3 gb_PosView;
layout(location = 4) out vec3 gb_NormalView;

in vec2 g_TexCoords;
in vec3 g_Normal;
in vec3 g_FragPos;
in vec3 g_FragPosViewSpace;
in vec3 g_NormalViewSpace;

#define MAX_TEXTURES 16

uniform sampler2D u_AlbedoTexture[MAX_TEXTURES];
uniform int u_AlbedoSize;

uniform sampler2D u_SpecularTexture[MAX_TEXTURES];
uniform int u_SpecularSize;

uniform sampler2D u_NormalTexture[MAX_TEXTURES];
uniform int u_NormalSize;

uniform float u_OpacityLimit;
uniform vec3 u_DayColor;

void main() {
	vec4 albedo = vec4(1.0f);
	for(int i = 0; i < u_AlbedoSize; i++) {
		albedo *= texture(u_AlbedoTexture[i], g_TexCoords);
	}

	float specValue = 1.0f;
	for(int i = 0; i < u_SpecularSize; i++) {
		specValue *= texture(u_SpecularTexture[i], g_TexCoords).r;
	}

	if(albedo.a < u_OpacityLimit) {
		discard;
	}

	gb_Pos = g_FragPos;
	gb_Normal = normalize(g_Normal);
	gb_AlbedoSpec.rgb = albedo.rgb * u_DayColor;
	gb_AlbedoSpec.a = specValue;
	gb_PosView = g_FragPosViewSpace;
	gb_NormalView = normalize(g_NormalViewSpace);
}