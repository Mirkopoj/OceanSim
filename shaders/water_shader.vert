#version 450

layout(location = 0) out vec3 fragPosWorld;
layout(location = 1) out vec2 vertPos;
layout(location = 2) out vec3 camPosWorld;
layout(location = 3) out mat4 view;

layout(set = 0, binding = 0) uniform GloablUbo {
   mat4 projection;
   mat4 view;
   mat4 invView;
   vec4 sunColor;
	vec4 scatterColor;
	vec4 bubbleColor;
	vec3 lightPosition;
	uint cols;
	float time;
	uint caminata;
} ubo;

struct CompUboIner
{
	float LengthScale;
	float CutoffHigh;
	float CutoffLow;
	float GravityAcceleration;
	float Depth;
	uint Size;
};

layout(set = 1, binding = 0) buffer CompUbo {
	CompUboIner data[4];
} comp_ubo;

layout(set = 1, binding = 1) uniform sampler2D Displacement_Turbulence0;
layout(set = 1, binding = 2) uniform sampler2D Derivatives0;

layout(set = 1, binding = 3) uniform sampler2D Displacement_Turbulence1;
layout(set = 1, binding = 4) uniform sampler2D Derivatives1;

layout(set = 1, binding = 5) uniform sampler2D Displacement_Turbulence2;
layout(set = 1, binding = 6) uniform sampler2D Derivatives2;

layout(set = 1, binding = 7) uniform sampler2D Displacement_Turbulence3;
layout(set = 1, binding = 8) uniform sampler2D Derivatives3;

void main() {
	uint plg = gl_VertexIndex / 3;
	uint gvi = gl_VertexIndex % 3;
	float i = gvi + plg;
	float xn = ubo.cols;
	float n = 4 * xn - 2;
	float r = mod(i, n);
	float c = floor(r / 2);
	float d = mod(floor(c / xn), 2);
	float s = 1 - 2 * d;

	float z = s * mod(i, 2) + floor(c / xn) * 2 + floor(i / n) * 2;
	float x = d * (xn - 1) + s * mod(floor((r + d) / 2), xn);

	vec2 id = vec2(x, z) * 5;

	vec3 position = vec3(id.x, 0, id.y)
		+ texture(Displacement_Turbulence0, id / comp_ubo.data[0].LengthScale).xyz
		+ texture(Displacement_Turbulence1, id / comp_ubo.data[1].LengthScale).xyz
		+ texture(Displacement_Turbulence2, id / comp_ubo.data[2].LengthScale).xyz
		+ texture(Displacement_Turbulence3, id / comp_ubo.data[3].LengthScale).xyz;
   vec4 positionWorld = vec4(position, 1.0);

	mat4 l_view = ubo.view;
	vec3 cpos = ubo.invView[3].xyz;
	if (ubo.caminata != 0){
		vec3 u = vec3(ubo.view[0][0], ubo.view[1][0], ubo.view[2][0]);
		vec3 v = vec3(ubo.view[0][1], ubo.view[1][1], ubo.view[2][1]);
		vec3 w = vec3(ubo.view[0][2], ubo.view[1][2], ubo.view[2][2]);
		cpos = vec3(cpos.x, -2, cpos.z)
			+ texture(Displacement_Turbulence0, cpos.xz / comp_ubo.data[0].LengthScale).xyz
			+ texture(Displacement_Turbulence1, cpos.xz / comp_ubo.data[1].LengthScale).xyz
			+ texture(Displacement_Turbulence2, cpos.xz / comp_ubo.data[2].LengthScale).xyz
			+ texture(Displacement_Turbulence3, cpos.xz / comp_ubo.data[3].LengthScale).xyz;
		l_view[3] = vec4(-dot(u, cpos), -dot(v, cpos), -dot(w, cpos), 1);
	}

   gl_Position = ubo.projection * l_view * positionWorld;

   fragPosWorld = position;
	vertPos = id;
	view = l_view;
	camPosWorld = cpos;
}
