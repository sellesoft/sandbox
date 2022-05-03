#version 330 core
#extension GL_ARB_separate_shader_objects : enable

uniform sampler2D tex;

layout(location = 0) in vec2 inUV;
layout(location = 1) in vec4 inColor;

layout(location = 0) out vec4 outColor;

void main() {
	outColor = inColor * texture(tex, inUV);
	//outColor = vec4(1,1,1,1);
}