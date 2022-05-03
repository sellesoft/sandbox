#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_KHR_vulkan_glsl : enable

layout(set = 1, binding = 0) uniform sampler2D nullSampler;

layout(location = 0) in vec2  inUV;
layout(location = 1) in float time;

layout(location = 0) out vec4 outColor;

float quant(float v, float res){
	return floor(v * res) / res;
}

float rand(vec2 st) {
    return fract(sin(dot(st.xy, vec2(12.9898,78.233)))*43758.5453123);
}

vec4 dither(){
	
	vec2 texSize = textureSize(nullSampler, 0);
	vec2 texInc = vec2(1 / texSize.x, 1/ texSize.y);
	
	//vec2 tc = inUV;
	float seed_float = rand(vec2(time,time/2.f));
	vec2 tc = vec2(seed_float, -seed_float);
	tc = vec2(quant(tc.x, texSize.x), quant(tc.y, texSize.y));
	vec2 randControl = vec2(tc.x + quant(time, 5), tc.y + quant(time, 5));
	float random = floor(rand(randControl) * 5);
	//return texture(nullSampler, tc);
	
	vec2 tcm = floor(tc * texSize);
	if(mod(tcm.y, 2.0) == 0 && mod(tcm.x, 2.0) == 0 && mod(random, 2) == 0){
		vec2 nextSamp = tc;
		if(mod(random, 4) == 0){
			nextSamp += texInc ;
		}else{
			nextSamp -= texInc ;
		}
		vec4 currCol = texture(nullSampler, tc);
		vec4 nextCol = texture(nullSampler, nextSamp);
		
		if(nextCol != currCol){
			return nextCol;
		}else {
			return currCol;
		}
	}else{
		vec2 nextSamp = tc;
		if(mod(random, 2) == 0){
			nextSamp += texInc ;
		}else{
			nextSamp -= texInc ;
		}
		vec4 currCol = texture(nullSampler, tc);
		vec4 nextCol = texture(nullSampler, nextSamp);
		
		if(nextCol != currCol){
			return nextCol;
		}
		else {
			return currCol;
		}
	}
	
}

void main() {
	outColor = dither();
	outColor.a = 1;
}