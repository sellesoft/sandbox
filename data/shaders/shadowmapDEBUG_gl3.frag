#version 330 core
#extension GL_ARB_separate_shader_objects : enable

uniform sampler2D shadowMap;

layout(location = 0) in vec2 inUV;

layout(location = 0) out vec4 outColor;

float LinearizeDepth(float depth){
	float n = 1; // camera z near
	float f = 96; // camera z far
	float z = depth;
	return (2.0 * n) / (f + n - z * (f - n));	
}

void main(){
	float depth = texture(shadowMap, inUV).r;
	outColor = vec4(vec3(LinearizeDepth(depth)), 1.0);
}