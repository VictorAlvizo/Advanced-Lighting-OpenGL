#version 330 core
layout(location = 0) out vec4 c_FragColor;
layout(location = 1) out vec4 c_BrightColor;

struct DirLight {
	vec3 m_Direction;
	vec3 m_Color;
	float m_ShinySpec;

	vec3 m_Diffuse;
	vec3 m_Specular;
};

struct PointLight {
	vec3 m_Pos;
	vec3 m_Color;

	float m_SpecShine;
};

struct SpotLight {
	vec3 m_Pos;
	vec3 m_Color;
	vec3 m_Direction;
	float m_Cutoff;
	float m_Outercutoff;
};

#define PL_SIZE 2

in vec2 g_TexCoords;

uniform sampler2D u_gbPos;
uniform sampler2D u_gbNormal;
uniform sampler2D u_gbAlbedoSpec;
uniform sampler2D u_SSAOTexture;
uniform sampler2D u_DirShadowMap;
uniform sampler2D u_SpotlightShadowMap;
uniform samplerCube u_CubeDepth[PL_SIZE];

uniform SpotLight u_Spotlight;
uniform mat4 u_SpotlightMatrix;

uniform PointLight u_PointsLights[PL_SIZE];
uniform float u_FarPlane;

uniform DirLight u_Dirlight;
uniform mat4 u_DirLightMatrix;

uniform vec3 u_ViewPos;

out vec4 fragColor;

// array of offset direction for sampling
vec3 gridSamplingDisk[20] = vec3[]
(
   vec3(1, 1,  1), vec3( 1, -1,  1), vec3(-1, -1,  1), vec3(-1, 1,  1), 
   vec3(1, 1, -1), vec3( 1, -1, -1), vec3(-1, -1, -1), vec3(-1, 1, -1),
   vec3(1, 1,  0), vec3( 1, -1,  0), vec3(-1, -1,  0), vec3(-1, 1,  0),
   vec3(1, 0,  1), vec3(-1,  0,  1), vec3( 1,  0, -1), vec3(-1, 0, -1),
   vec3(0, 1,  1), vec3( 0, -1,  1), vec3( 0, -1, -1), vec3( 0, 1, -1)
);

float ShadowCalcCubemap(vec3 fragPos, vec3 lightPos, int depthIndex) {
	vec3 fragToLight = fragPos - lightPos;
	float currentDepth = length(fragToLight);

	float shadow = 0.0f;
	float bias = 0.15f;
	int samples = 20;
	float viewDistance = length(u_ViewPos - fragPos);
	float diskRadius = (1.0f + (viewDistance / u_FarPlane)) / 25.0f;

	for(int i = 0; i < samples; i++) {
		float closestDepth = texture(u_CubeDepth[depthIndex], fragToLight + gridSamplingDisk[i] * diskRadius).r;
		closestDepth *= u_FarPlane;

		if(currentDepth - bias > closestDepth) {
			shadow += 1.0f;
		}
	}

	shadow /= float(samples);
	return shadow;
}

float ShadowCalc(vec4 fragPosLight, sampler2D shadowMap) {
	vec3 projCoords = fragPosLight.xyz / fragPosLight.w;
	projCoords = projCoords * 0.5f + 0.5f;
	float currentDepth = projCoords.z;

	float shadow = 0.0f;
	vec2 texelSize = 1.0f / textureSize(shadowMap, 0);
	for(int x = -1; x <= 1; ++x) {
		for(int y = -1; y <= 1; ++y) {
			float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
			shadow += currentDepth - 0.05f > pcfDepth ? 1.0f : 0.0f;
		}
	}

	shadow /= 9.0f;

	if(projCoords.z > 1.0f) {
		shadow = 0.0f;
	}

	return shadow;
}

vec3 CalculateSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 albedo, vec4 lightFragPos) {
	vec3 lightDir = normalize(light.m_Pos - fragPos);

	float diff = max(dot(lightDir, normal), 0.0f);
	vec3 diffuse = diff * albedo * light.m_Color;

	vec3 halfwayDir = normalize(lightDir + viewDir);
	float spec = pow(max(dot(halfwayDir, normal), 0.0f), 64.0f);
	vec3 specular = spec * light.m_Color * 0.3f;

	float theta = dot(lightDir, normalize(-light.m_Direction));
	float epsilon = (light.m_Cutoff - light.m_Outercutoff);
	float intensity = clamp((theta - light.m_Outercutoff) / epsilon, 0.0f, 1.0f);
	diffuse *= intensity;
	specular *= intensity;

	float shadow = ShadowCalc(lightFragPos, u_SpotlightShadowMap);
	return (1.0f - shadow) * (diffuse + specular);
}

vec3 CalculatePointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 albedo, vec4 fragPosLight, vec4 spotFragPosLight, int depthIndex) {
	vec3 lightDir = normalize(light.m_Pos - fragPos);
	float diff = max(dot(lightDir, normal), 0.0f);
	vec3 diffuse = diff * albedo * light.m_Color;

	vec3 halfwayDir = normalize(lightDir + viewDir);
	float spec = pow(max(dot(halfwayDir, normal), 0.0f), light.m_SpecShine);
	vec3 specular = spec * light.m_Color * 0.1f;

	float distance = length(fragPos - light.m_Pos);
	float attenuation = 1.0f / (distance * distance);

	diffuse *= attenuation;
	specular *= attenuation;

	float shadowPL = ShadowCalcCubemap(fragPos, light.m_Pos, depthIndex);
	float shadowDir = ShadowCalc(fragPosLight, u_DirShadowMap);

	float shadow = (shadowPL >= shadowDir) ? shadowPL : shadowDir;
	return (1.0f - shadow) * (diffuse + specular);	
}

vec3 CalculateDirLight(DirLight light, vec3 normal, vec3 viewDir, vec3 albedo, vec4 fragPosLight) {
	vec3 lightDir = normalize(-light.m_Direction);
	float diff = max(dot(lightDir, normal), 0.0f);
	vec3 diffuse = light.m_Diffuse * diff * albedo * light.m_Color;

	vec3 halfwayDir = normalize(lightDir + viewDir);
	float spec = pow(max(dot(halfwayDir, normal), 0.0f), light.m_ShinySpec);
	vec3 specular = light.m_Specular * spec * light.m_Color;

	float shadowVal = ShadowCalc(fragPosLight, u_DirShadowMap);
	return (1.0f - shadowVal) * (diffuse + specular);
}

void main() {
	vec3 fragPos = texture(u_gbPos, g_TexCoords).rgb;
	vec3 normal = texture(u_gbNormal, g_TexCoords).rgb;
	vec3 albedo = texture(u_gbAlbedoSpec, g_TexCoords).rgb;
	float specVal = texture(u_gbAlbedoSpec, g_TexCoords).a;
	float ssaoOcclusion = texture(u_SSAOTexture, g_TexCoords).r;

	vec4 dirLightFragPos = u_DirLightMatrix * vec4(fragPos, 1.0f);
	vec4 spotLightFragPos = u_SpotlightMatrix * vec4(fragPos, 1.0f);

	vec3 viewDir = normalize(u_ViewPos - fragPos);

	vec3 ambient = vec3(0.1f * albedo * ssaoOcclusion);

	vec3 lighting = vec3(0.0f);
	for(int i = 0; i < PL_SIZE; i++) {
		lighting += CalculatePointLight(u_PointsLights[i], normal, fragPos, viewDir, albedo, dirLightFragPos, spotLightFragPos, i);
	}

	lighting += CalculateDirLight(u_Dirlight, normal, viewDir, albedo, dirLightFragPos);
	lighting += CalculateSpotLight(u_Spotlight, normal, fragPos, viewDir, albedo, spotLightFragPos);

	vec3 result = ambient + lighting;
	
	float brightness = dot(result, vec3(0.2126, 0.7152, 0.0722));

	if(brightness > 1.0f) {
		c_BrightColor = vec4(result, 1.0f);
	} else {
		c_BrightColor = vec4(0.0f, 0.0f, 0.0f, 1.0f);
	}

	c_FragColor = vec4(result, 1.0f);
}