#version 450

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 fragPosWorld;
layout(location = 2) out vec3 fragNormalWorld;

layout(set = 0, binding = 0) uniform GloablUbo {
   mat4 projection;
   mat4 view;
   vec4 ambientLightColor;
	vec3 lightPosition;
	uint cols;
	float time;
}
ubo;

layout(set = 1, binding = 0, rgba16f) uniform readonly image2D Displacement_Turbulence;
layout(set = 1, binding = 1, rgba16f) uniform readonly image2D Derivatives;

void main() {
	float i = gl_VertexIndex;
	float xn = ubo.cols;
	float n = 4 * xn - 2;
	float r = mod(i, n);
	float c = floor(r / 2);
	float d = mod(floor(c / xn), 2);
	float s = 1 - 2 * d;

	float z = s * mod(i, 2) + floor(c / xn) * 2 + floor(i / n) * 2;
	float x = d * (xn - 1) + s * mod(floor((r + d) / 2), xn);

	ivec2 id = ivec2(x, z);

	vec4 derivatives = imageLoad(Derivatives, id);
	vec2 slope = vec2(derivatives.x / (1 + derivatives.z),
                derivatives.y / (1 + derivatives.w));
   vec3 normal = vec3(-slope.x, -1, -slope.y);

	vec3 position = vec3(x, 0, z)
		+ imageLoad(Displacement_Turbulence, id).xyz;
   vec4 positionWorld = vec4(position, 1.0);

   gl_Position = ubo.projection * ubo.view * positionWorld;

   fragNormalWorld = normalize(normal);
   fragPosWorld = position;
   fragColor = vec3(0, 0, 1);
}
