#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_KHR_vulkan_glsl : enable

layout(set = 1, binding = 0) uniform sampler2D albedoSampler;
layout(set = 1, binding = 0) uniform sampler2D normalSampler;
layout(set = 1, binding = 0) uniform sampler2D specularSampler;
layout(set = 1, binding = 0) uniform sampler2D lightSampler;

//layout(constant_id = 0) const bool ALPHA_MASK = false;
//layout(constant_id = 1) const float ALPHA_THRESHOLD = 0.0f;

layout(location = 0) in vec4  inColor;
layout(location = 1) in vec2  inTexCoord;
layout(location = 2) in vec3  inNormal;
layout(location = 3) in float time;
layout(location = 4) in vec2  screen;
layout(location = 5) in vec3  cp;


layout(location = 0) out vec4 outColor;

ivec2 texsize;
vec4 fc = gl_FragCoord;
vec2 tc = inTexCoord;

void main() {
	//TODO(delle,r) pbr shading, see pbrtexture in saschawillems' stuff
	//outColor = texture(albedoSampler, inTexCoord);
	
	float pi = 3.14159265359;
	
	fc.x /= screen.x;
	fc.y /= screen.y;
	texsize = textureSize(albedoSampler, 0);
	
	tc.x -= 0.5;
	tc.y -= 0.5;
	tc.x *= 2;
	tc.y *= 2;
	
	float dist = log(length(fc - vec4(cp, 1)));
	
	float res = 150; //floor(300 * fc.y / fc.x + 1); //(sin(time) + 1)/2 + 150;
	int colors = 1;
	
	float xpart = tc.x + (floor(fc.x * res)/res * (floor(fc.x * res)/res * cos(time) - floor(fc.y * res)/res * sin(time))- 0.1);
	float ypart = tc.y + (floor(fc.y * res)/res * (floor(fc.x * res)/res * sin(time) + floor(fc.y * res)/res * cos(time))- 0.1);
	
	float eq1 = floor((((xpart - 2 * abs(sin(0.2 * time))) * (xpart - 2 * abs(sin(0.2 * time))) * (ypart - 0.4 * abs(cos(0.2 * time)) - 0.4) * (ypart - 0.4 * abs(cos(0.2 * time)) - 0.4) - 0.1)) * colors) / colors;
	float eq2 = floor((sin(99 * floor(fc.x * res)/res - cos(10 * floor(fc.y * res)/res) + 0.8 * time) + cos(20 * floor(fc.y*res)/res + time) + sin(fc.x + 20 * sin(time / 20) * fc.x * fc.y))*colors)/colors;
	float eq3 = floor((cos(100 * floor(fc.x * res)/res + sin(10 * floor(fc.y * res)/res) + time) + sin(abs(sin(2 * time) + 20) * floor(fc.y*res)/res + time) - cos(sin(time/ 20) * fc.x * fc.x * 20))*colors)/colors;
	
	vec4 col1 = vec4(0.1 * eq2, 0.3 * eq2, 0.8 * eq2, 1);
	vec4 col2 = vec4(0.1 * eq3, 0.1 * eq2, 0.6 * eq3, 1);
	vec4 col3 = vec4(0.1 * eq1, 0.05 * eq1, 0.05 * eq1, 1);
	
	vec4 mix1 = mix(col2, col1, sin(2 * time + cos(2 * time)));
	
	
	
	vec4 col4 = floor(vec4((cos(6 * pi * floor(tc.x * res)/res + sin(6 * pi * floor(tc.y * res)/res + time) + 2 * time) + sin(12 * pi * floor(tc.y * res)/res) - cos(3 * pi * floor(tc.x * res)/res + sin(6 * pi * floor(tc.y * res)/res + time))), 0, 0, 1) * colors)/colors;
	//vec4 col4 = floor(vec4(cos(2 * pi * floor(tc.x * res)/res) * cos(2 * pi * floor(tc.y * res)/res) + sin(floor(tc.x * res)/res + cos(floor(tc.y * res)/res + time)), 0.5, 0.5, 1) * colors) / colors;
	
	
	vec4 mix2 = mix(mix1, col3, abs(0.5 * cos(time)));
	
	outColor = mix(col4, mix2, clamp(dist / 10, 0, 1));//cos(time));
	
	//outColor = vec4(1, 1, 1, 1);
	//outColor = vec4(0.5, inTexCoord.x, inTexCoord.y, 1);
	
	//outColor = inColor;
}