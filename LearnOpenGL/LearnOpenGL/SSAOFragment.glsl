#version 330 core

in vec2 g_TexCoords;

uniform sampler2D u_gbPos;
uniform sampler2D u_gbNormal;
uniform sampler2D u_TexNoise;

uniform vec3 u_Samples[64];
uniform mat4 u_Projection;

const vec2 noiseScale = vec2(800.0f / 4.0f, 600.0f / 4.0f);

out float fragColor;

void main() {
	vec3 fragPos = texture(u_gbPos, g_TexCoords).rgb;
	vec3 normal = normalize(texture(u_gbNormal, g_TexCoords).rgb);
	vec3 randomVec = normalize(texture(u_TexNoise, g_TexCoords * noiseScale).rgb);

	vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
	vec3 bitangent = cross(normal, tangent);
	mat3 TBN = mat3(tangent, bitangent, normal);

	float occlusion = 0.0f;
	for(int i = 0; i < 64; i++) {
		vec3 samplePos = TBN * u_Samples[i];
		samplePos = fragPos + samplePos * 0.5f;

		vec4 offset = vec4(samplePos, 1.0f);
		offset = u_Projection * offset;
		offset.xyz /= offset.w;
		offset.xyz = offset.xyz * 0.5f + 0.5f;

		float sampleDepth = texture(u_gbPos, offset.xy).z;
		float rangeCheck = smoothstep(0.0f, 1.0f, 0.5f / abs(fragPos.z - sampleDepth));
		occlusion += (sampleDepth >= samplePos.z + 0.025f ? 1.0f : 0.0f) * rangeCheck;
	}

	occlusion = 1.0f - (occlusion / 64);
	fragColor = occlusion;
}