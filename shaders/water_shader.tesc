#version 450

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

layout(location = 0) in vec3 ifragPosWorld[];
layout(location = 1) in vec2 ivertPos[];

layout(vertices = 3) out;
layout(location = 0) out vec2 overtPos[];

bool frustumCheck(int index) {
	const float radius = 2.0f;
	vec4 pos = gl_in[index].gl_Position;
	vec2 ndc = pos.xy/pos.w;
	return -radius < ndc.x && ndc.x < radius 
		 && -radius < ndc.y && ndc.y < radius;
}

float tessellationLevel(vec3 fragPosWorld) {
   vec3 cameraPosWorld = ubo.invView[3].xyz;
   float dist = length(cameraPosWorld - fragPosWorld);
	return clamp((150 - dist) / 10, 0.1, 30.0);
}

void main() {
	if (gl_InvocationID == 0) {
		if (frustumCheck(0) || frustumCheck(1) || frustumCheck(2)){
			vec3 fragPosWorld = (ifragPosWorld[1] + ifragPosWorld[2]) / 2;
			gl_TessLevelOuter[0] = tessellationLevel(fragPosWorld);
			fragPosWorld = (ifragPosWorld[2] + ifragPosWorld[0]) / 2;
			gl_TessLevelOuter[1] = tessellationLevel(fragPosWorld);
			fragPosWorld = (ifragPosWorld[0] + ifragPosWorld[1]) / 2;
			gl_TessLevelOuter[2] = tessellationLevel(fragPosWorld);
			gl_TessLevelInner[0] = (gl_TessLevelOuter[0] + gl_TessLevelOuter[1] + gl_TessLevelOuter[2]) / 3;
		} else {
			gl_TessLevelInner[0] = 0.0;
			gl_TessLevelOuter[0] = 0.0;
			gl_TessLevelOuter[1] = 0.0;
			gl_TessLevelOuter[2] = 0.0;
		}
	}
	gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
	overtPos[gl_InvocationID] = ivertPos[gl_InvocationID];
}

