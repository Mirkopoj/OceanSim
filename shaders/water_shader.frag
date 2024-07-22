#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragPosWorld;
layout(location = 2) in vec3 fragNormalWorld;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform GloablUbo {
   mat4 projection;
   mat4 view;
   vec4 ambientLightColor;
	vec3 lightPosition;
	uint cols;
	uint time;
}
ubo;

vec3 sub_surface_scatering() {
	float H = max(0, fragPosWorld.y);
	vec4 k = vec4(1.0, 1.0, 1.0, 1.0);
	vec3 Css = vec3(0, 0, 1);
	vec3 Cf = vec3(1, 1, 1);
	float Pf = 0.5;
	return (k.x + k.y) * Css + k.z * Css + k.w * Pf * Cf; 
}

void main() {
	vec3 lightColor = ubo.ambientLightColor.xyz;
   vec3 ambientLight = ubo.ambientLightColor.xyz * ubo.ambientLightColor.w;

   vec3 diffuseLight = lightColor * max(dot(normalize(fragNormalWorld), normalize(ubo.lightPosition)),0.0);
	//vec3 scatteredLight = sub_surface_scatering() * diffuseLight;

   outColor = vec4((diffuseLight + ambientLight) * fragColor, 1.0);
}
