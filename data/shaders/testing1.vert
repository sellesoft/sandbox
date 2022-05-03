#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_KHR_vulkan_glsl : enable

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

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec2 outTexCoord;
layout(location = 2) out vec3 outNormal;

vec3 quant3(vec3 v, float res){
	return vec3(floor(v.x * res) / res, floor(v.y * res) / res, floor(v.z * res) / res);
}


vec4 quant4(vec4 v, float res){
	return vec4(floor(v.x * res) / res,floor(v.y * res) / res, floor(v.z * res) / res, floor(v.w * res) / res);
}

vec3 VectorPlaneIntersect(vec3 planep, vec3 planen, vec3 ls, vec3 le){
	float plane_d = dot(-planen, planep);
	float ad = dot(ls, planen);
	float bd = dot(le, planen);
	float t = (-plane_d - ad) / (bd - ad);
	vec3 line_start_to_end = le - ls;
	vec3 line_to_intersect = line_start_to_end * t;
	return ls + line_to_intersect;
	
}

void main() {
    vec3 light = vec3(ubo.viewPos);
	
	vec3 normal = mat3(primitive.model) * inNormal;
	vec3 position = primitive.model[3].xyz;
	
	vec4 worldpos = primitive.model * vec4(inPosition.xyz, 1.0);
	
	
	vec3 mouseinter = VectorPlaneIntersect(worldpos.xyz, normal, ubo.viewPos.xyz, ubo.mouseWorld);
	
	float dist = length(worldpos.xyz - mouseinter);
	
	vec3 mtwp = normalize(worldpos.xyz - mouseinter);
	
	//worldpos.xyz += clamp(mtwp * 0.2 / (clamp(dist, 0.001, 100.)), vec3(0,0,0), vec3(0.08,0.08,0.08));
	
	vec4 qpos = quant4(worldpos, 20);
	
	//worldpos.xyz += vec3(0,sin(ubo.time),0);
	
    gl_Position = ubo.proj * ubo.view * worldpos;
	
	vec2 mp = (vec2(ubo.mousepos.x / ubo.screen.x - 1, ubo.mousepos.y / ubo.screen.y - 1)) / 2;
	
	float sdist = length(gl_Position.xy - mp);
	vec2 mptp = normalize(gl_Position.xy - mp);
	
	gl_Position += vec4(mptp, gl_Position.z, gl_Position.w) * 0.2 / (clamp(sdist, 0.1, 100.));
	
	
	//gl_Position = vec4(quant3(gl_Position.xyz, 20), gl_Position.w);
	outColor = vec4(clamp(dot(normalize(light - position), normal) * 0.9, .1f, 1),
					clamp(dot(normalize(light - position), normal) * 0.9, .1f, 1),
					clamp(dot(normalize(light - position), normal) * 0.9, .1f, 1), 1.0f);
	
	outColor *= vec4(normalize(inPosition.xyz) * 0.05 * sin(gl_VertexIndex * ubo.time / 2000) * vec3(1, 0.3, 0.2), 1.0f);
	
	//outColor = vec3(dist / 20);
	
	outTexCoord = inTexCoord;
	outNormal = normal;
}