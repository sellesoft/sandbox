#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 0) uniform UniformBufferObject{
	mat4  view;
	mat4  proj;
	vec4  lights[10];
	vec4  viewPos;
	vec2  screen;
	vec2  mousepos;
	vec3  mouseWorld;
	float time;
} ubo;

layout(push_constant) uniform PushConsts{
	mat4 model;
} primitive;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec4 inColor;
layout(location = 3) in vec3 inNormal;

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec3 outWorldPos;
layout(location = 2) out vec3 outLightPos;
layout(location = 3) out vec3 outViewPos;

void main() {
    vec3 light = vec3(ubo.viewPos);
	
	vec3 position = primitive.model[3].xyz;
	/*
	vec3 normal = mat3(primitive.model) * inNormal;
	
outColor = vec3(clamp(dot(normalize(light - position), normal) * 0.7, .1f, 1),
					clamp(dot(normalize(light - position), normal) * 0.7, .1f, 1),
					clamp(dot(normalize(light - position), normal) * 0.7, .1f, 1));
*/
	
    gl_Position = ubo.proj * ubo.view * primitive.model * vec4(inPosition.xyz, 1.0);
	outColor = inColor;
	outWorldPos = position;
	outLightPos = ubo.lights[0].xyz;
	outViewPos = (ubo.view * primitive.model * vec4(inPosition.xyz, 1.0)).xyz;
}