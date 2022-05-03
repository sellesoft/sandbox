#version 330 core
#extension GL_ARB_separate_shader_objects : enable
//#extension GL_EXT_debug_printf : enable

uniform UniformBufferObject{
	mat4  view;
	mat4  proj;
	vec4  lights[10];
	vec4  viewPos;
	vec2  screen;
	vec2  mousepos;
	vec3  mouseWorld;
	float time;
	mat4  depthMVP;
	int   enablePCF;
} ubo;

uniform PushConsts{
	mat4 model;
} primitive;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec4 inColor;
layout(location = 3) in vec3 inNormal;

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec2 outTexCoord;
layout(location = 2) out vec3 outNormal;
layout(location = 3) out float outLightBrightness;
layout(location = 4) out vec3 outWorldPos;
layout(location = 5) out vec3 viewPosition;
layout(location = 6) flat out int  outEnablePCF;
layout(location = 7) out vec4 outShadowCoord;
layout(location = 8) out vec3 outLightVec;
layout(location = 9) out vec3 outViewVec;
layout(location = 10) out vec4 outLights[10];

//translation to shadow map space [-1...1] -> [0...1]: (xy*0.5 + 0.5)
const mat4 biasMat = mat4(0.5, 0.0, 0.0, 0.0,
						  0.0, 0.5, 0.0, 0.0,
						  0.0, 0.0, 1.0, 0.0,
						  0.5, 0.5, 0.0, 1.0);

void main() {
	gl_Position = ubo.proj * ubo.view * primitive.model * vec4(inPosition.xyz, 1.0);
    outColor = inColor;
	outTexCoord = inTexCoord;
	outNormal = mat3(primitive.model) * inNormal;
	//outLightBrightness = ubo.lightPos.w;
	outWorldPos = vec3(primitive.model * vec4(inPosition.xyz, 1.0));
	outLights = ubo.lights;
	viewPosition = (ubo.view * primitive.model * vec4(inPosition.xyz, 1.0)).xyz;
	
	if(ubo.enablePCF != 0) outEnablePCF = 1;
	outShadowCoord = (biasMat * ubo.depthMVP * primitive.model) * vec4(inPosition.xyz, 1.0);
	outLightVec = normalize(ubo.lights[0].xyz - inPosition);
	vec4 pos = primitive.model * vec4(inPosition.xyz, 1.0);
	outViewVec = -pos.xyz;
}