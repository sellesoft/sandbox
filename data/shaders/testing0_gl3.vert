#version 330 core
#extension GL_ARB_separate_shader_objects : enable

uniform UniformBufferObject{
	mat4  view;
	mat4  proj;
	vec4  lights[10];
	vec4  viewPos;
	vec2  screen;
	vec2  mousepos;
	vec3  mouseWorld;
	float time;
} ubo;

uniform PushConsts{
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
layout(location = 6) out vec3  fragPos;
layout(location = 7) out vec2  mousePos;
layout(location = 8) out vec3  mouseworld;

void main() {
	
	outColor = inColor;
	outTexCoord = inTexCoord;
	outNormal = mat3(primitive.model) * inNormal;
	fragPos = vec3(primitive.model * vec4(inPosition, 1));
	mousePos = ubo.mousepos;
	camerapos = ubo.viewPos.xyz;
	mouseworld = ubo.mouseWorld;
	time = ubo.time;
	screen = ubo.screen;
	
	gl_Position = ubo.proj * ubo.view * primitive.model * vec4(inPosition, 1.0);
	
}