#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec4 inColor;
layout(location = 1) in vec3 inWorldPos;
layout(location = 2) in vec3 inLightPos;
layout(location = 3) in vec3 inViewPos;

layout(location = 0) out vec4 outColor;

void main() {
	vec3 xTangent = dFdx( inViewPos );
    vec3 yTangent = dFdy( inViewPos );
    vec3 faceNormal = normalize( cross( xTangent, yTangent ) );
	outColor = vec4(clamp(dot(normalize(inLightPos - inWorldPos), faceNormal) * 0.7, .1f, 1),
					clamp(dot(normalize(inLightPos - inWorldPos), faceNormal) * 0.7, .1f, 1),
					clamp(dot(normalize(inLightPos - inWorldPos), faceNormal) * 0.7, .1f, 1),
					1.0f);
	//outColor = inColor;
}