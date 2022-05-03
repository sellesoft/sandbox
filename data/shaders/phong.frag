#version 450
#extension GL_ARB_separate_shader_objects : enable
//#extension GL_EXT_debug_printf  : enable

layout(binding = 1) uniform sampler2D shadowMap;

layout(location = 0) in vec4 inColor;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in float lightBrightness;
layout(location = 4) in vec3 worldPosition;
layout(location = 5) in vec3 viewPosition;
layout(location = 6) in flat int  inEnablePCF;
layout(location = 7) in vec4 inShadowCoord;
layout(location = 8) in vec3 inLightVec;
layout(location = 9) in vec3 inViewVec;
layout(location = 10) in vec4 lights[10];

layout(location = 0) out vec4 outColor;

#define ambient 0.1

//ref: https://github.com/SaschaWillems/Vulkan/blob/master/data/shaders/glsl/shadowmapping/scene.frag
float textureProj(vec4 shadowCoord, vec2 off){
	float shadow = 1.0;
	if(shadowCoord.z > -1.0 && shadowCoord.z < 1.0){ //check if coords are within the texture's bounds
		float dist = texture(shadowMap, shadowCoord.st + off).r; //st == xy
		if(shadowCoord.w > 0.0 && dist < shadowCoord.z){
			shadow = ambient;
		}
	}
	return shadow;
}
float filterPCF(vec4 sc){
	ivec2 texDim = textureSize(shadowMap, 0);
	float scale = 1.5;
	float dx = scale * 1.0 / float(texDim.x);
	float dy = scale * 1.0 / float(texDim.y);
	
	float shadowFactor = 0.0;
	int count = 0;
	int range = 1;
	
	for(int x = -range; x <= range; x++){
		for(int y = -range; y <= range; y++){
			shadowFactor += textureProj(sc, vec2(dx*x, dy*y));
			count++;
		}
	}
	return shadowFactor / count;
}

vec4 calcLight(vec3 lightPos, vec3 worldPos, float brightness, vec3 normal){
	float r = length(lightPos - worldPos);
	float ang = brightness * dot(normalize(lightPos - worldPos), normal);
	float light = clamp(ang/(r * r), 0, 1);
	
	return  vec4(light, light, light, 1);
}

vec4 calcLight(vec3 lightPos, vec3 worldPos, float brightness, vec3 normal, vec3 color){
	float r = length(lightPos - worldPos);
	float ang = brightness * dot(normalize(lightPos - worldPos), normal);
	float light = clamp(ang/(r * r), 0, 1);
	
	return  vec4(light, light, light, 1) * vec4(color, 1);
}

void main() {
	/*
	outColor = vec4(0,0,0,1);
	for(int i = 0; i < 10; i++){
		if(lights[i].w != -1 && lights[i].w != 0){
			outColor += calcLight(lights[i].xyz, worldPosition, lights[i].w, inNormal);
			//outColor = clamp(outColor, vec4(0,0,0,0), vec4(1,1,1,1));
		}
	}
	*/
	//outColor = vec4(outColor.xyz * shadow, 1.0);
	
	
	
	float shadow = (inEnablePCF == 1) ? filterPCF(inShadowCoord / inShadowCoord.w) : textureProj(inShadowCoord / inShadowCoord.w, vec2(0.0));
	
	vec3 N = normalize(inNormal);
	vec3 L = normalize(inLightVec);
	vec3 V = normalize(inViewVec);
	vec3 R = normalize(-reflect(L, N));
	vec4 diffuse = max(dot(N, L), ambient) * inColor;
	outColor = shadow * diffuse;
}