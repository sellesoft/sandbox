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

layout(location = 0) out vec4  outColor;
layout(location = 1) out vec2  outTexCoord;
layout(location = 2) out vec3  outNormal;
layout(location = 3) out float time;
layout(location = 4) out vec2  screen;
layout(location = 5) out vec3  camerapos;

void main() {
	
	vec4 pos = ubo.proj * ubo.view * primitive.model * vec4(inPosition.xyz, 1.0);
	
	time = ubo.time;
	
	camerapos = vec3(ubo.view[3][0], ubo.view[3][1], ubo.view[3][2]);
	
	gl_Position = pos;
	
    //gl_Position = pos + 0.1 * sin(time * gl_VertexIndex / 2) + 0.1 * cos(time * gl_VertexIndex / 2);
    outColor = inColor;
	outTexCoord = inTexCoord;
	outNormal = mat3(primitive.model) * inNormal;
	
	screen = ubo.screen;
	
}