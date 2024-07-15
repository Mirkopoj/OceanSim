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
	uint time;
}
ubo;

layout(push_constant) uniform Push {
   mat4 modelMatrix;
   mat4 normalMatrix;
}
push;

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

	float a = 1;
	float w = 0.1; 
	float f = 0.00000001;

	float t = ubo.time;

	float y = a * sin(x * w + t * f);

	float yn = a * cos(x * w + t * f);
	vec3 dx = normalize(vec3(1, yn, 0));
	vec3 dz = vec3(0, 0, 1);
	vec3 normal = cross(dx, dz);

	vec3 position = vec3(x, y, z);
   vec4 positionWorld = push.modelMatrix * vec4(position ,1.0);

   gl_Position = ubo.projection * ubo.view * positionWorld;

   fragNormalWorld = normalize(mat3(push.normalMatrix) * normal);
   fragPosWorld = position;
   fragColor = vec3(0, 0, 1);
}
