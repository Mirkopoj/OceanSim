#version 450

const float PI = 3.1415926;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragPosWorld;
layout(location = 2) in vec3 fragNormalWorld;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform GloablUbo {
   mat4 projection;
   mat4 view;
   mat4 invView;
   vec4 ambientLightColor;
	vec3 lightPosition;
	uint cols;
	float time;
}
ubo;

float DotClamped (vec3 a, vec3 b) {
	return max(0.0, dot(a, b));
}

float SmithMaskingBeckmann(vec3 H, vec3 S, float roughness) {
	float hdots = max(0.001f, DotClamped(H, S));
	float a = hdots / (roughness * sqrt(1 - hdots * hdots));
	float a2 = a * a;

	return a < 1.6f ? (1.0f - 1.259f * a + 0.396f * a2) / (3.535f * a + 2.181 * a2) : 0.0f;
}

float Beckmann(float ndoth, float roughness) {
	float exp_arg = (ndoth * ndoth - 1) / (roughness * roughness * ndoth * ndoth);

	return exp(exp_arg) / (PI * roughness * roughness * ndoth * ndoth * ndoth * ndoth);
}

void main() {
	vec3 bubbleColor = vec3(0, 0, 1);
	float bubbleDensity = 0.2;
	float roughness = 0.1;
	float foam_roughness_modifier = 1.0;
	float height_modifier = 1.0;
	float wave_peak_scatter_strength = 1.0;
	float scatter_strength = 1.0;
	float scatter_shadow_strength = 1.0;
	float environment_light_strength = 1.0;
	float normal_depth_falloff = 1.0f;

	vec3 lightColor = ubo.ambientLightColor.xyz;
   vec3 ambientLight = ubo.ambientLightColor.xyz * ubo.ambientLightColor.w;

   vec3 diffuseLight = lightColor * max(dot(normalize(fragNormalWorld), normalize(ubo.lightPosition)),0.0);
	vec3 lightDir = normalize(ubo.lightPosition);
   vec3 cameraPosWorld = ubo.invView[3].xyz;
   vec3 viewDir = normalize(cameraPosWorld - fragPosWorld);
   vec3 halfwayDir = normalize(lightDir + viewDir);
	float depth = gl_FragCoord.z;

	vec3 macroNormal = vec3(0, -1, 0);
	vec3 mesoNormal = fragNormalWorld;
	mesoNormal = normalize(mix(vec3(0, 1, 0), mesoNormal, pow(clamp(depth, 0.0, 1.0), normal_depth_falloff)));

	float NdotL = DotClamped(mesoNormal, lightDir);

	float a = roughness ;//+ foam * foam_roughness_modifier;
	float ndoth = max(0.0001f, dot(mesoNormal, halfwayDir));

	float viewMask = SmithMaskingBeckmann(halfwayDir, viewDir, a);
	float lightMask = SmithMaskingBeckmann(halfwayDir, lightDir, a);
	
	float G = 1.0/(1.0 + viewMask + lightMask);

	float eta = 1.33f;
	float R = ((eta - 1) * (eta - 1)) / ((eta + 1) * (eta + 1));
	float thetaV = acos(viewDir.y);

	float numerator = pow(1 - dot(mesoNormal, viewDir), 5 * exp(-2.69 * a));
	float F = R + (1 - R) * numerator / (1.0f + 22.7f * pow(a, 1.5f));
	F = clamp(F, 0.0, 1.0);
	
	vec3 specular = lightColor * F * G * Beckmann(ndoth, a);
	specular /= 4.0f * max(0.001f, DotClamped(macroNormal, lightDir));
	specular *= DotClamped(mesoNormal, lightDir);

	/*vec3 envReflection = texCUBE(_EnvironmentMap, reflect(-viewDir, mesoNormal)).rgb;
	envReflection *= environment_light_strength;*/

	float H = max(0.0f, -fragPosWorld.y) * height_modifier;
	
	float k1 = wave_peak_scatter_strength * H * pow(DotClamped(lightDir, -viewDir), 4.0f) * pow(0.5f - 0.5f * dot(lightDir, mesoNormal), 3.0f);
	float k2 = scatter_strength * pow(DotClamped(viewDir, mesoNormal), 2.0f);
	float k3 = scatter_shadow_strength * NdotL;
	float k4 = bubbleDensity;

	vec3 scatteredLight = (k1 + k2) * fragColor * (1.0/(1 + lightMask));
	scatteredLight += k3 * fragColor + k4 * bubbleColor;
	scatteredLight *= lightColor;

	vec3 out_color = (1 - F) * scatteredLight + specular;// + F * envReflection;
	out_color = max(vec3(0, 0, 0), out_color);
   outColor = vec4(out_color, 1.0);
}
