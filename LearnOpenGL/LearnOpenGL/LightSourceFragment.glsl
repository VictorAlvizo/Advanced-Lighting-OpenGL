#version 330 core
layout(location = 0) out vec4 c_FragColor;
layout(location = 1) out vec4 c_BrightColor;

uniform vec3 u_Color;

out vec4 fragColor;

void main() {
	c_FragColor = vec4(u_Color, 1.0f);

	float brightness = dot(c_FragColor.rgb, vec3(0.2126, 0.7152, 0.0722));

	if(brightness > 1.0f) {
		c_BrightColor = vec4(c_FragColor.rgb, 1.0f);
	}else {
		c_BrightColor = vec4(0.0f, 0.0f, 0.0f, 1.0f);
	}
}