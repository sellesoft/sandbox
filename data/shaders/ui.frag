#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_KHR_vulkan_glsl : enable

layout(set=0, binding=0) uniform sampler2D tex;

layout(location = 0) in vec2 inUV;
layout(location = 1) in vec4 inColor;

layout(location = 0) out vec4 outColor;

void main() {
	outColor = inColor * texture(tex, inUV);
	//outColor = vec4(1,1,1,1);
}